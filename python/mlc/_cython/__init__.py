from __future__ import annotations

import ctypes
import pathlib
import typing
from collections.abc import Callable

from . import core as _core  # type: ignore[import-not-found]
from .base import DSO_SUFFIX, SYSTEM, Ptr
from .c_class import c_class, py_class
from .core import (  # type: ignore[import-not-found]
    PyAny,
    Str,
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

if typing.TYPE_CHECKING:
    from mlc import Func


LIB: ctypes.CDLL = _core.LIB
LIB_PATH: pathlib.Path = _core.LIB_PATH
