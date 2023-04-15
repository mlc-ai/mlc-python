from __future__ import annotations

import ctypes
import functools
import typing
from collections.abc import Callable

from mlc._cython import (
    MLCHeader,
    TypeField,
    TypeInfo,
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
from mlc.core import Object

from .utils import (
    Structure,
    add_vtable_methods_for_type_cls,
    get_parent_type,
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

        if not issubclass(super_type_cls, PyClass):
            raise TypeError(
                "Not a subclass of `mlc.PyClass`: "
                f"`{super_type_cls.__module__}.{super_type_cls.__qualname__}`"
            )

        # Step 1. Create the type according to its parent type
        parent_type_info: TypeInfo = get_parent_type(super_type_cls)._mlc_type_info  # type: ignore[attr-defined]
        type_info: TypeInfo = type_create(parent_type_info.type_index, type_key)
        type_index = type_info.type_index

        # Step 2. Reflect all the fields of the type
        fields, d_fields = inspect_dataclass_fields(
            type_key,
            super_type_cls,
            parent_type_info,
        )
        num_bytes = _add_field_properties(fields)
        type_info.fields = tuple(fields)
        type_info.d_fields = tuple(d_fields)
        type_register_fields(type_index, fields)
        mlc_init = make_mlc_init(fields)

        # Step 3. Create the proxy class with the fields as properties
        @functools.wraps(super_type_cls, updated=())
        class type_cls(super_type_cls):  # type: ignore[valid-type,misc]
            __slots__ = ()

            def _mlc_init(self, *args: typing.Any) -> None:
                mlc_init(self, *args)

            def __new__(cls, *args: typing.Any, **kwargs: typing.Any) -> type[ClsType]:
                return type_create_instance(cls, type_index, num_bytes)

        type_info.type_cls = type_cls
        setattr(type_cls, "_mlc_type_info", type_info)
        for field in fields:
            attach_field(
                type_cls,
                name=field.name,
                getter=field.getter,
                setter=field.setter,
                frozen=field.frozen,
            )

        # Step 4. Register the structure of the class
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
        type_register_structure(
            type_index,
            struct_kind,
            sub_structure_indices=tuple(sub_structure_indices),
            sub_structure_kinds=tuple(sub_structure_kinds),
        )
        setattr(type_cls, "_mlc_structure", struct)

        # Step 5. Attach methods
        fn: Callable[..., typing.Any]
        type_add_method(type_index, "__init__", type_cls, 1)  # static
        if init:
            fn = method_init(super_type_cls, d_fields)
            attach_method(super_type_cls, type_cls, "__init__", fn, check_exists=True)
        if repr:
            fn = _method_repr(type_key, fields)
            type_add_method(type_index, "__str__", fn, 1)  # static
            attach_method(super_type_cls, type_cls, "__repr__", fn, check_exists=True)
            attach_method(super_type_cls, type_cls, "__str__", fn, check_exists=True)
        add_vtable_methods_for_type_cls(super_type_cls, type_index=type_index)
        return type_cls

    return decorator


def _add_field_properties(type_fields: list[TypeField]) -> int:
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
    return ctypes.sizeof(CType)


def _method_repr(
    type_key: str,
    fields: list[TypeField],
) -> Callable[[ClsType], str]:
    field_names = tuple(field.name for field in fields)

    def method(self: ClsType) -> str:
        fields = (f"{name}={getattr(self, name)!r}" for name in field_names)
        return f"{type_key}({', '.join(fields)})"

    return method
