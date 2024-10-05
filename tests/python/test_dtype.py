import mlc
import pytest
from mlc import DataType


@pytest.mark.parametrize("x", ["int32", "int32x3", "float8"])
def test_dtype_init_str(x: str) -> None:
    func = mlc.get_global_func("mlc.testing.cxx_dtype")
    y = func(DataType(x))
    z = func(x)
    assert isinstance(y, DataType) and y == DataType(x) and str(y) == x
    assert isinstance(z, DataType) and z == DataType(x) and str(z) == x


@pytest.mark.parametrize("x", ["int", "int32xx3", "float821"])
def test_dtype_init_fail(x: str) -> None:
    func = mlc.get_global_func("mlc.testing.cxx_dtype")
    try:
        print(func(x))
    except ValueError as e:
        assert str(e) == f"Cannot convert to `dtype` from string: {x}"
    else:
        assert False
