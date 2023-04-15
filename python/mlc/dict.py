from __future__ import annotations

import ctypes
import itertools
from collections.abc import Iterable, Mapping
from typing import Any

from . import _cython as cy
from .object import Object


@cy.register_type("object.Dict")
class Dict(Object):
    capacity: int
    size: int
    data: ctypes.c_void_p

    def __init__(
        self,
        _source: Iterable[tuple[Any, Any]] | Mapping[Any, Any] | None = None,
        **kwargs: Any,
    ) -> None:
        if isinstance(_source, Mapping):
            py_args = tuple(_source.items())
        elif isinstance(_source, Iterable):
            py_args = tuple(_source)
        elif _source is None:
            py_args = ()
        else:
            raise TypeError(f"expected mapping or iterable, got {type(_source).__name__}")
        if kwargs:
            py_args += tuple(kwargs.items())
        py_args = tuple(itertools.chain.from_iterable(py_args))
        cy.list_dict_init(self, py_args)

    def __len__(self) -> int:
        return self.size

    def __iter__(self) -> Iterable[Any]:
        return self.keys()

    def keys(self) -> Iterable[Any]:
        cap = self.capacity
        i = -1
        while (i := Dict._C("__iter_advance__", self, i)) < cap:
            yield Dict._C("__iter_get_key__", self, i)

    def values(self) -> Iterable[Any]:
        cap = self.capacity
        i = -1
        while (i := Dict._C("__iter_advance__", self, i)) < cap:
            yield Dict._C("__iter_get_value__", self, i)

    def items(self) -> Iterable[tuple[Any, Any]]:
        cap = self.capacity
        i = -1
        while (i := Dict._C("__iter_advance__", self, i)) < cap:
            yield Dict._C("__iter_get_key__", self, i), Dict._C("__iter_get_value__", self, i)

    def __getitem__(self, key: Any) -> Any:
        return Dict._C("__getitem__", self, key)
