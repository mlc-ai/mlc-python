import traceback
from io import StringIO

import mlc


def test_throw_exception_from_c() -> None:
    func = mlc.Func.get("mlc.testing.throw_exception_from_c")
    try:
        func()
    except ValueError:
        msg = traceback.format_exc().strip().splitlines()
        assert "Traceback (most recent call last)" in msg[0]
        assert "in test_throw_exception_from_c" in msg[1]
        assert "ValueError: This is an error message" in msg[-1]
        if mlc._cython.SYSTEM != "Darwin":
            # FIXME: for some reason, `c_api_tests.cc` is not in the traceback on macOS
            # TODO: fix macOS libbacktrace integration on macOS
            assert "c_api_tests.cc" in msg[-3]


def test_throw_exception_from_ffi() -> None:
    def throw_ValueError() -> None:
        raise ValueError("This is a ValueError")

    err: mlc.Error = mlc.Func.get("mlc.testing.throw_exception_from_ffi")(throw_ValueError)
    assert err.kind == "ValueError"
    assert str(err) == "This is a ValueError"
    io = StringIO()
    err.print_exc(file=io)
    msg = io.getvalue().rstrip().splitlines()
    assert "Traceback (most recent call last)" in msg[0]
    assert "ValueError: This is a ValueError" in msg[-1]
    assert msg[1].endswith(", in throw_ValueError")


def test_throw_exception_from_ffi_in_c() -> None:
    def throw_ValueError() -> None:
        def _inner() -> None:
            raise ValueError("This is a ValueError")

        _inner()

    try:
        mlc.Func.get("mlc.testing.throw_exception_from_ffi_in_c")(throw_ValueError)
    except ValueError:
        msg = traceback.format_exc().strip().splitlines()
        assert "Traceback (most recent call last)" in msg[0]
        assert "in test_throw_exception_from_ffi_in_c" in msg[1]
        assert "ValueError: This is a ValueError" in msg[-1]
        if mlc._cython.SYSTEM != "Darwin":
            # FIXME: for some reason, `c_api_tests.cc` is not in the traceback on macOS
            # TODO: fix macOS libbacktrace integration on macOS
            idx_c_api_tests = next(i for i, line in enumerate(msg) if "c_api_tests.cc" in line)
            idx_handle_error = next(
                i for i, line in enumerate(msg) if "_func_safe_call_impl" in line
            )
            assert idx_c_api_tests < idx_handle_error
