from __future__ import annotations

import copy
from collections.abc import Callable
from typing import Any, Literal

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
        eq_s: Callable | Literal["default"] | None = "default",
        hash_s: Callable | Literal["default"] | None = "default",
        deepcopy: Callable | Literal["default"] | None = "default",
    ) -> None:
        register_opauqe_type(ty)
        name = ty.__module__ + "." + ty.__name__

        if isinstance(eq_s, str) and eq_s == "default":
            func_register(f"Opaque.eq_s.{name}", False, lambda a, b: a == b)
        elif callable(eq_s):
            func_register(f"Opaque.eq_s.{name}", False, eq_s)
        else:
            assert eq_s is None, "eq_s must be a callable, a literal 'default', or None"

        if isinstance(hash_s, str) and hash_s == "default":
            func_register(f"Opaque.hash_s.{name}", False, lambda a: hash(a))
        elif callable(hash_s):
            func_register(f"Opaque.hash_s.{name}", False, hash_s)
        else:
            assert hash_s is None, "hash_s must be a callable, a literal 'default', or None"

        if isinstance(deepcopy, str) and deepcopy == "default":
            func_register(f"Opaque.deepcopy.{name}", False, lambda a: copy.deepcopy(a))
        elif callable(deepcopy):
            func_register(f"Opaque.deepcopy.{name}", False, deepcopy)
        else:
            assert deepcopy is None, "deepcopy must be a callable, a literal 'default', or None"


def _default_serialize(opaques: list[Any]) -> str:
    import jsonpickle  # type: ignore[import-untyped]

    return jsonpickle.dumps(list(opaques))


def _default_deserialize(json_str: str) -> list[Any]:
    import jsonpickle  # type: ignore[import-untyped]

    return jsonpickle.loads(json_str)


func_register("mlc.Opaque.default.serialize", False, _default_serialize)
func_register("mlc.Opaque.default.deserialize", False, _default_deserialize)
