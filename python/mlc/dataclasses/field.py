from __future__ import annotations

import dataclasses
from typing import Any, Literal

from mlc._cython import TypeField


@dataclasses.dataclass
class Field:
    structure: Literal["bind", "nobind"] | None
    _name: str | None

    def __post_init__(self) -> None:
        if self.structure not in (None, "bind", "nobind"):
            raise ValueError(
                f"Invalid `field.structure`. Expected `bind` or `nobind`, but got: {self.structure}"
            )


def field(
    *,
    structure: Literal["bind", "nobind"] | None = "nobind",
) -> Any:
    return Field(structure=structure, _name=None)


def get_dataclass_fields(type_cls: type, type_fields: list[TypeField]) -> list[Field]:
    fields = []
    for type_field in type_fields:
        if type_field.name.startswith("_"):
            continue
        attr = getattr(type_cls, type_field.name, None)
        if isinstance(attr, property):
            attr = type_cls._mlc_dataclass_fields.get(  # type: ignore[attr-defined]
                type_field.name,
                None,
            )
        if attr is None:
            d_field = field()
        elif isinstance(attr, Field):
            d_field = attr
        else:
            raise ValueError(f"Cannot recognize field: {type_field.name}")
        d_field._name = type_field.name
        fields.append(d_field)
    return fields
