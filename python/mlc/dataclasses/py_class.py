from __future__ import annotations

try:
    from typing import dataclass_transform
except ImportError:
    from typing_extensions import dataclass_transform

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

from .utils import Field as _Field
from .utils import (
    Structure,
    add_vtable_methods_for_type_cls,
    get_parent_type,
    inspect_dataclass_fields,
    method_init,
    structure_parse,
    structure_to_c,
)
from .utils import field as _field

InputClsType = typing.TypeVar("InputClsType")


@dataclass_transform(field_specifiers=(_field, _Field))
def py_class(
    type_key: str | type | None = None,
    *,
    init: bool = True,
    repr: bool = True,
    frozen: bool = False,
    structure: typing.Literal["bind", "nobind", "var"] | None = None,
) -> Callable[[type[InputClsType]], type[InputClsType]]:
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

    def decorator(super_type_cls: type[InputClsType]) -> type[InputClsType]:
        nonlocal type_key
        if type_key is None:
            type_key = f"{super_type_cls.__module__}.{super_type_cls.__qualname__}"
        assert isinstance(type_key, str)

        # Step 1. Create the type according to its parent type
        parent_type_info: TypeInfo = get_parent_type(super_type_cls)._mlc_type_info  # type: ignore[attr-defined]
        type_info: TypeInfo = type_create(parent_type_info.type_index, type_key)
        type_index = type_info.type_index

        # Step 2. Reflect all the fields of the type
        fields, d_fields = inspect_dataclass_fields(
            type_key,
            super_type_cls,
            parent_type_info,
            frozen=frozen,
        )
        num_bytes = _add_field_properties(fields)
        type_info.fields = tuple(fields)
        type_info.d_fields = tuple(d_fields)
        type_register_fields(type_index, fields)
        mlc_init = make_mlc_init(fields)

        # Step 3. Create the proxy class with the fields as properties
        type_cls: type[InputClsType] = _create_cls(
            cls=super_type_cls,
            mlc_init=mlc_init,
            mlc_new=lambda cls, *args, **kwargs: type_create_instance(cls, type_index, num_bytes),
        )

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

        # Step 5. Add `__init__` method
        type_add_method(type_index, "__init__", _method_new(type_cls), 1)  # static
        # Step 6. Attach methods
        fn: Callable[..., typing.Any]
        if init:
            fn = method_init(super_type_cls, d_fields)
            attach_method(super_type_cls, type_cls, "__init__", fn, check_exists=True)
        if repr:
            fn = _method_repr(type_key, fields)
            type_add_method(type_index, "__str__", fn, 1)  # static
            attach_method(super_type_cls, type_cls, "__repr__", fn, check_exists=True)
            attach_method(super_type_cls, type_cls, "__str__", fn, check_exists=True)
        elif (fn := vars(super_type_cls).get("__str__", None)) is not None:
            assert callable(fn)
            type_add_method(type_index, "__str__", fn, 1)
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
) -> Callable[[InputClsType], str]:
    field_names = tuple(field.name for field in fields)

    def method(self: InputClsType) -> str:
        fields = (f"{name}={getattr(self, name)!r}" for name in field_names)
        return f"{type_key}({', '.join(fields)})"

    return method


def _method_new(type_cls: type[InputClsType]) -> Callable[..., InputClsType]:
    def method(*args: typing.Any) -> InputClsType:
        obj = type_cls.__new__(type_cls)
        obj._mlc_init(*args)  # type: ignore[attr-defined]
        return obj

    return method


def _create_cls(
    cls: type,
    mlc_init: Callable[..., None],
    mlc_new: Callable[..., None],
) -> type[InputClsType]:
    cls_name = cls.__name__
    cls_bases = cls.__bases__
    attrs = dict(cls.__dict__)
    if cls_bases == (object,):
        cls_bases = (Object,)

    def _add_method(fn: Callable, fn_name: str) -> None:
        attrs[fn_name] = fn
        fn.__module__ = cls.__module__
        fn.__name__ = fn_name
        fn.__qualname__ = f"{cls_name}.{fn_name}"

    attrs["__slots__"] = ()
    attrs.pop("__dict__", None)
    attrs.pop("__weakref__", None)
    _add_method(mlc_init, "_mlc_init")
    _add_method(mlc_new, "__new__")

    new_cls = type(cls_name, cls_bases, attrs)
    new_cls.__module__ = cls.__module__
    new_cls = functools.wraps(cls, updated=())(new_cls)  # type: ignore
    return new_cls
