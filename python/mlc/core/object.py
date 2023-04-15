from __future__ import annotations

from mlc._cython import PyAny
from mlc.dataclasses.c_class import c_class


@c_class("object.Object")
class Object(PyAny):
    def json(self) -> str:
        return super()._mlc_json()

    @staticmethod
    def from_json(json_str: str) -> Object:
        return PyAny._mlc_from_json(json_str)  # type: ignore[attr-defined]

    def eq_s(
        self,
        other: Object,
        *,
        bind_free_vars: bool = True,
        assert_mode: bool = False,
    ) -> bool:
        return PyAny._mlc_eq_s(self, other, bind_free_vars, assert_mode)  # type: ignore[attr-defined]

    def hash_s(self) -> int:
        return PyAny._mlc_hash_s(self)  # type: ignore[attr-defined]
