from __future__ import annotations

import ctypes
import functools
import typing
from collections.abc import Callable

from mlc._cython import (
    TypeField,
    TypeInfo,
    TypeMethod,
    attach_field,
    attach_method,
    make_mlc_init,
    type_add_method,
    type_create,
    type_create_instance,
    type_field_get_accessor,
    type_register_fields,
    type_register_structure,
)
from mlc.core.object import Object

from .utils import (
    Structure,
    inspect_dataclass_fields,
    method_init,
    structure_parse,
    structure_to_c,
)

ClsType = typing.TypeVar("ClsType")


class PyClass(Object):
    _mlc_type_info = Object._mlc_type_info

    def __init__(self, *args: typing.Any, **kwargs: typing.Any) -> None:
        raise NotImplementedError

    def __str__(self) -> str:
        return self.__repr__()


def py_class(
    type_key: str | type | None = None,
    *,
    init: bool = True,
    repr: bool = True,
    structure: typing.Literal["bind", "nobind", "var"] | None = None,
) -> Callable[[type[ClsType]], type[ClsType]]:
    if isinstance(type_key, type):
        return py_class(
            type_key=None,
            init=init,
            repr=repr,
            structure=structure,
        )(type_key)
    if structure not in (None, "bind", "nobind", "var"):
        raise ValueError(
            "Invalid `structure`. Expected one of: 'none', 'bind', 'nobind', 'var', "
            f"but got: {structure}"
        )

    def decorator(super_type_cls: type[ClsType]) -> type[ClsType]:
        nonlocal type_key
        if type_key is None:
            type_key = f"{super_type_cls.__module__}.{super_type_cls.__qualname__}"
        assert isinstance(type_key, str)

        extra_methods: dict[str, Callable] = {}
        methods: list[TypeMethod] = []
        # Step 1. Extract fields and methods
        parent_type_info: TypeInfo = _type_parent(super_type_cls)._mlc_type_info  # type: ignore[attr-defined]
        fields, d_fields = inspect_dataclass_fields(type_key, super_type_cls, parent_type_info)
        CType = _model_as_ctype(fields)
        # Step 2. Add extra methods
        if init:
            extra_methods["__init__"] = method_init(super_type_cls, d_fields)
        if repr:
            extra_methods["__repr__"] = _insert_to_method_list(
                methods,
                name="__str__",
                fn=_method_repr(type_key, fields),
                is_static=True,
            )
        # Step 3. Figure out the structure of the class
        struct: Structure
        struct_kind: int
        sub_structure_indices: list[int]
        sub_structure_kinds: list[int]
        if (struct := vars(super_type_cls).get("_mlc_structure", None)) is not None:
            assert isinstance(struct, Structure)
        else:
            struct = structure_parse(structure, d_fields)
        (
            struct_kind,
            sub_structure_indices,
            sub_structure_kinds,
        ) = structure_to_c(struct, fields)
        if struct.kind is None:
            assert struct_kind == 0
            assert not sub_structure_indices
            assert not sub_structure_kinds
        # Step 4. Register the type
        num_bytes = ctypes.sizeof(CType)
        type_info: TypeInfo = type_create(parent_type_info.type_index, type_key)
        mlc_init = make_mlc_init(fields)
        type_index = type_info.type_index

        @functools.wraps(super_type_cls, updated=())
        class type_cls(super_type_cls):  # type: ignore[valid-type,misc]
            __slots__ = ()

            def _mlc_init(self, *args: typing.Any) -> None:
                mlc_init(self, *args)

            def __new__(cls, *args: typing.Any, **kwargs: typing.Any) -> type[ClsType]:
                return type_create_instance(cls, type_index, num_bytes)

        type_info.type_cls = type_cls
        type_info.fields = tuple(fields)
        type_register_fields(type_index, fields)
        type_register_structure(
            type_index,
            struct_kind,
            sub_structure_indices=tuple(sub_structure_indices),
            sub_structure_kinds=tuple(sub_structure_kinds),
        )
        type_add_method(type_index, "__init__", type_cls, 1)
        for method in methods:
            type_add_method(type_index, method.name, method.func, method.kind)

        # Step 5. Attach fields
        setattr(type_cls, "_mlc_dataclass_fields", {})
        for field in type_info.fields:
            attach_field(
                type_cls,
                name=field.name,
                getter=field.getter,
                setter=field.setter,
                frozen=field.frozen,
            )

        # Step 5. Attach methods
        for method_name, fn in extra_methods.items():
            attach_method(
                type_cls,
                name=method_name,
                method=fn,
            )
        setattr(type_cls, "_mlc_type_info", type_info)
        setattr(type_cls, "_mlc_structure", struct)
        return type_cls

    return decorator


def _type_parent(type_cls: type) -> type:
    if not issubclass(type_cls, PyClass):
        raise TypeError(
            f"Not a subclass of `mlc.PyClass`: `{type_cls.__module__}.{type_cls.__qualname__}`"
        )
    for base in type_cls.__bases__:
        if hasattr(base, "_mlc_type_info"):
            return base
    raise ValueError(
        f"No parent found for `{type_cls.__module__}.{type_cls.__qualname__}`. It must inherit from `mlc.PyClass`."
    )


def _model_as_ctype(type_fields: list[TypeField]) -> type[ctypes.Structure]:
    from mlc._cython import MLCHeader

    c_fields = [("_mlc_header", MLCHeader)]
    for type_field in type_fields:
        field_name = type_field.name
        field_ty_c = type_field.ty._ctype()
        c_fields.append((field_name, field_ty_c))

    class CType(ctypes.Structure):
        _fields_ = c_fields

    for field in type_fields:
        field.offset = getattr(CType, field.name).offset
        field.getter, field.setter = type_field_get_accessor(field)
    return CType


def _insert_to_method_list(
    methods: list[TypeMethod],
    name: str,
    fn: Callable[..., typing.Any],
    is_static: bool,
) -> Callable[..., typing.Any]:
    from mlc.core import Func

    new_method = TypeMethod(
        name=name,
        func=Func(fn),
        kind=1 if is_static else 0,
    )

    for i, method in enumerate(methods):
        if method.name == name:
            methods[i] = new_method
            return fn
    methods.append(new_method)
    return fn


def _method_repr(
    type_key: str,
    fields: list[TypeField],
) -> Callable[[ClsType], str]:
    field_names = tuple(field.name for field in fields)

    def method(self: ClsType) -> str:
        fields = (f"{name}={getattr(self, name)!r}" for name in field_names)
        return f"{type_key}({', '.join(fields)})"

    return method
