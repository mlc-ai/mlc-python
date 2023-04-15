import functools
import typing
from collections.abc import Callable

from . import core as _core

if typing.TYPE_CHECKING:
    from mlc import Func

CallableType = typing.TypeVar("CallableType", bound=Callable)
ClsType = typing.TypeVar("ClsType", bound=type)


def register_func(
    name: str, allow_override: bool = False
) -> Callable[[CallableType], CallableType]:
    def decorator(func: CallableType) -> CallableType:
        _core.register_func(name, allow_override, func)
        return func

    return decorator


def get_global_func(name: str, allow_missing: bool = False) -> "Func":
    ret = _core.get_global_func(name)
    if (not allow_missing) and (ret is None):
        raise ValueError(f"Can't find global function: {name}")
    return ret


def register_type(type_key: str) -> Callable[[type[ClsType]], type[ClsType]]:
    def decorator(cls: type[ClsType]) -> type[ClsType]:
        class WrappedClass(cls):  # type: ignore[valid-type,misc]
            __slots__ = ()

        functools.update_wrapper(WrappedClass, cls, updated=())
        type_index: int
        attrs: dict[str, typing.Any] = {}
        vtable: dict[str, typing.Any] = {}
        type_index = _core.type_reflect(cls, type_key, attrs, vtable)
        for key, value in attrs.items():
            if callable(value):

                @functools.wraps(value)
                def wrapped(
                    self: typing.Any,
                    *args: typing.Any,
                    **kwargs: typing.Any,
                ) -> typing.Any:
                    return value(self, *args, **kwargs)

                setattr(WrappedClass, key, wrapped)
            else:
                setattr(WrappedClass, key, value)
        setattr(WrappedClass, "_mlc_vtable", vtable)
        _core.register_type(WrappedClass, type_index, type_key)
        return WrappedClass

    return decorator
