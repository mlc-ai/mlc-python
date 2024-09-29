from __future__ import annotations

from mlc._cython import PyAny, c_class


@c_class("object.Object")
class Object(PyAny):
    def __init__(self) -> None:
        self._mlc_init("__init__")


class PyObject(Object):
    _mlc_type_info = Object._mlc_type_info

    def __str__(self) -> str:
        return self.__repr__()
