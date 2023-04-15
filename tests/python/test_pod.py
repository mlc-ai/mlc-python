import mlc
import pytest


def test_cxx_none() -> None:
    func = mlc.get_global_func("mlc.testing.cxx_none")
    assert func() is None


def test_cxx_nullptr() -> None:
    func = mlc.get_global_func("mlc.testing.cxx_null")
    assert func() is None


@pytest.mark.parametrize("x", [-1, 0, 1])
def test_cxx_int(x: int) -> None:
    func = mlc.get_global_func("mlc.testing.cxx_int")
    y = func(x)
    assert isinstance(y, int) and y == x


@pytest.mark.parametrize("x", [-1, 0, 1, 2.0, -2.0])
def test_cxx_float(x: float) -> None:
    func = mlc.get_global_func("mlc.testing.cxx_float")
    y = func(x)
    assert isinstance(y, float) and y == x


@pytest.mark.parametrize("x", [0x0, 0xDEADBEEF])
def test_cxx_ptr(x: int) -> None:
    import ctypes

    func = mlc.get_global_func("mlc.testing.cxx_ptr")
    y = func(ctypes.c_void_p(x))
    if x == 0:
        assert y is None
    else:
        assert isinstance(y, ctypes.c_void_p) and y.value == x
