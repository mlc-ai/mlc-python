from __future__ import annotations

from typing import Any

from mlc._cython import Ptr, c_class_core, opaque_init, register_opauqe_type

from .object import Object


@c_class_core("mlc.core.Opaque")
class Opaque(Object):
    handle: Ptr

    def __init__(self, instance: Any) -> None:
        opaque_init(self, instance)

    @staticmethod
    def register(ty: type) -> None:
        register_opauqe_type(ty)
