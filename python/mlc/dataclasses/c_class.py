import functools
import typing
import warnings
from collections.abc import Callable

from mlc._cython import (
    TypeInfo,
    TypeMethod,
    attach_field,
    attach_method,
    type_index2type_methods,
    type_key2py_type_info,
)

from .utils import inspect_dataclass_fields, method_init, prototype_py

ClsType = typing.TypeVar("ClsType")


def c_class(
    type_key: str,
    init: bool = True,
) -> Callable[[type[ClsType]], type[ClsType]]:
    def decorator(super_type_cls: type[ClsType]) -> type[ClsType]:
        @functools.wraps(super_type_cls, updated=())
        class type_cls(super_type_cls):  # type: ignore[valid-type,misc]
            __slots__ = ()

        # Step 1. Retrieve `type_info` and set `_mlc_type_info`
        type_info: TypeInfo = type_key2py_type_info(type_key)
        if type_info.type_cls is not None:
            raise ValueError(f"Type is already registered: {type_key}")
        type_info.type_cls = type_cls
        setattr(type_cls, "_mlc_type_info", type_info)

        # Step 2. Check if all fields are exposed as type annotations
        if not type_key.startswith("mlc.core.typing."):
            _check_c_class(type_key, super_type_cls, type_info)

        # Step 3. Attach fields
        setattr(type_cls, "_mlc_dataclass_fields", {})
        for field in type_info.fields:
            attach_field(
                cls=type_cls,
                name=field.name,
                getter=field.getter,
                setter=field.setter,
                frozen=field.frozen,
            )

        # Step 4. Attach methods
        method: TypeMethod
        for method in type_index2type_methods(type_info.type_index):
            if not method.name.startswith("_"):
                attach_method(
                    parent_cls=super_type_cls,
                    cls=type_cls,
                    name=method.name,
                    method=method.as_callable(),
                    check_exists=False,
                )
        if init:
            _, d_fields = inspect_dataclass_fields(type_key, type_cls, type_info)
            attach_method(
                parent_cls=super_type_cls,
                cls=type_cls,
                name="__init__",
                method=method_init(super_type_cls, d_fields),
                check_exists=True,
            )
        return type_cls

    return decorator


def _check_c_class(
    type_key: str,
    type_cls: type[ClsType],
    type_info: TypeInfo,
) -> None:
    fields, _ = inspect_dataclass_fields(type_key, type_cls, type_info)
    type_hints = {field.name: field.ty for field in fields}
    warned: bool = False
    for field in type_info.fields:
        if field.name in type_hints:
            c_ty_str = field.ty.__str__()
            py_ty_str = type_hints.pop(field.name).__str__()
            if c_ty_str != py_ty_str and not (c_ty_str == "char*" and py_ty_str == "str"):
                warnings.warn(
                    f"Type mismatch on `{type_cls.__module__}.{type_cls.__qualname__}.{field.name}`: "
                    f"Expected `{c_ty_str}`, but got `{py_ty_str}`."
                )
                warned = True
        else:
            warnings.warn(
                f"Attribute not found: `{type_cls.__module__}.{type_cls.__qualname__}.{field.name}`. "
                f"Add `{field.name}: {field.ty}` to class definition."
            )
            warned = True
    if type_hints:
        extra_attrs = ", ".join(str(k) for k in type_hints)
        warnings.warn(f"Unused attributes in class definition: {extra_attrs}")
        warned = True
    method: TypeMethod
    for method in type_index2type_methods(type_info.type_index):
        if method.name.startswith("_"):
            continue
        func = getattr(type_cls, method.name, None)
        if func is None:
            warnings.warn(
                f"Method not found `{type_cls.__module__}.{type_cls.__qualname__}.{method.name}`. "
            )
            warned = True
        elif not callable(func):
            warnings.warn(
                f"Attribute `{type_cls.__module__}.{type_cls.__qualname__}.{method.name}` is not a method. "
            )
            warned = True
    if warned:
        warnings.warn(
            f"One or multiple warnings in `{type_cls.__module__}.{type_cls.__qualname__}`. Its prototype is:\n"
            + prototype_py(type_info)
        )
