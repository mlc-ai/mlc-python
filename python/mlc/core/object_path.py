from __future__ import annotations

from typing import Any

from mlc._cython import c_class_core

from .object import Object


@c_class_core("mlc.core.ObjectPath")
class ObjectPath(Object):
    kind: int
    key: Any
    prev: Object | None
    length: int

    @staticmethod
    def root() -> ObjectPath:
        return ObjectPath._C(b"root")

    def with_field(self, field: str) -> ObjectPath:
        return ObjectPath._C(b"with_field", self, field)

    def with_list_index(self, index: int) -> ObjectPath:
        return ObjectPath._C(b"with_list_index", self, index)

    def with_dict_key(self, key: Any) -> ObjectPath:
        return ObjectPath._C(b"with_dict_key", self, key)

    def equal(self, other: ObjectPath) -> bool:
        return ObjectPath._C(b"equal", self, other)

    def get_prefix(self, length: int) -> ObjectPath:
        return ObjectPath._C(b"get_prefix", self, length)

    def is_prefix_of(self, other: ObjectPath) -> bool:
        return ObjectPath._C(b"is_prefix_of", self, other)

    def __getitem__(self, key: str | int) -> ObjectPath:
        if isinstance(key, int):
            return self.with_list_index(key)
        elif isinstance(key, str):
            return self.with_field(key)
        else:
            raise TypeError(
                f"Unsupported key type: {type(key)}. Please explicitly use "
                "`with_field`, `with_list_index`, or `with_dict_key` methods."
            )
