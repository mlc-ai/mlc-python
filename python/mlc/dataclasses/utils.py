from __future__ import annotations

import typing


def attach_field(
    cls: type,
    name: str,
    getter: typing.Callable[[typing.Any], typing.Any] | None,
    setter: typing.Callable[[typing.Any, typing.Any], None] | None,
    frozen: bool,
) -> None:
    def fget(this: typing.Any, _name: str = name) -> typing.Any:
        return getter(this)  # type: ignore[misc]

    def fset(this: typing.Any, value: typing.Any, _name: str = name) -> None:
        setter(this, value)  # type: ignore[misc]

    prop = property(
        fget=fget if getter else None,
        fset=fset if (not frozen) and setter else None,
        doc=f"{cls.__module__}.{cls.__qualname__}.{name}",
    )
    setattr(cls, name, prop)


def attach_method(
    cls: type,
    name: str,
    method: typing.Callable,
) -> None:
    method.__module__ = cls.__module__
    method.__name__ = name
    method.__qualname__ = f"{cls.__qualname__}.{name}"  # type: ignore[attr-defined]
    method.__doc__ = f"Method `{name}` of class `{cls.__qualname__}`"  # type: ignore[attr-defined]
    setattr(cls, name, method)
