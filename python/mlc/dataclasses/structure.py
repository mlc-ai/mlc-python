from __future__ import annotations

import dataclasses
from typing import Literal

from mlc._cython import TypeField

from .field import Field

KIND_MAP = {None: 0, "nobind": 1, "bind": 2, "var": 3}
SUB_STRUCTURES = {"nobind": 0, "bind": 1}


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
