from __future__ import annotations

from collections.abc import Callable
from typing import Any

from mlc._cython import Ptr, c_class_core, func_register, opaque_init, register_opauqe_type

from .object import Object


@c_class_core("mlc.core.Opaque")
class Opaque(Object):
    handle: Ptr

    def __init__(self, instance: Any) -> None:
        opaque_init(self, instance)

    @staticmethod
    def register(
        ty: type,
        eq_s: Callable | None = None,
        hash_s: Callable | None = None,
    ) -> None:
        register_opauqe_type(ty)
        name = ty.__module__ + "." + ty.__name__
        if eq_s is not None:
            assert callable(eq_s)
            func_register(f"Opaque.eq_s.{name}", False, eq_s)
        if hash_s is not None:
            assert callable(hash_s)
            func_register(f"Opaque.hash_s.{name}", False, hash_s)


def _default_serialize(opaques: list[Any]) -> str:
    import jsonpickle  # type: ignore[import-untyped]

    return jsonpickle.dumps(list(opaques))


def _default_deserialize(json_str: str) -> list[Any]:
    import jsonpickle  # type: ignore[import-untyped]

    return jsonpickle.loads(json_str)


func_register("mlc.Opaque.default.serialize", False, _default_serialize)
func_register("mlc.Opaque.default.deserialize", False, _default_deserialize)
