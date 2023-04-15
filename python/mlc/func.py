from collections.abc import Callable
from typing import Any

from . import _cython as cy
from .object import Object


@cy.register_type("object.Func")
class Func(Object):
    def __init__(self, func: Callable) -> None:
        assert callable(func), "func must be callable"
        cy.func_init(self, func)

    def __call__(self, *args: Any) -> Any:
        return cy.func_call(self, args)

    @staticmethod
    def get(name: str, allow_missing: bool = False) -> "Func":
        return cy.get_global_func(name, allow_missing)
