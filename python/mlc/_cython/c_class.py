from __future__ import annotations

import functools
import typing
from collections.abc import Callable

from .base import TypeInfo

ClsType = typing.TypeVar("ClsType")


def c_class(type_key: str) -> Callable[[type[ClsType]], type[ClsType]]:
    from . import core

    def check_cls(type_cls: type, type_info: TypeInfo) -> None:
        # TODO: add this back
        # cls_fields = typing.get_type_hints(cls)
        for field in type_info.fields:
            # str_field = str(field.type_ann)
            # if field.name in cls_fields:
            #     type_ann = core.TypeAnn(cls_fields.pop(field.name))
            #     str_cls_field = str(type_ann)
            #     if str_cls_field != str_field:
            #         warnings.warn(
            #             f"In `{cls.__qualname__}`, attribute `{field.name}` type mismatch. "
            #             f"Expected `{str_field}`, but it is defined as `{str_cls_field}`."
            #         )
            # else:
            #     warnings.warn(
            #         f"In `{cls.__qualname__}`, attribute `{field.name}` not found. "
            #         f"Add `{field.name}: {str_field}` to class definition."
            #     )
            ...

    def decorator(super_type_cls: type[ClsType]) -> type[ClsType]:
        @functools.wraps(super_type_cls, updated=())
        class type_cls(super_type_cls):  # type: ignore[valid-type,misc]
            __slots__ = ()
            _mlc_type_info: TypeInfo | None = None

        type_cls._mlc_type_info = core.type_key2type_info(type_key, type_cls)
        type_info = type_cls._mlc_type_info

        check_cls(super_type_cls, type_info)

        for method in type_info.methods:
            if not method.name.startswith("_"):
                func = method.func
                func_call = core.func_call
                if method.kind == 0:  # member method
                    fn = lambda this, *args: func_call(func, (this, *args))
                elif method.kind == 1:
                    fn = staticmethod(lambda *args: func_call(func, args))
                else:
                    raise ValueError(f"Unknown method kind: {method.kind}")
                _attach_method(type_cls, method.name, fn)

        for field in type_info.fields:
            name = field.name
            setattr(
                type_cls,
                name,
                property(
                    fget=_make_fget(name, field.getter),
                    fset=_make_fset(name, field.setter),
                    doc=f"{type_key}::{name}",
                ),
            )
        return type_cls

    return decorator


def _make_fget(
    name: str,
    getter: typing.Callable[[typing.Any], typing.Any] | None,
) -> typing.Callable[[typing.Any], typing.Any]:
    def fget(this: typing.Any, _name: str = name) -> typing.Any:
        return getter(this)

    return fget if getter is not None else None


def _make_fset(
    name: str,
    setter: typing.Callable[[typing.Any, typing.Any], None] | None,
) -> typing.Callable[[typing.Any, typing.Any], None]:
    def fset(this: typing.Any, value: typing.Any, _name: str = name) -> None:
        setter(this, value)

    return fset if setter is not None else None


def _attach_method(
    cls: ClsType,
    name: str,
    method: typing.Callable,
    annotations: dict[str, type] | None = None,
) -> None:
    method.__module__ = cls.__module__
    method.__name__ = name
    method.__qualname__ = f"{cls.__qualname__}.{name}"  # type: ignore[attr-defined]
    method.__doc__ = f"Method `{name}` of class `{cls.__qualname__}`"  # type: ignore[attr-defined]
    method.__annotations__ = annotations or {}
    setattr(cls, name, method)


def py_class(type_key: str) -> Callable[[type[ClsType]], type[ClsType]]:
    def decorator(cls: type[ClsType]) -> type[ClsType]:
        return cls

    return decorator
