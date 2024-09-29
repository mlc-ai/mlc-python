import typing
from collections.abc import Callable

if typing.TYPE_CHECKING:
    from mlc import Func


CallableType = typing.TypeVar("CallableType", bound=Callable)


def register_func(
    name: str, allow_override: bool = False
) -> Callable[[CallableType], CallableType]:
    from . import core

    def decorator(func: CallableType) -> CallableType:
        core.register_func(name, allow_override, func)
        return func

    return decorator


def get_global_func(name: str, allow_missing: bool = False) -> "Func":
    from . import core

    ret = core.get_global_func(name)
    if (not allow_missing) and (ret is None):
        raise ValueError(f"Can't find global function: {name}")
    return ret
