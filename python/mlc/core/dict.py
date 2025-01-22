from __future__ import annotations

import itertools
from abc import ABCMeta
from collections.abc import ItemsView, Iterable, Iterator, KeysView, Mapping, ValuesView
from typing import Any, TypeVar, overload

from mlc._cython import MetaNoSlots, Ptr, c_class_core

from .object import Object

K = TypeVar("K")
V = TypeVar("V")
T = TypeVar("T")


class _UnspecifiedType: ...


Unspecified = _UnspecifiedType()


class DictMeta(MetaNoSlots, ABCMeta): ...


@c_class_core("object.Dict")
class Dict(Object, Mapping[K, V], metaclass=DictMeta):
    capacity: int
    size: int
    data: Ptr

    def __init__(
        self,
        _source: Iterable[tuple[K, V]] | Mapping[K, V] | None = None,
        **kwargs: V,
    ) -> None:
        if isinstance(_source, Mapping):
            py_args = tuple(_source.items())
        elif isinstance(_source, Iterable):
            py_args = tuple(_source)  # type: ignore[arg-type]
        elif _source is None:
            py_args = ()
        else:
            raise TypeError(f"expected mapping or iterable, got {type(_source).__name__}")
        if kwargs:
            py_args += tuple(kwargs.items())
        self._mlc_init(*itertools.chain.from_iterable(py_args))

    def __len__(self) -> int:
        return self.size

    def __iter__(self) -> Iterator[K]:
        return self._keys_iterator()

    def _keys_iterator(self) -> Iterator[K]:
        cap = self.capacity
        i = -1
        while (i := Dict._C(b"__iter_advance__", self, i)) < cap:
            yield Dict._C(b"__iter_get_key__", self, i)

    def _values_iterator(self) -> Iterator[V]:
        cap = self.capacity
        i = -1
        while (i := Dict._C(b"__iter_advance__", self, i)) < cap:
            yield Dict._C(b"__iter_get_value__", self, i)

    def _items_iterator(self) -> Iterator[tuple[K, V]]:
        cap = self.capacity
        i = -1
        while (i := Dict._C(b"__iter_advance__", self, i)) < cap:
            yield Dict._C(b"__iter_get_key__", self, i), Dict._C(b"__iter_get_value__", self, i)

    def keys(self) -> KeysView[K]:
        return _DictKeysView(self)

    def values(self) -> ValuesView[V]:
        return _DictValuesView(self)

    def items(self) -> ItemsView[K, V]:
        return _DictItemsView(self)

    def __getitem__(self, key: K) -> V:
        return Dict._C(b"__getitem__", self, key)

    def __setitem__(self, key: K, value: V) -> None:
        Dict._C(b"__setitem__", self, key, value)

    def __delitem__(self, key: K) -> None:
        Dict._C(b"__delitem__", self, key)

    def pop(self, key: K, default: T | _UnspecifiedType = Unspecified) -> V | T:
        try:
            return Dict._C(b"__delitem__", self, key)
        except KeyError:
            if default is Unspecified:
                raise
            assert not isinstance(default, _UnspecifiedType)
            return default

    def setdefault(self, key: K, default: V | None = None) -> V | None:
        try:
            return self[key]
        except KeyError:
            self[key] = default  # type: ignore[assignment]
            return default

    # Additional methods required by the Mapping ABC
    def __contains__(self, key: object) -> bool:
        try:
            self[key]  # type: ignore
            return True
        except KeyError:
            return False

    @overload
    def get(self, key: K) -> V | None: ...

    @overload
    def get(self, key: K, default: V | T) -> V | T: ...

    def get(self, key: K, default: V | T | None = None) -> V | T | None:
        try:
            return self[key]
        except KeyError:
            return default

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, Dict) and self._mlc_address == other._mlc_address:
            return True
        if not isinstance(other, (Dict, dict)) or len(self) != len(other):
            return False
        return all(v == other.get(k, Unspecified) for k, v in self.items())

    def __ne__(self, other: Any) -> bool:
        return not (self == other)


class _DictKeysView(KeysView[K]):
    def __init__(self, mapping: Dict[K, V]) -> None:
        self._mapping = mapping

    def __len__(self) -> int:
        return len(self._mapping)

    def __iter__(self) -> Iterator[K]:
        return self._mapping._keys_iterator()

    def __contains__(self, key: object) -> bool:
        return key in self._mapping


class _DictValuesView(ValuesView[V]):
    def __init__(self, mapping: Dict[K, V]) -> None:
        self._mapping = mapping

    def __len__(self) -> int:
        return len(self._mapping)

    def __iter__(self) -> Iterator[V]:
        return self._mapping._values_iterator()

    def __contains__(self, value: object) -> bool:
        return any(v == value for v in self)


class _DictItemsView(ItemsView[K, V]):
    def __init__(self, mapping: Dict[K, V]) -> None:
        self._mapping = mapping

    def __len__(self) -> int:
        return len(self._mapping)

    def __iter__(self) -> Iterator[tuple[K, V]]:
        return self._mapping._items_iterator()

    def __contains__(self, item: object) -> bool:
        if not isinstance(item, tuple) or len(item) != 2:
            return False
        key, value = item
        try:
            v = self._mapping[key]
        except KeyError:
            return False
        else:
            return v == value
