from __future__ import annotations

from abc import ABCMeta
from collections.abc import Iterable, Iterator, Sequence
from typing import TypeVar, overload

from mlc._cython import MetaNoSlots, Ptr, c_class_core

from .object import Object

T = TypeVar("T")


class ListMeta(MetaNoSlots, ABCMeta): ...


@c_class_core("object.List")
class List(Object, Sequence[T], metaclass=ListMeta):
    capacity: int
    size: int
    data: Ptr

    def __init__(self, iterable: Iterable[T] = ()) -> None:
        self._mlc_init(*iterable)

    def __len__(self) -> int:
        return self.size

    @overload
    def __getitem__(self, index: int) -> T: ...

    @overload
    def __getitem__(self, index: slice) -> Sequence[T]: ...

    def __getitem__(self, index: int | slice) -> T | Sequence[T]:
        if isinstance(index, int):
            length = len(self)
            if not -length <= index < length:
                raise IndexError(f"list index out of range: {index}")
            if index < 0:
                index += length
            return List._C(b"__iter_at__", self, index)
        elif isinstance(index, slice):
            # Implement slicing
            start, stop, step = index.indices(len(self))
            return List([self[i] for i in range(start, stop, step)])
        else:
            raise TypeError(f"list indices must be integers or slices, not {type(index).__name__}")

    def __iter__(self) -> Iterator[T]:
        return iter(self[i] for i in range(len(self)))
