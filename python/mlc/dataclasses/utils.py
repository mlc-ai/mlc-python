from __future__ import annotations

import inspect
import typing
from collections.abc import Callable

from mlc._cython import TypeField


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
    old_field = getattr(cls, name, None)
    setattr(cls, name, prop)
    cls._mlc_dataclass_fields[name] = old_field  # type: ignore[attr-defined]


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


def method_init(
    type_cls: type,
    type_hints: dict[str, type],
    fields: typing.Sequence[TypeField],
) -> Callable[..., None]:
    sig = inspect.Signature(
        parameters=[
            inspect.Parameter(
                name=field.name,
                kind=inspect.Parameter.POSITIONAL_OR_KEYWORD,
            )
            for field in fields
        ],
    )

    def bind_args(*args: typing.Any, **kwargs: typing.Any) -> inspect.BoundArguments:
        try:
            bound = sig.bind(*args, **kwargs)
            bound.apply_defaults()
        except TypeError as e:
            name = f"{type_cls.__module__}.{type_cls.__qualname__}"
            raise TypeError(f"Error in `{name}.__init__`: {e}")  # type: ignore[attr-defined]
        return bound

    def method(self: type, *args: typing.Any, **kwargs: typing.Any) -> None:
        args = bind_args(*args, **kwargs).args
        self._mlc_init(*args)  # type: ignore[attr-defined]

    method.__annotations__ = dict(type_hints) | {"return": None}
    return method
