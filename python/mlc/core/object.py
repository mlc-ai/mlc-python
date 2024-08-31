from __future__ import annotations

from mlc._cython import PyAny, register_type


@register_type("object.Object")
class Object(PyAny):
    def __init__(self) -> None:
        self._mlc_init("__init__")
