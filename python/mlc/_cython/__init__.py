from __future__ import annotations

import ctypes
import pathlib

from . import core as _core  # type: ignore[import-not-found]
from .base import (
    DSO_SUFFIX,
    SYSTEM,
    DataTypeCode,
    DeviceType,
    DLDataType,
    DLDevice,
    MetaNoSlots,
    MLCAny,
    MLCHeader,
    MLCObjPtr,
    Ptr,
    TypeField,
    TypeInfo,
    TypeMethod,
    device_normalize,
    dtype_normalize,
)
from .core import (  # type: ignore[import-not-found]
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
    type_cast,
    type_create,
    type_field_get_accessor,
    type_key2py_type_info,
)

LIB: ctypes.CDLL = _core.LIB
LIB_PATH: pathlib.Path = _core.LIB_PATH
PyAny: type = _core.PyAnyNoSlots
Str: type = _core.Str
