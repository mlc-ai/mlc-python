from __future__ import annotations

import ctypes
import dataclasses
import enum
import platform
import sys
import threading
import types
import typing
from collections.abc import Callable
from io import StringIO
from pathlib import Path

import ml_dtypes
import numpy as np

if typing.TYPE_CHECKING:
    from mlc import DataType, Device, Error, Func
    from mlc.core import typing as mlc_typing

    from .core import PyAny  # type: ignore[import-not-found]


class InternalError(Exception):
    pass


InternalError.__module__ = "mlc"
Ptr = ctypes.c_void_p
SYSTEM: str = platform.system()
DSO_SUFFIX: str = ".so" if SYSTEM == "Linux" else ".dylib" if SYSTEM == "Darwin" else ".dll"
_DATACLASS_SLOTS = {"slots": True} if sys.version_info >= (3, 10) else {}
TLS = threading.local()
TLS.str2exc = {}
TLS.str2bytes = {}
ERR_KIND2CLS = {
    "AttributeError": AttributeError,
    "IndexError": IndexError,
    "KeyError": KeyError,
    "TypeError": TypeError,
    "ValueError": ValueError,
    "RuntimeError": RuntimeError,
    "NameError": NameError,
    "AssertionError": AssertionError,
    "ImportError": ImportError,
    "OSError": OSError,
    "SyntaxError": SyntaxError,
    "SystemError": SystemError,
    "NotImplementedError": NotImplementedError,
    "InternalError": InternalError,
}
DTYPE_PRESET: dict[typing.Any, typing.Any] = {
    np.dtype(np.bool_): "bool",
    np.dtype(np.int8): "int8",
    np.dtype(np.int16): "int16",
    np.dtype(np.int32): "int32",
    np.dtype(np.int64): "int64",
    np.dtype(np.uint8): "uint8",
    np.dtype(np.uint16): "uint16",
    np.dtype(np.uint32): "uint32",
    np.dtype(np.uint64): "uint64",
    np.dtype(np.float16): "float16",
    np.dtype(np.float32): "float32",
    np.dtype(np.float64): "float64",
    np.dtype(ml_dtypes.bfloat16): "bfloat16",
    np.dtype(ml_dtypes.float8_e4m3fn): "float8_e4m3fn",
    np.dtype(ml_dtypes.float8_e5m2): "float8_e5m2",
}


class DLDataType(ctypes.Structure):
    _fields_ = [  # noqa: RUF012
        ("code", ctypes.c_uint8),
        ("bits", ctypes.c_uint8),
        ("lanes", ctypes.c_uint16),
    ]


class DLDevice(ctypes.Structure):
    _fields_ = [  # noqa: RUF012
        ("device_type", ctypes.c_int),
        ("device_id", ctypes.c_int),
    ]


class MLCHeader(ctypes.Structure):
    _fields_ = [  # noqa: RUF012
        ("type_index", ctypes.c_int32),
        ("ref_cnt", ctypes.c_int32),
        ("deleter", Ptr),
    ]


class MLCAny(ctypes.Structure):
    _fields_ = [  # noqa: RUF012
        ("type_index", ctypes.c_int32),
        ("ref_cnt", ctypes.c_int32),
        ("deleter", Ptr),
    ]


class MLCObjPtr(ctypes.Structure):
    _fields_ = [  # noqa: RUF012
        ("ptr", Ptr),
    ]


class DeviceType(enum.Enum):
    cpu = 1
    cuda = 2
    cuda_host = 3
    opencl = 4
    vulkan = 7
    mps = 8
    vpi = 9
    rocm = 10
    rocm_host = 11
    ext_dev = 21
    cuda_managed = 13
    one_api = 14
    webgpu = 15
    hexagon = 16
    maia = 17


class DataTypeCode(enum.Enum):
    int = 0
    uint = 1
    float = 2
    handle = 3
    bfloat = 4
    complex = 5
    bool = 6
    float8_e4m3fn = 7
    float8_e5m2 = 8


@dataclasses.dataclass(eq=False, **_DATACLASS_SLOTS)
class TypeField:
    name: str
    offset: int
    num_bytes: int
    frozen: bool
    ty: PyAny | mlc_typing.Type
    getter: Callable[[typing.Any], typing.Any] | None = None
    setter: Callable[[typing.Any, typing.Any], None] | None = None


@dataclasses.dataclass(eq=False, **_DATACLASS_SLOTS)
class TypeMethod:
    name: str
    func: PyAny | Func
    kind: int  # 0: member method, 1: static method

    def as_callable(self) -> Callable[..., typing.Any]:
        from .core import func_call  # type: ignore[import-not-found]

        func = self.func
        if self.kind == 0:  # member method
            fn = lambda this, *args: func_call(func, (this, *args))
        elif self.kind == 1:
            fn = staticmethod(lambda *args: func_call(func, args))  # type: ignore[assignment]
        else:
            raise ValueError(f"Unknown method kind: {self.kind}")
        return fn


@dataclasses.dataclass(eq=False, **_DATACLASS_SLOTS)
class TypeInfo:
    type_cls: type | None
    type_index: int
    type_key: str
    type_depth: int
    type_ancestors: tuple[int, ...]
    fields: tuple[TypeField, ...]
    methods: tuple[TypeMethod, ...]
    methods_lookup: dict[str, TypeMethod] = dataclasses.field(default_factory=dict)

    def __post_init__(self) -> None:
        for method in self.methods:
            self.methods_lookup[method.name] = method.func

    def prototype(self) -> str:
        io = StringIO()
        print(f"class {self.type_key}:", file=io)
        print(f"  # type_index: {self.type_index}; type_ancestors: {self.type_ancestors}", file=io)
        for field in self.fields:
            ty = str(field.ty)
            if ty == "char*":
                ty = "str"
            print(f"  {field.name}: {ty}", file=io)
        for fn in self.methods:
            if fn.name.startswith("_"):
                continue
            if fn.kind == 0:
                print(f"  def {fn.name}(self, *args): ...", file=io)
            else:
                print("  @staticmethod", file=io)
                print(f"  def {fn.name}(*args): ...", file=io)
        return io.getvalue().rstrip()


def translate_exception_to_c(exception: Exception) -> tuple[bytes, int, bytes]:
    from .core import str_py2c  # type: ignore[import-not-found]

    def _kind() -> bytes:
        kind: str = exception.__class__.__name__
        if kind not in TLS.str2exc:
            TLS.str2exc[kind] = exception.__class__
            TLS.str2bytes[kind] = str_py2c(kind)
        return TLS.str2bytes[kind]

    def _bytes_info() -> bytes:
        bytes_info = list[bytes]()
        tb = exception.__traceback__
        while tb:
            code = tb.tb_frame.f_code
            bytes_info.append(str_py2c(code.co_name))
            bytes_info.append(str_py2c(str(tb.tb_lineno)))
            bytes_info.append(str_py2c(code.co_filename))
            tb = tb.tb_next
        bytes_info.append(str_py2c(str(exception)))
        bytes_info.reverse()
        bytes_info.append(b"")
        return b"\0".join(bytes_info)

    kind: bytes = _kind()
    bytes_info = _bytes_info()
    return kind, len(bytes_info), bytes_info


def translate_exception_from_c(err: Error) -> Exception:
    from .core import error_pycode_fake  # type: ignore[import-not-found]

    kind, info = err.kind, err._info
    msg, info = info[0], info[1:]
    if kind in ERR_KIND2CLS:
        exception = ERR_KIND2CLS[kind](msg)
    else:
        exception = RuntimeError(kind + ": " + msg)
    while info:
        filename, lineno, funcname, info = info[0], int(info[1]), info[2], info[3:]
        fake_code = error_pycode_fake(filename, funcname, lineno)
        try:
            exec(fake_code)
        except Exception as fake_exception:
            tb_new = fake_exception.__traceback__.tb_next  # type: ignore[union-attr]
        exception = exception.with_traceback(
            types.TracebackType(
                tb_next=exception.__traceback__,
                tb_frame=tb_new.tb_frame,  # type: ignore[union-attr]
                tb_lasti=tb_new.tb_lasti,  # type: ignore[union-attr]
                tb_lineno=lineno or 0,
            )
        )
    return exception


def lib_path(core_path: str) -> tuple[Path, ctypes.CDLL]:
    path = Path(core_path).parent.parent / "lib" / f"libmlc_registry{DSO_SUFFIX}"
    if not path.exists():
        raise FileNotFoundError(f"Cannot find the MLC registry library at {path}")
    lib = ctypes.CDLL(str(path), ctypes.RTLD_GLOBAL)
    return path, lib


def dtype_normalize(dtype: str | np.dtype | DataType) -> str | DataType:
    dtype = DTYPE_PRESET.get(dtype, dtype)
    if isinstance(dtype, np.dtype):
        dtype = str(dtype)
    return dtype


def device_normalize(device: str | Device) -> str | Device:
    return device


def new_object(cls):  # noqa: ANN001, ANN202
    """Helper function for pickle"""
    return cls.__new__(cls)


class MetaNoSlots(type):
    def __new__(
        cls,
        name: str,
        bases: tuple[type, ...],
        dict: dict[str, typing.Any],
    ) -> type:
        if dict.get("__slots__", ()):
            raise TypeError("Non-empty __slots__ not allowed in MLC dataclasses")
        dict["__slots__"] = ()
        return super().__new__(cls, name, bases, dict)
