from typing import Any

import mlc
import numpy as np
import pytest


@mlc.dataclasses.c_class("mlc.testing.ReflectionTestObj")
class ReflectionTestObj(mlc.Object):
    x_mutable: str
    y_immutable: int

    def __init__(self, x_mutable: str, y_immutable: int) -> None:
        self._mlc_init("__init__", x_mutable, y_immutable)

    def YPlusOne(self) -> int:
        raise NotImplementedError


@mlc.dataclasses.c_class("mlc.testing.c_class")
class CClassForTest(mlc.Object):
    i8: int
    i16: int
    i32: int
    i64: int
    f32: float
    f64: float
    raw_ptr: mlc.Ptr
    dtype: mlc.DataType
    device: mlc.Device
    any: Any
    func: mlc.Func
    ulist: list[Any]
    udict: dict
    str_: str

    def __init__(  # noqa: PLR0913
        self,
        i8: int,
        i16: int,
        i32: int,
        i64: int,
        f32: float,
        f64: float,
        raw_ptr: mlc.Ptr,
        dtype: mlc.DataType,
        device: mlc.Device,
        any: Any,
        func: mlc.Func,
        ulist: list[Any],
        udict: dict,
        str_: str,
    ) -> None:
        self._mlc_init(
            "__init__",
            i8,
            i16,
            i32,
            i64,
            f32,
            f64,
            raw_ptr,
            dtype,
            device,
            any,
            func,
            ulist,
            udict,
            str_,
        )


def test_c_class_mutable() -> None:
    obj = ReflectionTestObj("hello", 42)
    assert obj.x_mutable == "hello"
    assert obj.y_immutable == 42
    obj.x_mutable = "world"
    assert obj.x_mutable == "world"


def test_c_class_immutable() -> None:
    obj = ReflectionTestObj("hello", 42)
    assert obj.y_immutable == 42
    try:
        obj.y_immutable = 43
        assert False
    except AttributeError:
        pass


def test_c_class_method() -> None:
    obj = ReflectionTestObj("hello", 42)
    assert obj.YPlusOne() == 43


@pytest.fixture
def c_class_for_test() -> CClassForTest:
    def _plus_one(x: int) -> int:
        return x + 1

    kwargs = {
        "i8": 8,
        "i16": 16,
        "i32": 32,
        "i64": 64,
        "f32": 1.5,
        "f64": 2.5,
        "raw_ptr": mlc.Ptr(0xDEADBEEF),
        "dtype": "float8",
        "device": "cuda:0",
        "any": "hello",
        "func": _plus_one,
        "ulist": [1, 2.0, "three", lambda: 4],
        "udict": {"1": 1, "2": 2.0, "3": "three", "4": lambda: 4},
        "str_": "world",
    }
    return CClassForTest(**kwargs)  # type: ignore[arg-type]


def test_c_class_i8(c_class_for_test: CClassForTest) -> None:
    obj = c_class_for_test
    assert obj.i8 == 8
    obj.i8 *= -2
    assert obj.i8 == -16


def test_c_class_i16(c_class_for_test: CClassForTest) -> None:
    obj = c_class_for_test
    assert obj.i16 == 16
    obj.i16 *= -2
    assert obj.i16 == -32


def test_c_class_i32(c_class_for_test: CClassForTest) -> None:
    obj = c_class_for_test
    assert obj.i32 == 32
    obj.i32 *= -2
    assert obj.i32 == -64


def test_c_class_i64(c_class_for_test: CClassForTest) -> None:
    obj = c_class_for_test
    assert obj.i64 == 64
    obj.i64 *= -2


def test_c_class_f32(c_class_for_test: CClassForTest) -> None:
    obj = c_class_for_test
    assert np.isclose(obj.f32, 1.5)
    obj.f32 *= 2
    assert np.isclose(obj.f32, 3.0)


def test_c_class_f64(c_class_for_test: CClassForTest) -> None:
    obj = c_class_for_test
    assert np.isclose(obj.f64, 2.5)
    obj.f64 *= 2
    assert np.isclose(obj.f64, 5.0)


def test_c_class_raw_ptr(c_class_for_test: CClassForTest) -> None:
    obj = c_class_for_test
    assert obj.raw_ptr.value == 0xDEADBEEF
    obj.raw_ptr = mlc.Ptr(0xDEAD1234)
    assert obj.raw_ptr.value == 0xDEAD1234


def test_c_class_dtype(c_class_for_test: CClassForTest) -> None:
    obj = c_class_for_test
    assert obj.dtype == mlc.DataType("float8")
    obj.dtype = "int32"  # type: ignore[assignment]
    assert obj.dtype == mlc.DataType("int32")


def test_c_class_deivce(c_class_for_test: CClassForTest) -> None:
    obj = c_class_for_test
    assert obj.device == mlc.Device("cuda:0")
    obj.device = "cpu:0"  # type: ignore[assignment]
    assert obj.device == mlc.Device("cpu:0")


def test_c_class_any(c_class_for_test: CClassForTest) -> None:
    obj = c_class_for_test
    assert isinstance(obj.any, str) and obj.any == "hello"
    obj.any = 42
    assert isinstance(obj.any, int) and obj.any == 42


def test_c_class_func(c_class_for_test: CClassForTest) -> None:
    obj = c_class_for_test
    assert obj.func(1) == 2
    obj.func = lambda x: x + 2  # type: ignore[assignment]
    assert obj.func(1) == 3


def test_c_class_ulist(c_class_for_test: CClassForTest) -> None:
    obj = c_class_for_test
    assert (
        len(obj.ulist) == 4
        and obj.ulist[0] == 1
        and np.isclose(obj.ulist[1], 2.0)
        and obj.ulist[2] == "three"
        and obj.ulist[3]() == 4
    )
    obj.ulist = [4, 3.0, "two"]
    assert (
        len(obj.ulist) == 3
        and obj.ulist[0] == 4
        and np.isclose(obj.ulist[1], 3.0)
        and obj.ulist[2] == "two"
    )


def test_c_class_udict(c_class_for_test: CClassForTest) -> None:
    obj = c_class_for_test
    assert (
        len(obj.udict) == 4
        and obj.udict["1"] == 1
        and np.isclose(obj.udict["2"], 2.0)
        and obj.udict["3"] == "three"
        and obj.udict["4"]() == 4
    )
    assert obj.str_ == "world"
    obj.udict = {"4": 4, "3": "two", "2": 3.0}
    assert (
        len(obj.udict) == 3
        and obj.udict["4"] == 4
        and obj.udict["3"] == "two"
        and np.isclose(obj.udict["2"], 3.0)
    )
