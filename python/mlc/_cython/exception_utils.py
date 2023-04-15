import threading
import types
import typing

if typing.TYPE_CHECKING:
    from mlc import Error


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


def translate_exception_from_c(err: "Error") -> Exception:
    from .core import pycode_fake

    kind, info = err.kind, err._info
    msg, info = info[0], info[1:]
    if kind in ERR_KIND2CLS:
        exception = ERR_KIND2CLS[kind](msg)
    else:
        exception = RuntimeError(kind + ": " + msg)
    while info:
        filename, lineno, funcname, info = info[0], int(info[1]), info[2], info[3:]
        fake_code = pycode_fake(filename, funcname, lineno)
        try:
            exec(fake_code)
        except Exception as err:
            tb_new = err.__traceback__.tb_next  # type: ignore[union-attr]
        exception = exception.with_traceback(
            types.TracebackType(
                tb_next=exception.__traceback__,
                tb_frame=tb_new.tb_frame,  # type: ignore[union-attr]
                tb_lasti=tb_new.tb_lasti,  # type: ignore[union-attr]
                tb_lineno=lineno or 0,
            )
        )
    return exception
