from __future__ import annotations

import ctypes
import dataclasses
import inspect
import typing
from collections.abc import Callable
from typing import Any, Literal, get_type_hints

from mlc._cython import MISSING, TypeField, TypeInfo
from mlc.core import typing as mlc_typing

KIND_MAP = {None: 0, "nobind": 1, "bind": 2, "var": 3}
SUB_STRUCTURES = {"nobind": 0, "bind": 1}


@dataclasses.dataclass
class Field:
    structure: Literal["bind", "nobind"] | None
    default_factory: Callable[[], Any]
    _name: str | None

    def __post_init__(self) -> None:
        if self.structure not in (None, "bind", "nobind"):
            raise ValueError(
                "Invalid `field.structure`. Expected `bind` or `nobind`, "
                f"but got: {self.structure}"
            )


class DefaultFactory(typing.NamedTuple):
    fn: Callable[[], typing.Any]


def method_init(
    type_cls: type,
    fields: list[Field],
) -> Callable[..., None]:
    annotations: dict[str, typing.Any] = {"return": None}
    params_without_defaults: list[inspect.Parameter] = []
    params_with_defaults: list[inspect.Parameter] = []
    ordering = [0] * len(fields)
    for i, field in enumerate(fields):
        assert field._name is not None
        name: str = field._name
        annotations[name] = typing.Any  # TODO: Handle annotations better
        if field.default_factory is MISSING:
            ordering[i] = len(params_without_defaults)
            params_without_defaults.append(
                inspect.Parameter(
                    name=name,
                    kind=inspect.Parameter.POSITIONAL_OR_KEYWORD,
                )
            )
        else:
            ordering[i] = -len(params_with_defaults) - 1
            params_with_defaults.append(
                inspect.Parameter(
                    name=name,
                    kind=inspect.Parameter.POSITIONAL_OR_KEYWORD,
                    default=DefaultFactory(fn=field.default_factory),
                )
            )
    for i, order in enumerate(ordering):
        if order < 0:
            ordering[i] = len(params_without_defaults) - order - 1
    sig = inspect.Signature(
        parameters=[
            *params_without_defaults,
            *params_with_defaults,
        ],
    )

    signature_str = (
        f"{type_cls.__module__}.{type_cls.__qualname__}.__init__("
        + ", ".join(p.name for p in sig.parameters.values())
        + ")"
    )

    def bind_args(*args: typing.Any, **kwargs: typing.Any) -> inspect.BoundArguments:
        bound = sig.bind(*args, **kwargs)
        bound.apply_defaults()
        return bound

    def method(self: type, *args: typing.Any, **kwargs: typing.Any) -> None:
        try:
            args = bind_args(*args, **kwargs).args
            args = tuple(arg.fn() if isinstance(arg, DefaultFactory) else arg for arg in args)
            args = tuple(args[order] for order in ordering)
            self._mlc_init(*args)  # type: ignore[attr-defined]
        except Exception as e:
            raise TypeError(f"Error in `{signature_str}`: {e}")  # type: ignore[attr-defined]
        try:
            post_init = self.__post_init__  # type: ignore[attr-defined]
        except AttributeError:
            pass
        else:
            post_init()

    method.__signature__ = sig  # type: ignore[attr-defined]
    method.__annotations__ = annotations
    return method


def field(
    *,
    default: Any = MISSING,
    default_factory: Any = MISSING,
    structure: Literal["bind", "nobind"] | None = "nobind",
) -> Any:
    if default is not MISSING and default_factory is not MISSING:
        raise ValueError("Cannot specify both `default` and `default_factory`")
    if default is not MISSING:
        default_factory = lambda: default
    return Field(
        structure=structure,
        default_factory=default_factory,
        _name=None,
    )


def inspect_dataclass_fields(
    type_key: str,
    type_cls: type,
    parent_type_info: TypeInfo,
) -> tuple[list[TypeField], list[Field]]:
    def _get_num_bytes(field_ty: Any) -> int:
        if hasattr(field_ty, "_ctype"):
            return ctypes.sizeof(field_ty._ctype())
        return 0

    # Step 1. Inspect and extract all the `TypeField`s
    type_hints = get_type_hints(type_cls)
    type_fields: list[TypeField] = []
    for type_field in parent_type_info.fields:
        field_name = type_field.name
        field_ty = type_field.ty
        if type_hints.pop(field_name, None) is None:
            raise ValueError(
                f"Missing field `{type_key}::{field_name}`, "
                f"which appears in its parent class `{parent_type_info.type_key}`."
            )
        type_fields.append(
            TypeField(
                name=field_name,
                offset=-1,
                num_bytes=_get_num_bytes(field_ty),
                frozen=False,
                ty=field_ty,
            )
        )
    for field_name, field_ty_py in type_hints.items():
        if field_name.startswith("_"):
            continue
        field_ty = mlc_typing.from_py(field_ty_py)
        type_fields.append(
            TypeField(
                name=field_name,
                offset=-1,
                num_bytes=_get_num_bytes(field_ty),
                frozen=False,
                ty=field_ty,
            )
        )
    # Step 2. Convert `TypeField`s to dataclass `Field`s
    d_fields: list[Field] = []
    for type_field in type_fields:
        lhs = type_field.name
        if lhs.startswith("_"):
            continue
        rhs = getattr(type_cls, lhs, MISSING)
        if isinstance(rhs, property):
            rhs = type_cls._mlc_dataclass_fields.get(lhs, MISSING)  # type: ignore[attr-defined]
        if rhs is MISSING:
            d_field = field()
        elif isinstance(rhs, (int, float, str, bool, type(None))):
            d_field = field(default=rhs)
        elif isinstance(rhs, Field):
            d_field = rhs
        else:
            raise ValueError(f"Cannot recognize field: {type_field.name}")
        d_field._name = type_field.name
        d_fields.append(d_field)
    return type_fields, d_fields


@dataclasses.dataclass
class Structure:
    kind: Literal["bind", "nobind", "var"] | None
    fields: tuple[str, ...] = dataclasses.field(default_factory=tuple)

    def _type_check(self) -> None:
        if not isinstance(self.kind, (str, type(None))):
            raise ValueError(
                f"Invalid `Structure.kind`. Expected `str`, but got: {type(self.kind)}"
            )
        if self.kind is None and self.fields:
            raise ValueError(
                "Invalid `Structure.fields`. Expected empty tuple, but got non-empty tuple"
            )
        if not isinstance(self.fields, tuple):
            raise ValueError(
                f"Invalid `Structure.fields`. Expected `tuple`, but got: {type(self.fields)}"
            )
        for field in self.fields:
            if not isinstance(field, str):
                raise ValueError(
                    f"Invalid element in `Structure.fields`. Expected `str`, but got: {type(field)}"
                )

    def __post_init__(self) -> None:
        self._type_check()
        fields = []
        if self.kind not in KIND_MAP:
            raise ValueError(
                f"Invalid `_mlc_structure`. Expected kind in `{KIND_MAP}`, but got `{self.kind}`"
            )
        for name_kind in self.fields:
            name, kind = _parse_name_kind(name_kind)
            fields.append(f"{name}:{kind}")
        self.fields = tuple(fields)


def _parse_name_kind(name_kind: str) -> tuple[str, str]:
    if ":" in name_kind:
        name, kind = name_kind.split(":")
    else:
        name, kind = name_kind, "nobind"
    if kind not in SUB_STRUCTURES:
        raise ValueError(
            f"Invalid `_mlc_structure`. Expected kind in `{SUB_STRUCTURES}`, but got `{name_kind}`"
        )
    return name, kind


def structure_to_c(
    struct: Structure,
    fields: list[TypeField],
) -> tuple[
    int,  # structure_kind
    list[int],  # sub_structure_indices
    list[int],  # sub_structure_kinds
]:
    field_to_indices = {field.name: i for i, field in enumerate(fields)}
    sub_indices: list[int] = []
    sub_kinds: list[int] = []
    for name, kind in map(_parse_name_kind, struct.fields):
        if name not in field_to_indices:
            raise ValueError(
                f"Invalid field name in `_mlc_structure`. Expected field name in `{field_to_indices}`, "
                f"but got: {name}"
            )
        sub_indices.append(field_to_indices[name])
        sub_kinds.append(SUB_STRUCTURES[kind])
    return KIND_MAP[struct.kind], sub_indices, sub_kinds


def structure_parse(
    structure: Literal["bind", "nobind", "var"] | None,
    d_fields: list[Field],
) -> Structure:
    assert structure in (None, "bind", "nobind", "var")
    if structure is None:
        return Structure(kind=None, fields=())
    struct_fields: list[str] = []
    for d_field in d_fields:
        sub_structure = d_field.structure
        assert sub_structure in (None, "nobind", "bind")
        if sub_structure is not None:
            struct_fields.append(f"{d_field._name}:{sub_structure}")
    return Structure(
        kind=structure,
        fields=tuple(struct_fields),
    )
