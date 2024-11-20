from __future__ import annotations

import ctypes
import typing
from collections.abc import Callable

from mlc._cython import TypeField, TypeInfo, TypeMethod, type_create, type_field_get_accessor

from .utils import attach_field, attach_method, method_init

ClsType = typing.TypeVar("ClsType")


def py_class(
    type_key: str,
    init: bool = True,
    repr: bool = True,
) -> Callable[[type[ClsType]], type[ClsType]]:
    def decorator(super_type_cls: type[ClsType]) -> type[ClsType]:
        extra_methods: dict[str, Callable] = {}
        methods: list[TypeMethod] = []
        fields: list[TypeField]
        type_cls: type[ClsType]
        type_info: TypeInfo

        # Step 1. Extract fields and methods
        parent_type_info: TypeInfo = _type_parent(super_type_cls)._mlc_type_info  # type: ignore[attr-defined]
        type_hints = typing.get_type_hints(super_type_cls)
        CType, fields = _model_as_ctype(type_key, type_hints, parent_type_info)

        # Step 2. Add extra methods
        if init:
            extra_methods["__init__"] = method_init(super_type_cls, type_hints, fields)
        if repr:
            extra_methods["__repr__"] = _insert_to_method_list(
                methods,
                name="__str__",
                fn=_method_repr(type_key, fields),
                is_static=True,
            )
        # Step 3. Register the type
        type_cls, type_info = type_create(
            super_type_cls,
            type_key=type_key,
            fields=fields,
            methods=methods,
            parent_type_index=parent_type_info.type_index,
            num_bytes=ctypes.sizeof(CType),
        )

        # Step 4. Attach fields
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
        return type_cls

    return decorator


def _type_parent(type_cls: type) -> type:
    from mlc.core.object import PyClass

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


def _model_as_ctype(
    type_key: str,
    type_hints: dict[str, type],
    parent_type_info: TypeInfo,
) -> tuple[type[ctypes.Structure], list[TypeField]]:
    from mlc._cython import MLCHeader
    from mlc.core import typing as mlc_typing

    type_hints = dict(type_hints)
    fields: list[TypeField] = []
    c_fields = [("_mlc_header", MLCHeader)]
    for type_field in parent_type_info.fields:
        field_name = type_field.name
        if type_hints.pop(field_name, None) is None:
            raise ValueError(
                f"Missing field `{type_key}::{field_name}`, "
                f"which appears in its parent class `{parent_type_info.type_key}`."
            )
        field_ty = type_field.ty
        field_ty_c = field_ty._ctype()
        c_fields.append((field_name, field_ty_c))
        fields.append(
            TypeField(
                name=field_name,
                offset=-1,
                num_bytes=ctypes.sizeof(field_ty_c),
                frozen=False,
                ty=field_ty,
            )
        )

    for field_name, field_ty_py in type_hints.items():
        if field_name.startswith("_"):
            continue
        field_ty = mlc_typing.from_py(field_ty_py)
        field_ty_c = field_ty._ctype()
        c_fields.append((field_name, field_ty_c))
        fields.append(
            TypeField(
                name=field_name,
                offset=-1,
                num_bytes=ctypes.sizeof(field_ty_c),
                frozen=False,
                ty=field_ty,
            )
        )

    class CType(ctypes.Structure):
        _fields_ = c_fields

    for field in fields:
        field.offset = getattr(CType, field.name).offset
        field.getter, field.setter = type_field_get_accessor(field)
    return CType, fields


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
