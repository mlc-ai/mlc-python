import ml_dtypes
import mlc
import numpy as np
import pytest
import torch
from mlc import DataType


@pytest.mark.parametrize("x", ["int32", "int32x3", "float8"])
def test_dtype_init_str(x: str) -> None:
    func = mlc.Func.get("mlc.testing.cxx_dtype")
    y = func(DataType(x))
    z = func(x)
    assert isinstance(y, DataType) and y == DataType(x) and str(y) == x
    assert isinstance(z, DataType) and z == DataType(x) and str(z) == x


@pytest.mark.parametrize("x", ["int", "int32xx3", "float821"])
def test_dtype_init_fail(x: str) -> None:
    func = mlc.Func.get("mlc.testing.cxx_dtype")
    try:
        print(func(x))
    except ValueError as e:
        assert str(e) == f"Cannot convert to `dtype` from string: {x}"
    else:
        assert False


@pytest.mark.parametrize("x", [DataType("int32"), DataType("int32x3"), DataType("float8")])
def test_dtype_init_dtype(x: DataType) -> None:
    y = DataType(x)
    assert x == y


@pytest.mark.parametrize(
    "x, y",
    [
        (torch.int32, "int32"),
        (torch.float16, "float16"),
        (torch.float8_e4m3fn, "float8_e4m3fn"),
        (torch.float8_e4m3fnuz, "float8_e4m3fnuz"),
        (torch.float8_e5m2, "float8_e5m2"),
        (torch.float8_e5m2fnuz, "float8_e5m2fnuz"),
    ],
)
def test_dtype_init_torch(x: str, y: DataType) -> None:
    assert y == str(DataType(x))


@pytest.mark.parametrize(
    "x, y",
    [
        (ml_dtypes.int2, "int2"),
        (ml_dtypes.int4, "int4"),
        (ml_dtypes.uint2, "uint2"),
        (ml_dtypes.uint4, "uint4"),
        (ml_dtypes.float8_e3m4, "float8_e3m4"),
        (ml_dtypes.float8_e4m3, "float8_e4m3"),
        (ml_dtypes.float8_e4m3b11fnuz, "float8_e4m3b11fnuz"),
        (ml_dtypes.float8_e4m3fn, "float8_e4m3fn"),
        (ml_dtypes.float8_e4m3fnuz, "float8_e4m3fnuz"),
        (ml_dtypes.float8_e5m2, "float8_e5m2"),
        (ml_dtypes.float8_e5m2fnuz, "float8_e5m2fnuz"),
        (ml_dtypes.float8_e8m0fnu, "float8_e8m0fnu"),
        (ml_dtypes.float4_e2m1fn, "float4_e2m1fn"),
        (ml_dtypes.float6_e2m3fn, "float6_e2m3fn"),
        (ml_dtypes.float6_e3m2fn, "float6_e3m2fn"),
    ],
)
def test_dtype_init_ml_dtypes(x: type, y: str) -> None:
    assert y == str(DataType(x))


@pytest.mark.parametrize(
    "x, y",
    [
        ("int32", torch.int32),
        ("float16", torch.float16),
        ("float8_e4m3fn", torch.float8_e4m3fn),
        ("float8_e4m3fnuz", torch.float8_e4m3fnuz),
        ("float8_e5m2", torch.float8_e5m2),
        ("float8_e5m2fnuz", torch.float8_e5m2fnuz),
    ],
)
def test_dtype_to_torch(x: str, y: torch.dtype) -> None:
    assert DataType(x).torch() == y


@pytest.mark.parametrize(
    "x, y",
    [
        ("int2", ml_dtypes.int2),
        ("int4", ml_dtypes.int4),
        ("uint2", ml_dtypes.uint2),
        ("uint4", ml_dtypes.uint4),
        ("int32", np.int32),
        ("float16", np.float16),
        ("float8_e3m4", ml_dtypes.float8_e3m4),
        ("float8_e4m3", ml_dtypes.float8_e4m3),
        ("float8_e4m3b11fnuz", ml_dtypes.float8_e4m3b11fnuz),
        ("float8_e4m3fn", ml_dtypes.float8_e4m3fn),
        ("float8_e4m3fnuz", ml_dtypes.float8_e4m3fnuz),
        ("float8_e5m2", ml_dtypes.float8_e5m2),
        ("float8_e5m2fnuz", ml_dtypes.float8_e5m2fnuz),
        ("float8_e8m0fnu", ml_dtypes.float8_e8m0fnu),
        ("float4_e2m1fn", ml_dtypes.float4_e2m1fn),
        ("float6_e2m3fn", ml_dtypes.float6_e2m3fn),
        ("float6_e3m2fn", ml_dtypes.float6_e3m2fn),
    ],
)
def test_dtype_to_numpy(x: str, y: type) -> None:
    assert DataType(x).numpy() == np.dtype(y)


def test_dtype_register() -> None:
    code = DataType.register("float8_custom", bits=8)
    dtype = DataType("float8_custom")
    assert dtype.code == code and dtype.bits == 8 and dtype.lanes == 1
    assert str(dtype) == "float8_custom"
