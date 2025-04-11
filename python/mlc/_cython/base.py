from __future__ import annotations

import ctypes
import dataclasses
import enum
import functools
import platform
import sys
import threading
import types
import typing
from collections.abc import Callable
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
ClsType = typing.TypeVar("ClsType")
MISSING = type("MISSING", (), {})()
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
DTYPE_PRESET: dict[typing.Any, str] = {
    np.dtype(np.bool_): "bool",
    np.dtype(ml_dtypes.int2): "int2",
    np.dtype(ml_dtypes.int4): "int4",
    np.dtype(np.int8): "int8",
    np.dtype(np.int16): "int16",
    np.dtype(np.int32): "int32",
    np.dtype(np.int64): "int64",
    np.dtype(ml_dtypes.uint2): "uint2",
    np.dtype(ml_dtypes.uint4): "uint4",
    np.dtype(np.uint8): "uint8",
    np.dtype(np.uint16): "uint16",
    np.dtype(np.uint32): "uint32",
    np.dtype(np.uint64): "uint64",
    np.dtype(np.float16): "float16",
    np.dtype(np.float32): "float32",
    np.dtype(np.float64): "float64",
    # bfloat16
    np.dtype(ml_dtypes.bfloat16): "bfloat16",
    # 8-bit floating point representations
    np.dtype(ml_dtypes.float8_e3m4): "float8_e3m4",
    np.dtype(ml_dtypes.float8_e4m3): "float8_e4m3",
    np.dtype(ml_dtypes.float8_e4m3b11fnuz): "float8_e4m3b11fnuz",
    np.dtype(ml_dtypes.float8_e4m3fn): "float8_e4m3fn",
    np.dtype(ml_dtypes.float8_e4m3fnuz): "float8_e4m3fnuz",
    np.dtype(ml_dtypes.float8_e5m2): "float8_e5m2",
    np.dtype(ml_dtypes.float8_e5m2fnuz): "float8_e5m2fnuz",
    np.dtype(ml_dtypes.float8_e8m0fnu): "float8_e8m0fnu",
    # Microscaling (MX) sub-byte floating point representations
    np.dtype(ml_dtypes.float4_e2m1fn): "float4_e2m1fn",  # higher 4 bits are unused
    np.dtype(ml_dtypes.float6_e2m3fn): "float6_e2m3fn",  # higher 2 bits are unused
    np.dtype(ml_dtypes.float6_e3m2fn): "float6_e3m2fn",  # higher 2 bits are unused
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
    # 8-bit floating point representations
    float8_e3m4 = 7
    float8_e4m3 = 8
    float8_e4m3b11fnuz = 9
    float8_e4m3fn = 10
    float8_e4m3fnuz = 11
    float8_e5m2 = 12
    float8_e5m2fnuz = 13
    float8_e8m0fnu = 14
    # Microscaling (MX) sub-byte floating point representations
    float4_e2m1fn = 15  # higher 4 bits are unused
    float6_e2m3fn = 16  # higher 2 bits are unused
    float6_e3m2fn = 17  # higher 2 bits are unused


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


@dataclasses.dataclass
class Field:
    structure: typing.Literal["bind", "nobind"] | None
    default_factory: Callable[[], typing.Any]
    name: str | None

    def __post_init__(self) -> None:
        if self.structure not in (None, "bind", "nobind"):
            raise ValueError(
                f"Invalid `field.structure`. Expected `bind` or `nobind`, but got: {self.structure}"
            )


@dataclasses.dataclass(eq=False, **_DATACLASS_SLOTS)
class TypeInfo:
    type_cls: type | None
    type_index: int
    type_key: str
    type_depth: int
    type_ancestors: tuple[int, ...]
    fields: tuple[TypeField, ...]
    d_fields: tuple[Field, ...] = ()

    def get_parent(self) -> TypeInfo:
        from .core import type_index2cached_py_type_info  # type: ignore[import-not-found]

        type_index = self.type_ancestors[-1]
        return type_index2cached_py_type_info(type_index)


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
        bytes_info = [b if b else b"<null>" for b in bytes_info]
        bytes_info.reverse()
        bytes_info.append(b"")
        return b"\0".join(bytes_info)

    kind: bytes = _kind()
    bytes_info = _bytes_info()
    return kind, len(bytes_info), bytes_info


def translate_exception_from_c(err: Error) -> Exception:
    from .core import error_pycode_fake  # type: ignore[import-not-found]

    kind, info = err.kind, err._info
    if info:
        msg, info = info[0], info[1:]
    else:
        msg = ""
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
    base = Path(core_path).parent.parent
    for path in [
        # only one candidate for now
        base / "lib" / "mlc" / f"libmlc{DSO_SUFFIX}",
    ]:
        if path.exists():
            break
    else:
        raise FileNotFoundError(f"Cannot find the MLC library at {path}")
    lib = ctypes.CDLL(str(path), ctypes.RTLD_GLOBAL)
    return path, lib


def dtype_normalize(dtype: typing.Any) -> str | DataType:
    if isinstance(dtype, str):
        return dtype

    def _torch_dtype_to_str() -> str | None:
        if "torch" not in sys.modules:
            return None
        import torch

        if not isinstance(dtype, torch.dtype):
            return None

        return str(dtype)[6:]  # remove `torch.`

    def _numpy_dtype_to_str() -> str | None:
        try:
            np_dtype = np.dtype(dtype)
        except:
            return None
        return DTYPE_PRESET.get(np_dtype, None) or str(dtype)

    if (np_dtype := _numpy_dtype_to_str()) is not None:
        return np_dtype
    if (torch_dtype := _torch_dtype_to_str()) is not None:
        return torch_dtype
    from mlc.core.dtype import DataType

    return dtype if isinstance(dtype, DataType) else str(dtype)


def device_normalize(device: typing.Any) -> str | Device:
    def _torch_device_to_str() -> str | None:
        if "torch" not in sys.modules:
            return None
        import torch

        if not isinstance(device, torch.device):
            return None
        index = device.index or 0

        return f"{device.type}:{index}"

    if (torch_device := _torch_device_to_str()) is not None:
        return torch_device
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
    parent_cls: type,
    cls: type,
    name: str,
    method: typing.Callable,
    check_exists: bool,
) -> None:
    if check_exists:
        if name in vars(parent_cls):
            msg = f"Cannot add `{name}` method in `{parent_cls}` because it's already defined"
            translation = {
                "__init__": "init",
                "__str__": "repr",
                "__repr__": "repr",
            }
            if name in translation:
                msg += f". Use `{translation[name]}=False` in dataclass declaration"
            raise ValueError(msg)
    method.__module__ = cls.__module__
    method.__name__ = name
    method.__qualname__ = f"{cls.__qualname__}.{name}"  # type: ignore[attr-defined]
    method.__doc__ = f"Method `{name}` of class `{cls.__qualname__}`"  # type: ignore[attr-defined]
    setattr(cls, name, method)


def c_class_core(type_key: str) -> Callable[[type[ClsType]], type[ClsType]]:
    def decorator(super_type_cls: type[ClsType]) -> type[ClsType]:
        from .core import (  # type: ignore[import-not-found]
            type_index2type_methods,
            type_key2py_type_info,
        )

        @functools.wraps(super_type_cls, updated=())
        class type_cls(super_type_cls):  # type: ignore[valid-type,misc]
            __slots__ = ()

        # Step 1. Retrieve `type_info` and set `_mlc_type_info`
        type_info: TypeInfo = type_key2py_type_info(type_key)
        if type_info.type_cls is not None:
            raise ValueError(f"Type is already registered: {type_key}")
        type_info.type_cls = type_cls
        type_info.d_fields = tuple(
            Field(
                structure=None,
                default_factory=MISSING,
                name=f.name,
            )
            for f in type_info.fields
        )
        # TODO: support `d_fields` for `c_class_core`

        # Step 2. Attach fields
        setattr(type_cls, "_mlc_type_info", type_info)
        for field in type_info.fields:
            attach_field(
                cls=type_cls,
                name=field.name,
                getter=field.getter,
                setter=field.setter,
                frozen=field.frozen,
            )

        # Step 4. Attach methods
        method: TypeMethod
        for method in type_index2type_methods(type_info.type_index):
            if not method.name.startswith("_"):
                attach_method(
                    parent_cls=super_type_cls,
                    cls=type_cls,
                    name=method.name,
                    method=method.as_callable(),
                    check_exists=False,
                )
        return type_cls

    return decorator
