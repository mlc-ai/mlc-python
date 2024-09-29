from collections.abc import Callable
from typing import Any

from mlc._cython import c_class, func_call, func_init, get_global_func

from .object import Object


@c_class("object.Func")
class Func(Object):
    def __init__(self, func: Callable) -> None:
        assert callable(func), "func must be callable"
        func_init(self, func)

    def __call__(self, *args: Any) -> Any:
        return func_call(self, args)

    @staticmethod
    def get(name: str, allow_missing: bool = False) -> "Func":
        return get_global_func(name, allow_missing)
