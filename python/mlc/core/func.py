from __future__ import annotations

from collections.abc import Callable
from typing import Any, TypeVar

from mlc._cython import c_class_core, func_call, func_get, func_init, func_register

from .object import Object

_CallableType = TypeVar("_CallableType", bound=Callable)


@c_class_core("object.Func")
class Func(Object):
    def __init__(self, func: Callable) -> None:
        assert callable(func), "func must be callable"
        func_init(self, func)

    def __call__(self, *args: Any) -> Any:
        return func_call(self, args)

    @staticmethod
    def get(name: str, allow_missing: bool = False) -> Func:
        ret = func_get(name)
        if (not allow_missing) and (ret is None):
            raise ValueError(f"Can't find global function: {name}")
        return ret

    @staticmethod
    def register(
        name: str,
        allow_override: bool = False,
    ) -> Callable[[_CallableType], _CallableType]:
        def decorator(func: _CallableType) -> _CallableType:
            func_register(name, allow_override, func)
            return func

        return decorator


def json_loads(s: str) -> Any:
    return _json_loads(s)


def build_info() -> dict[str, Any]:
    return _build_info()


_json_loads = Func.get("mlc.core.JSONLoads")
_build_info = Func.get("mlc.core.BuildInfo")
