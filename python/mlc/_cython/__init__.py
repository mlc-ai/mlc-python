from __future__ import annotations

import typing
from collections.abc import Callable

from .core import (  # type: ignore[import-not-found]
    # constants
    DSO_SUFFIX,
    LIB,
    LIB_PATH,
    SYSTEM,
    # MLCAny
    PyAny,
    Str,
    # Methods
    device_as_pair,
    dtype_as_triple,
    error_get_info,
    error_pycode_fake,
    func_call,
    func_get,
    func_init,
    func_register,
    str_c2py,
    str_py2c,
)
from .type_info import Ptr, c_class, py_class

if typing.TYPE_CHECKING:
    from mlc import Func


_CallableType = typing.TypeVar("_CallableType", bound=Callable)


def register_func(
    name: str,
    allow_override: bool = False,
) -> Callable[[_CallableType], _CallableType]:
    def decorator(func: _CallableType) -> _CallableType:
        func_register(name, allow_override, func)
        return func

    return decorator


def get_global_func(name: str, allow_missing: bool = False) -> Func:
    ret = func_get(name)
    if (not allow_missing) and (ret is None):
        raise ValueError(f"Can't find global function: {name}")
    return ret
