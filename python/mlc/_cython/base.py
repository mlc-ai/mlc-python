from __future__ import annotations

import ctypes
import dataclasses
import platform
import sys
import threading
import types
import typing
from collections.abc import Callable
from pathlib import Path

if typing.TYPE_CHECKING:
    from mlc import Error

    from .core import PyAny  # type: ignore[import-not-found]


Ptr = ctypes.c_void_p


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


class MLCObjPtr(ctypes.Structure):
    _fields_ = [  # noqa: RUF012
        ("ptr", Ptr),
    ]


if sys.version_info >= (3, 10):
    DATACLASS_SLOTS = {"slots": True}
else:
    DATACLASS_SLOTS = {}


@dataclasses.dataclass(eq=False, **DATACLASS_SLOTS)
class TypeField:
    name: str
    offset: int
    num_bytes: int
    is_read_only: bool
    ty: PyAny  # actual type: mlc.typing.Type
    getter: Callable[[typing.Any], typing.Any] | None = None
    setter: Callable[[typing.Any, typing.Any], None] | None = None


@dataclasses.dataclass(eq=False, **DATACLASS_SLOTS)
class TypeMethod:
    name: str
    func: PyAny
    kind: int


@dataclasses.dataclass(eq=False, **DATACLASS_SLOTS)
class TypeInfo:
    type_index: int
    type_key: str
    type_depth: int
    type_ancestors: tuple[int, ...]
    fields: tuple[TypeField, ...]
    methods: tuple[TypeMethod, ...]

    def prototype(self) -> str:
        # io = StringIO()
        # print(f"class {self.type_key}:", file=io)
        # print(f"  # type_index: {self.type_index}; type_ancestors: {self.type_ancestors}", file=io)
        # for field in self.fields:
        #     print(f"  {field.name}: {field.type_ann}", file=io)
        # for fn in self.methods:
        #     if fn.kind == 0:
        #         print(f"  def {fn.name}(self, *args): ...", file=io)
        #     else:
        #         print("  @staticmethod", file=io)
        #         print(f"  def {fn.name}(*args): ...", file=io)
        # return io.getvalue().rstrip()
        raise NotImplementedError


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


class InternalError(Exception):
    pass


InternalError.__module__ = "mlc"


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


SYSTEM = platform.system()
DSO_SUFFIX = ".so" if SYSTEM == "Linux" else ".dylib" if SYSTEM == "Darwin" else ".dll"


def lib_path(core_path: str) -> tuple[Path, ctypes.CDLL]:
    path = Path(core_path).parent.parent / "lib" / f"libmlc_registry{DSO_SUFFIX}"
    if not path.exists():
        raise FileNotFoundError(f"Cannot find the MLC registry library at {path}")
    lib = ctypes.CDLL(str(path), ctypes.RTLD_GLOBAL)
    return path, lib
