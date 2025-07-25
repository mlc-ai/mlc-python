from __future__ import annotations

import typing
from collections.abc import Callable

from mlc._cython import PyAny, TypeInfo, c_class_core

try:
    from warnings import deprecated  # type: ignore[attr-defined]
except ImportError:
    from typing_extensions import deprecated


@c_class_core("object.Object")
class Object(PyAny):
    def __init__(self, *args: typing.Any, **kwargs: typing.Any) -> None:
        def init() -> None:
            self._mlc_init()

        init(*args, **kwargs)

    @property
    def id_(self) -> int:
        return self._mlc_address

    def is_(self, other: Object) -> bool:
        return isinstance(other, Object) and self._mlc_address == other._mlc_address

    def __copy__(self: Object) -> Object:
        return PyAny._mlc_copy_shallow(self)  # type: ignore[attr-defined]

    def __deepcopy__(self: Object, memo: dict[int, Object] | None) -> Object:
        return PyAny._mlc_copy_deep(self)

    def __replace__(self: Object, /, **changes: typing.Any) -> Object:
        unpacked: list[typing.Any] = [self]
        for key, value in changes.items():
            unpacked.append(key)
            unpacked.append(value)
        return PyAny._mlc_copy_replace(*unpacked)

    def __hash__(self) -> int:
        return hash((type(self), self._mlc_address))

    def __eq__(self, other: typing.Any) -> bool:
        return eq_ptr(self, other)

    def __ne__(self, other: typing.Any) -> bool:
        return not self == other

    def _mlc_setattr(self, name: str, value: typing.Any) -> None:
        type_info: TypeInfo = type(self)._mlc_type_info
        for field in type_info.fields:
            if field.name == name:
                if field.setter is None:
                    raise AttributeError(f"Attribute `{name}` missing setter")
                field.setter(self, value)
                return
        raise AttributeError(f"Attribute `{name}` not found in `{type(self)}`")

    def _mlc_getattr(self, name: str) -> typing.Any:
        type_info: TypeInfo = type(self)._mlc_type_info
        for field in type_info.fields:
            if field.name == name:
                if field.getter is None:
                    raise AttributeError(f"Attribute `{name}` missing getter")
                return field.getter(self)
        raise AttributeError(f"Attribute `{name}` not found in `{type(self)}`")

    def swap(self, other: typing.Any) -> None:
        if type(self) == type(other):
            self._mlc_swap(other)
        else:
            raise TypeError(f"Cannot different types: `{type(self)}` and `{type(other)}`")

    @deprecated(
        "Method `.json` is deprecated. Use `mlc.json_dumps` instead.",
        stacklevel=2,
    )
    def json(
        self,
        fn_opaque_serialize: Callable[[list[typing.Any]], str] | None = None,
    ) -> str:
        return json_dumps(self, fn_opaque_serialize)

    @staticmethod
    @deprecated(
        "Method `.from_json` is deprecated. Use `mlc.json_loads` instead.",
        stacklevel=2,
    )
    def from_json(
        json_str: str,
        fn_opaque_deserialize: Callable[[str], list[typing.Any]] | None = None,
    ) -> Object:
        return json_loads(json_str, fn_opaque_deserialize)

    @deprecated(
        "Method `.eq_s` is deprecated. Use `mlc.eq_s` instead.",
        stacklevel=2,
    )
    def eq_s(
        self,
        other: Object,
        *,
        bind_free_vars: bool = True,
        assert_mode: bool = False,
    ) -> bool:
        return eq_s(self, other, bind_free_vars=bind_free_vars, assert_mode=assert_mode)

    @deprecated(
        "Method `.eq_s_fail_reason` is deprecated. Use `mlc.eq_s_fail_reason` instead.",
        stacklevel=2,
    )
    def eq_s_fail_reason(
        self,
        other: Object,
        *,
        bind_free_vars: bool = True,
    ) -> tuple[bool, str]:
        return eq_s_fail_reason(self, other, bind_free_vars=bind_free_vars)

    @deprecated(
        "Method `.hash_s` is deprecated. Use `mlc.hash_s` instead.",
        stacklevel=2,
    )
    def hash_s(self) -> int:
        return hash_s(self)

    @deprecated(
        "Method `.eq_ptr` is deprecated. Use `mlc.eq_ptr` instead.",
        stacklevel=2,
    )
    def eq_ptr(self, other: typing.Any) -> bool:
        return eq_ptr(self, other)


def json_dumps(
    object: typing.Any,
    fn_opaque_serialize: Callable[[list[typing.Any]], str] | None = None,
) -> str:
    assert isinstance(object, Object), f"Expected `mlc.Object`, got `{type(object)}`"
    return object._mlc_json(fn_opaque_serialize)  # type: ignore[attr-defined]


def json_loads(
    json_str: str,
    fn_opaque_deserialize: Callable[[str], list[typing.Any]] | None = None,
) -> Object:
    return PyAny._mlc_from_json(json_str, fn_opaque_deserialize)  # type: ignore[attr-defined]


def eq_s(
    lhs: typing.Any,
    rhs: typing.Any,
    *,
    bind_free_vars: bool = True,
    assert_mode: bool = False,
) -> bool:
    assert isinstance(lhs, Object), f"Expected `mlc.Object`, got `{type(lhs)}`"
    assert isinstance(rhs, Object), f"Expected `mlc.Object`, got `{type(rhs)}`"
    return PyAny._mlc_eq_s(lhs, rhs, bind_free_vars, assert_mode)  # type: ignore[attr-defined]


def eq_s_fail_reason(
    lhs: typing.Any,
    rhs: typing.Any,
    *,
    bind_free_vars: bool = True,
) -> tuple[bool, str]:
    assert isinstance(lhs, Object), f"Expected `mlc.Object`, got `{type(lhs)}`"
    assert isinstance(rhs, Object), f"Expected `mlc.Object`, got `{type(rhs)}`"
    return PyAny._mlc_eq_s_fail_reason(lhs, rhs, bind_free_vars)


def hash_s(obj: typing.Any) -> int:
    assert isinstance(obj, Object), f"Expected `mlc.Object`, got `{type(obj)}`"
    return PyAny._mlc_hash_s(obj)  # type: ignore[attr-defined]


def eq_ptr(lhs: typing.Any, rhs: typing.Any) -> bool:
    assert isinstance(lhs, Object), f"Expected `mlc.Object`, got `{type(lhs)}`"
    assert isinstance(rhs, Object), f"Expected `mlc.Object`, got `{type(rhs)}`"
    return lhs._mlc_address == rhs._mlc_address
