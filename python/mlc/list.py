import ctypes
from collections.abc import Iterable
from typing import Any

from . import _cython as cy
from .object import Object


@cy.register_type("object.List")
class List(Object):
    capacity: int
    size: int
    data: ctypes.c_void_p

    def __init__(self, iterable: Iterable[Any] = ()) -> None:
        cy.list_dict_init(self, tuple(iterable))

    def __len__(self) -> int:
        return self.size

    def __getitem__(self, index: int) -> Any:
        length = len(self)
        if not -length <= index < length:
            raise IndexError(f"list index out of range: {index}")
        if index < 0:
            index += length
        return List._C("__iter_at__", self, index)

    def __iter__(self) -> Iterable[Any]:
        for i in range(len(self)):
            yield self[i]
