from __future__ import annotations

from abc import ABCMeta
from collections.abc import Iterable, Iterator, Sequence
from typing import Any, TypeVar, overload

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
    def __getitem__(self, i: int) -> T: ...

    @overload
    def __getitem__(self, i: slice) -> Sequence[T]: ...

    def __getitem__(self, i: int | slice) -> T | Sequence[T]:
        if isinstance(i, int):
            i = _normalize_index(i, len(self))
            return List._C(b"__iter_at__", self, i)
        elif isinstance(i, slice):
            # Implement slicing
            start, stop, step = i.indices(len(self))
            return List([self[i] for i in range(start, stop, step)])
        else:
            raise TypeError(f"list indices must be integers or slices, not {type(i).__name__}")

    def __iter__(self) -> Iterator[T]:
        return iter(self[i] for i in range(len(self)))

    def insert(self, i: int, x: T) -> None:
        i = _normalize_index(i, len(self) + 1)
        return List._C(b"_insert", self, i, x)

    def append(self, x: T) -> None:
        return List._C(b"_append", self, x)

    def pop(self, i: int = -1) -> T:
        i = _normalize_index(i, len(self))
        return List._C(b"_pop", self, i)

    def clear(self) -> None:
        return List._C(b"_clear", self)

    def extend(self, iterable: Iterable[T]) -> None:
        return List._C(b"_extend", self, *iterable)

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, List) and self._mlc_address == other._mlc_address:
            return True
        if not isinstance(other, (list, tuple, List)):
            return False
        if len(self) != len(other):
            return False
        return all(a == b for a, b in zip(self, other))

    def __ne__(self, other: Any) -> bool:
        return not (self == other)


def _normalize_index(i: int, length: int) -> int:
    if not -length <= i < length:
        raise IndexError(f"list index out of range: {i}")
    if i < 0:
        i += length
    return i
