from __future__ import annotations

try:
    from typing import dataclass_transform
except ImportError:
    from typing_extensions import dataclass_transform

import typing
from collections.abc import Callable

from mlc._cython import (
    TypeField,
    TypeInfo,
    make_mlc_init,
    type_add_method,
    type_create,
    type_create_instance,
    type_register_fields,
    type_register_structure,
)

from . import utils

InputClsType = typing.TypeVar("InputClsType")


@dataclass_transform(field_specifiers=(utils.field, utils.Field))
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

        # Step 1. Create `type_info`
        parent_type_info: TypeInfo = utils.get_parent_type(super_type_cls)._mlc_type_info  # type: ignore[attr-defined]
        type_info: TypeInfo = type_create(parent_type_info.type_index, type_key)
        type_index = type_info.type_index

        # Step 2. Reflect all the fields of the type
        fields, d_fields, num_bytes = utils.inspect_dataclass_fields(
            super_type_cls,
            parent_type_info,
            frozen=frozen,
            py_mode=True,
        )
        type_info.fields = tuple(fields)
        type_info.d_fields = tuple(d_fields)
        type_register_fields(type_index, fields)

        # Step 3. Register the structure of the class
        struct: utils.Structure
        struct_kind: int
        sub_structure_indices: list[int]
        sub_structure_kinds: list[int]
        if (struct := vars(super_type_cls).get("_mlc_structure", None)) is not None:
            assert isinstance(struct, utils.Structure)
        else:
            struct = utils.structure_parse(structure, d_fields)
        (
            struct_kind,
            sub_structure_indices,
            sub_structure_kinds,
        ) = utils.structure_to_c(struct, fields)
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

        # Step 4. Attach methods
        # Step 4.1. Method `__init__`
        fn_init: Callable[..., None] | None = None
        if init:
            fn_init = utils.method_init(super_type_cls, d_fields)
        else:
            fn_init = None
        # Step 4.2. Method `__repr__` and `__str__`
        fn_repr: Callable[[InputClsType], str] | None = None
        if repr:
            fn_repr = _method_repr(type_key, fields)
            type_add_method(type_index, "__str__", fn_repr, 1)  # static
        elif (fn_repr := vars(super_type_cls).get("__str__", None)) is not None:
            assert callable(fn_repr)
            type_add_method(type_index, "__str__", fn_repr, 1)
        else:
            fn_repr = None

        # Step 5. Create the proxy class with the fields as properties
        type_cls: type[InputClsType] = utils.create_type_class(
            cls=super_type_cls,
            type_info=type_info,
            methods={
                "_mlc_init": make_mlc_init(fields),
                "__new__": lambda cls, *args, **kwargs: type_create_instance(
                    cls, type_index, num_bytes
                ),
                "__init__": fn_init,
                "__repr__": fn_repr,
                "__str__": fn_repr,
            },
        )
        type_add_method(type_index, "__init__", _method_new(type_cls), 1)  # static
        setattr(type_cls, "_mlc_structure", struct)
        return type_cls

    return decorator


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
