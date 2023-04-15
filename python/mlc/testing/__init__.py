from __future__ import annotations

from mlc import Object, register_type


@register_type("mlc.testing.ReflectionTestObj")
class ReflectionTestObj(Object):
    x_mutable: str
    y_immutable: int

    def __init__(self, x_mutable: str, y_immutable: int) -> None:
        self._mlc_init("__init__", x_mutable, y_immutable)

    def YPlusOne(self) -> int:
        raise NotImplementedError
