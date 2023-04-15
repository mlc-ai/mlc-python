from __future__ import annotations

from . import _cython as cy


@cy.register_type("object.Object")
class Object(cy.core.PyAny):
    def __init__(self) -> None:
        self._mlc_init("__init__")
