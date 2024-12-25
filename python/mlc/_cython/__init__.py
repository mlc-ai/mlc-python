import ctypes as _ctypes
import pathlib as _pathlib

from . import core as _core  # type: ignore[import-not-found]
from .base import (
    DSO_SUFFIX,
    MISSING,
    SYSTEM,
    DataTypeCode,
    DeviceType,
    DLDataType,
    DLDevice,
    Field,
    MetaNoSlots,
    MLCAny,
    MLCHeader,
    MLCObjPtr,
    Ptr,
    TypeField,
    TypeInfo,
    TypeMethod,
    attach_field,
    attach_method,
    c_class_core,
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
    make_mlc_init,
    str_c2py,
    str_py2c,
    type_add_method,
    type_cast,
    type_create,
    type_create_instance,
    type_field_get_accessor,
    type_index2cached_py_type_info,
    type_index2type_methods,
    type_key2py_type_info,
    type_register_fields,
    type_register_structure,
)

LIB: _ctypes.CDLL = _core.LIB
LIB_PATH: _pathlib.Path = _core.LIB_PATH
Str: type[str] = _core.Str


class PyAny(_core.PyAny, metaclass=MetaNoSlots):
    __slots__ = ()
