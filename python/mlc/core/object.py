from __future__ import annotations

from mlc._cython import PyAny
from mlc.dataclasses import c_class


@c_class("object.Object")
class Object(PyAny):
    def json(self) -> str:
        return super()._mlc_json()

    @staticmethod
    def from_json(json_str: str) -> Object:
        return PyAny._mlc_from_json(json_str)  # type: ignore[attr-defined]


class PyClass(Object):
    _mlc_type_info = Object._mlc_type_info

    def __str__(self) -> str:
        return self.__repr__()
