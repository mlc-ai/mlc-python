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


@dataclasses.dataclass
class Field:
    structure: typing.Literal["bind", "nobind"] | None
    default_factory: Callable[[], typing.Any]
    name: str | None

    def __post_init__(self) -> None:
        if self.structure not in (None, "bind", "nobind"):
            raise ValueError(
                "Invalid `field.structure`. Expected `bind` or `nobind`, "
                f"but got: {self.structure}"
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
