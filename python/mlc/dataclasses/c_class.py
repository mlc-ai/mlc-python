import typing
import warnings
from collections.abc import Callable

try:
    from typing import dataclass_transform
except ImportError:
    from typing_extensions import dataclass_transform

from mlc._cython import (
    TypeInfo,
    TypeMethod,
    type_index2type_methods,
    type_key2py_type_info,
)
from mlc.core import typing as mlc_typing

from . import utils

InputClsType = typing.TypeVar("InputClsType")


@dataclass_transform(field_specifiers=(utils.field, utils.Field))
def c_class(
    type_key: str,
    init: bool = True,
) -> Callable[[type[InputClsType]], type[InputClsType]]:
    def decorator(super_type_cls: type[InputClsType]) -> type[InputClsType]:
        # Step 1. Retrieve `type_info` from registry
        parent_type_info: TypeInfo = utils.get_parent_type(super_type_cls)._mlc_type_info  # type: ignore[attr-defined]
        type_info: TypeInfo = type_key2py_type_info(type_key)
        if type_info.type_cls is not None:
            raise ValueError(f"Type is already registered: {type_key}")

        # Step 2. Reflect all the fields of the type
        _, d_fields, _ = utils.inspect_dataclass_fields(
            super_type_cls,
            parent_type_info,
            frozen=False,
        )
        type_info.d_fields = tuple(d_fields)
        # Check if all fields are exposed as type annotations
        _check_c_class(super_type_cls, type_info)

        # Step 4. Attach methods
        fn_init: Callable[..., None] | None = None
        if init:
            fn_init = utils.method_init(super_type_cls, d_fields)
        else:
            fn_init = None
        # Step 5. Create the proxy class with the fields as properties
        type_cls: type[InputClsType] = utils.create_type_class(
            cls=super_type_cls,
            type_info=type_info,
            methods={
                "__init__": fn_init,
            },
        )
        return type_cls

    return decorator


def _check_c_class(
    type_cls: type[InputClsType],
    type_info: TypeInfo,
) -> None:
    type_hints = typing.get_type_hints(type_cls)
    warned: bool = False
    for field in type_info.fields:
        if field.name in type_hints:
            c_ty_str = field.ty.__str__()
            py_ty = type_hints.pop(field.name)
            py_ty_str = mlc_typing.from_py(py_ty).__str__()
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
            + utils.prototype(type_info, lang="py")
        )
