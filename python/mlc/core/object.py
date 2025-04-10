from __future__ import annotations

import typing

from mlc._cython import PyAny, c_class_core


@c_class_core("object.Object")
class Object(PyAny):
    def __init__(self, *args: typing.Any, **kwargs: typing.Any) -> None:
        def init() -> None:
            self._mlc_init()

        init(*args, **kwargs)

    def json(self) -> str:
        return super()._mlc_json()

    @staticmethod
    def from_json(json_str: str) -> Object:
        return PyAny._mlc_from_json(json_str)  # type: ignore[attr-defined]

    def eq_s(
        self,
        other: Object,
        *,
        bind_free_vars: bool = True,
        assert_mode: bool = False,
    ) -> bool:
        return PyAny._mlc_eq_s(self, other, bind_free_vars, assert_mode)  # type: ignore[attr-defined]

    def eq_s_fail_reason(
        self,
        other: Object,
        *,
        bind_free_vars: bool = True,
    ) -> tuple[bool, str]:
        return PyAny._mlc_eq_s_fail_reason(self, other, bind_free_vars)

    def hash_s(self) -> int:
        return PyAny._mlc_hash_s(self)  # type: ignore[attr-defined]

    def eq_ptr(self, other: typing.Any) -> bool:
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
        return self.eq_ptr(other)

    def __ne__(self, other: typing.Any) -> bool:
        return not self == other

    def swap(self, other: typing.Any) -> None:
        if type(self) == type(other):
            self._mlc_swap(other)
        else:
            raise TypeError(f"Cannot different types: `{type(self)}` and `{type(other)}`")
