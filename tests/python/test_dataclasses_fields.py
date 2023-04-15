from typing import Any, Optional, Union

import mlc
import numpy as np
import pytest


@mlc.c_class("mlc.testing.c_class")
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
    str_readonly: str
    ###
    list_any: list[Any]
    list_list_int: list[list[int]]
    dict_any_any: dict[Any, Any]
    dict_str_any: dict[str, Any]
    dict_any_str: dict[Any, str]
    dict_str_list_int: dict[str, list[int]]
    ###
    opt_i64: Optional[int]
    opt_f64: Optional[float]
    opt_raw_ptr: Optional[mlc.Ptr]
    opt_dtype: Optional[mlc.DataType]
    opt_device: Optional[mlc.Device]
    opt_func: Optional[mlc.Func]
    opt_ulist: Optional[list]
    opt_udict: Optional[dict[Any, Any]]
    opt_str: Optional[str]
    ###
    opt_list_any: Optional[list[Any]]
    opt_list_list_int: Optional[list[list[int]]]
    opt_dict_any_any: Optional[dict]
    opt_dict_str_any: Optional[dict[str, Any]]
    opt_dict_any_str: Optional[dict[Any, str]]
    opt_dict_str_list_int: Optional[dict[str, list[int]]]

    def i64_plus_one(self) -> int:
        return type(self)._C(b"i64_plus_one", self)


@mlc.py_class("mlc.testing.py_class")
class PyClassForTest(mlc.PyClass):
    i8: int  # `py_class` doesn't support `int8`, it will effectively be `int64_t`
    i16: int  # `py_class` doesn't support `int16`, it will effectively be `int64_t`
    i32: int  # `py_class` doesn't support `int32`, it will effectively be `int64_t`
    i64: int
    f32: float  # `py_class` doesn't support `float32`, it will effectively be `float64`
    f64: float
    raw_ptr: mlc.Ptr
    dtype: mlc.DataType
    device: mlc.Device
    any: Any
    func: mlc.Func
    ulist: list[Any]
    udict: dict
    str_: str
    str_readonly: str
    ###
    list_any: list[Any]
    list_list_int: list[list[int]]
    dict_any_any: dict[Any, Any]
    dict_str_any: dict[str, Any]
    dict_any_str: dict[Any, str]
    dict_str_list_int: dict[str, list[int]]
    ###
    opt_i64: Optional[int]
    opt_f64: Optional[float]
    opt_raw_ptr: Optional[mlc.Ptr]
    opt_dtype: Optional[mlc.DataType]
    opt_device: Optional[mlc.Device]
    opt_func: Optional[mlc.Func]
    opt_ulist: Optional[list]
    opt_udict: Optional[dict[Any, Any]]
    opt_str: Optional[str]
    ###
    opt_list_any: Optional[list[Any]]
    opt_list_list_int: Optional[list[list[int]]]
    opt_dict_any_any: Optional[dict]
    opt_dict_str_any: Optional[dict[str, Any]]
    opt_dict_any_str: Optional[dict[Any, str]]
    opt_dict_str_list_int: Optional[dict[str, list[int]]]

    def i64_plus_one(self) -> int:
        return self.i64 + 1


MLCClassForTest = Union[CClassForTest, PyClassForTest]


@pytest.fixture
def mlc_class_for_test(request: pytest.FixtureRequest) -> MLCClassForTest:
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
        "func": lambda x: x + 1,
        "ulist": [1, 2.0, "three", lambda: 4],
        "udict": {"1": 1, "2": 2.0, "3": "three", "4": lambda: 4},
        "str_": "world",
        "str_readonly": "world",
        ###
        "list_any": [1, 2.0, "three", lambda: 4],
        "list_list_int": [[1, 2, 3], [4, 5, 6]],
        "dict_any_any": {1: 1.0, 2.0: 2, "three": "four", 4: lambda: 5},
        "dict_str_any": {"1": 1.0, "2.0": 2, "three": "four", "4": lambda: 5},
        "dict_any_str": {1: "1.0", 2.0: "2", "three": "four", 4: "5"},
        "dict_str_list_int": {"1": [1, 2, 3], "2": [4, 5, 6]},
        ###
        "opt_i64": -64,
        "opt_f64": None,
        "opt_raw_ptr": None,
        "opt_dtype": None,
        "opt_device": "cuda:0",
        "opt_func": None,
        "opt_ulist": None,
        "opt_udict": None,
        "opt_str": None,
        ###
        "opt_list_any": [1, 2.0, "three", lambda: 4],
        "opt_list_list_int": [[1, 2, 3], [4, 5, 6]],
        "opt_dict_any_any": None,
        "opt_dict_str_any": {"1": 1.0, "2.0": 2, "three": "four", "4": lambda: 5},
        "opt_dict_any_str": {1: "1.0", 2.0: "2", "three": "four", 4: "5"},
        "opt_dict_str_list_int": {"1": [1, 2, 3], "2": [4, 5, 6]},
    }
    return request.param(**kwargs)  # type: ignore[arg-type]


def pytest_generate_tests(metafunc: pytest.Metafunc) -> None:
    if "mlc_class_for_test" in metafunc.fixturenames:
        metafunc.parametrize("mlc_class_for_test", [CClassForTest, PyClassForTest], indirect=True)


def test_mlc_class_i8(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.i8 == 8
    obj.i8 *= -2
    assert obj.i8 == -16  # TODO: ban "float -> int" auto conversion
    with pytest.raises(TypeError):
        obj.i8 = "wrong"  # type: ignore[assignment]


def test_mlc_class_overflow(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.i8 == 8
    if isinstance(obj, CClassForTest):
        with pytest.raises(OverflowError):
            obj.i8 = 1234567
    else:
        obj.i8 = 1234567  # `int8_t` is not supported. instead it's backed by `int64_t`
        assert obj.i8 == 1234567


def test_mlc_class_i16(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.i16 == 16
    obj.i16 *= -2
    assert obj.i16 == -32


def test_mlc_class_i32(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.i32 == 32
    obj.i32 *= -2
    assert obj.i32 == -64


def test_mlc_class_i64(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.i64 == 64
    obj.i64 *= -2


def test_mlc_class_f32(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert np.isclose(obj.f32, 1.5)
    obj.f32 *= 2
    assert np.isclose(obj.f32, 3.0)
    with pytest.raises(TypeError):
        obj.f32 = "wrong"  # type: ignore[assignment]


def test_mlc_class_f64(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert np.isclose(obj.f64, 2.5)
    obj.f64 *= 2
    assert np.isclose(obj.f64, 5.0)


def test_mlc_class_raw_ptr(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.raw_ptr.value == 0xDEADBEEF
    obj.raw_ptr = mlc.Ptr(0xDEAD1234)
    assert obj.raw_ptr.value == 0xDEAD1234
    obj.raw_ptr = None  # type: ignore[assignment]
    assert obj.raw_ptr.value is None


def test_mlc_class_dtype(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.dtype == mlc.DataType("float8")
    obj.dtype = "int32"  # type: ignore[assignment]
    assert obj.dtype == mlc.DataType("int32")
    with pytest.raises(ValueError):
        obj.dtype = "int32something"  # type: ignore[assignment]


def test_mlc_class_deivce(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.device == mlc.Device("cuda:0")
    obj.device = "cpu:0"  # type: ignore[assignment]
    assert obj.device == mlc.Device("cpu:0")
    with pytest.raises(TypeError):
        obj.device = 42  # type: ignore[assignment]


def test_mlc_class_any(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert isinstance(obj.any, str) and obj.any == "hello"
    obj.any = 42
    assert isinstance(obj.any, int) and obj.any == 42


def test_mlc_class_func(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.func(1) == 2
    obj.func = lambda x: x + 2  # type: ignore[assignment]
    assert obj.func(1) == 3


def test_mlc_class_ulist(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
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


def test_mlc_class_udict(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert (
        len(obj.udict) == 4
        and obj.udict["1"] == 1
        and np.isclose(obj.udict["2"], 2.0)
        and obj.udict["3"] == "three"
        and obj.udict["4"]() == 4
    )
    obj.udict = {"4": 4, "3": "two", "2": 3.0}
    assert (
        len(obj.udict) == 3
        and obj.udict["4"] == 4
        and obj.udict["3"] == "two"
        and np.isclose(obj.udict["2"], 3.0)
    )


def test_mlc_class_str(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.str_ == "world"
    obj.str_ = "hello"
    assert obj.str_ == "hello"


def test_mlc_class_str_readonly(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.str_readonly == "world"
    if isinstance(obj, CClassForTest):
        with pytest.raises(AttributeError):
            obj.str_readonly = "hello"
    else:
        # TODO: support readonly attribute
        ...
    assert obj.str_readonly == "world"


def test_mlc_class_list_any(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert (
        len(obj.list_any) == 4
        and obj.list_any[0] == 1
        and np.isclose(obj.list_any[1], 2.0)
        and obj.list_any[2] == "three"
        and obj.list_any[3]() == 4
    )
    obj.list_any = [4, 3.0, "two"]
    with pytest.raises(TypeError):
        obj.list_any = mlc.Str("wrong")  # type: ignore[assignment]
    assert (
        len(obj.list_any) == 3
        and obj.list_any[0] == 4
        and np.isclose(obj.list_any[1], 3.0)
        and obj.list_any[2] == "two"
    )


def test_mlc_class_list_list_int(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert (
        len(obj.list_list_int) == 2
        and len(obj.list_list_int[0]) == 3
        and obj.list_list_int[0][0] == 1
        and obj.list_list_int[0][1] == 2
        and obj.list_list_int[0][2] == 3
        and len(obj.list_list_int[1]) == 3
        and obj.list_list_int[1][0] == 4
        and obj.list_list_int[1][1] == 5
        and obj.list_list_int[1][2] == 6
    )
    obj.list_list_int = [[]]
    with pytest.raises(TypeError) as e:
        obj.list_list_int = [4, 3, 2, 1, 2, 3]  # type: ignore[list-item]
    assert "Expected `list` or `tuple`, but got" in str(e.value)
    with pytest.raises(TypeError) as e:
        obj.list_list_int = None  # type: ignore[assignment]
    assert "Expected `list` or `tuple`, but got" in str(e.value)
    assert len(obj.list_list_int) == 1 and len(obj.list_list_int[0]) == 0


def test_mlc_class_dict_any_any(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert (
        len(obj.dict_any_any) == 4
        and obj.dict_any_any[1] == 1.0
        and np.isclose(obj.dict_any_any[2.0], 2)
        and obj.dict_any_any["three"] == "four"
        and obj.dict_any_any[4]() == 5
    )
    obj.dict_any_any = {4: 4, "three": "two", 3.0: 3}
    with pytest.raises(TypeError):
        obj.dict_any_any = mlc.Str("wrong")  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.dict_any_any = 42  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.dict_any_any = None  # type: ignore[assignment]
    assert (
        len(obj.dict_any_any) == 3
        and obj.dict_any_any[4] == 4
        and obj.dict_any_any["three"] == "two"
        and np.isclose(obj.dict_any_any[3.0], 3)
    )


def test_mlc_class_dict_str_any(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert (
        len(obj.dict_str_any) == 4
        and obj.dict_str_any["1"] == 1.0
        and np.isclose(obj.dict_str_any["2.0"], 2)
        and obj.dict_str_any["three"] == "four"
        and obj.dict_str_any["4"]() == 5
    )
    obj.dict_str_any = {}
    with pytest.raises(TypeError):
        obj.dict_str_any = mlc.Str("wrong")  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.dict_str_any = 42  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.dict_str_any = None  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.dict_str_any = {4: 4, "three": "two", 3.0: 3}  # type: ignore[dict-item]
    assert len(obj.dict_str_any) == 0


def test_mlc_class_dict_any_str(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert (
        len(obj.dict_any_str) == 4
        and obj.dict_any_str[1] == "1.0"
        and obj.dict_any_str[2.0] == "2"
        and obj.dict_any_str["three"] == "four"
        and obj.dict_any_str[4] == "5"
    )
    obj.dict_any_str = {4: "4", "three": "two", 3.0: "3"}
    with pytest.raises(TypeError):
        obj.dict_any_str = mlc.Str("wrong")  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.dict_any_str = 42  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.dict_any_str = None  # type: ignore[assignment]
    assert (
        len(obj.dict_any_str) == 3
        and obj.dict_any_str[4] == "4"
        and obj.dict_any_str["three"] == "two"
        and obj.dict_any_str[3.0] == "3"
    )


def test_mlc_class_dict_str_list_int(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert (
        len(obj.dict_str_list_int) == 2
        and len(obj.dict_str_list_int["1"]) == 3
        and obj.dict_str_list_int["1"][0] == 1
        and obj.dict_str_list_int["1"][1] == 2
        and obj.dict_str_list_int["1"][2] == 3
        and len(obj.dict_str_list_int["2"]) == 3
        and obj.dict_str_list_int["2"][0] == 4
        and obj.dict_str_list_int["2"][1] == 5
        and obj.dict_str_list_int["2"][2] == 6
    )
    obj.dict_str_list_int = {"1": [4, 3, 2], "2": [1, 2, 3]}
    with pytest.raises(TypeError):
        obj.dict_str_list_int = mlc.Str("wrong")  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.dict_str_list_int = 42  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.dict_str_list_int = None  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.dict_str_list_int = {"1": 1, "2": [2]}  # type: ignore[dict-item]
    assert (
        len(obj.dict_str_list_int) == 2
        and tuple(obj.dict_str_list_int["1"]) == (4, 3, 2)
        and tuple(obj.dict_str_list_int["2"]) == (1, 2, 3)
    )


def test_mlc_class_opt_i64(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.opt_i64 == -64
    obj.opt_i64 *= -2
    assert obj.opt_i64 == 128
    obj.opt_i64 = None
    assert obj.opt_i64 is None


def test_mlc_class_opt_f64(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.opt_f64 is None
    obj.opt_f64 = 1.5
    assert np.isclose(obj.opt_f64, 1.5)
    obj.opt_f64 = None
    assert obj.opt_f64 is None


def test_mlc_class_opt_raw_ptr(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.opt_raw_ptr is None
    obj.opt_raw_ptr = mlc.Ptr(0xDEAD1234)
    assert obj.opt_raw_ptr.value == 0xDEAD1234
    obj.opt_raw_ptr = None
    assert obj.opt_raw_ptr is None


def test_mlc_class_opt_dtype(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.opt_dtype is None
    obj.opt_dtype = "int32"  # type: ignore[assignment]
    with pytest.raises(ValueError):
        obj.opt_dtype = "int32something"  # type: ignore[assignment]
    assert obj.opt_dtype == mlc.DataType("int32")
    obj.opt_dtype = None
    assert obj.opt_dtype is None


def test_mlc_class_opt_device(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.opt_device == mlc.Device("cuda:0")
    obj.opt_device = "cpu:0"  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.opt_device = 42  # type: ignore[assignment]
    assert obj.opt_device == mlc.Device("cpu:0")
    obj.opt_device = None
    assert obj.opt_device is None


def test_mlc_class_opt_func(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.opt_func is None
    obj.opt_func = lambda x: x + 2  # type: ignore[assignment]
    assert obj.opt_func(1) == 3  # type: ignore[misc]
    obj.opt_func = None
    assert obj.opt_func is None
    with pytest.raises(TypeError):
        obj.opt_func = mlc.Str("wrong")  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.opt_func = 42  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.opt_func = mlc.Device("cuda:0")


def test_mlc_class_opt_ulist(mlc_class_for_test: MLCClassForTest) -> None:
    # TODO: support assinging `mlc.List` to `opt_ulist`
    obj = mlc_class_for_test
    assert obj.opt_ulist is None
    obj.opt_ulist = [4, 3.0, "two"]
    with pytest.raises(TypeError):
        obj.opt_ulist = mlc.Str("wrong")  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.opt_ulist = 42  # type: ignore[assignment]
    assert (
        len(obj.opt_ulist) == 3  # type: ignore[arg-type]
        and obj.opt_ulist[0] == 4  # type: ignore[index]
        and np.isclose(obj.opt_ulist[1], 3.0)  # type: ignore[index]
        and obj.opt_ulist[2] == "two"  # type: ignore[index]
    )
    obj.opt_ulist = None
    assert obj.opt_ulist is None


def test_mlc_class_opt_udict(mlc_class_for_test: MLCClassForTest) -> None:
    # TODO: support assinging `mlc.Dict` to `opt_udict`
    obj = mlc_class_for_test
    assert obj.opt_udict is None
    obj.opt_udict = {"4": 4, "3": "two", "2": 3.0}
    with pytest.raises(TypeError):
        obj.opt_udict = mlc.Str("wrong")  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.opt_udict = 42  # type: ignore[assignment]
    assert (
        len(obj.opt_udict) == 3  # type: ignore[arg-type]
        and obj.opt_udict["4"] == 4  # type: ignore[index]
        and obj.opt_udict["3"] == "two"  # type: ignore[index]
        and np.isclose(obj.opt_udict["2"], 3.0)  # type: ignore[index]
    )
    obj.opt_udict = None
    assert obj.opt_udict is None


def test_mlc_class_opt_str(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.opt_str is None
    obj.opt_str = "hello"
    assert obj.opt_str == "hello"
    obj.opt_str = None
    assert obj.opt_str is None


def test_mlc_class_opt_list_any(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert len(obj.opt_list_any) == 4  # type: ignore[arg-type]
    assert tuple(obj.opt_list_any[:3]) == (1, 2.0, "three")  # type: ignore[index]
    assert obj.opt_list_any[3]() == 4  # type: ignore[index]
    obj.opt_list_any = [4, 3.0, "two"]
    with pytest.raises(TypeError):
        obj.opt_list_any = mlc.Str("wrong")  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.opt_list_any = 42  # type: ignore[assignment]
    assert (
        len(obj.opt_list_any) == 3  # type: ignore[arg-type]
        and obj.opt_list_any[0] == 4  # type: ignore[index]
        and np.isclose(obj.opt_list_any[1], 3.0)  # type: ignore[index]
        and obj.opt_list_any[2] == "two"  # type: ignore[index]
    )
    obj.opt_list_any = None
    assert obj.opt_list_any is None


def test_mlc_class_opt_list_list_int(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert len(obj.opt_list_list_int) == 2  # type: ignore[arg-type]
    assert len(obj.opt_list_list_int[0]) == 3  # type: ignore[index]
    assert tuple(obj.opt_list_list_int[0]) == (1, 2, 3)  # type: ignore[index]
    assert len(obj.opt_list_list_int[1]) == 3  # type: ignore[index]
    assert tuple(obj.opt_list_list_int[1]) == (4, 5, 6)  # type: ignore[index]
    obj.opt_list_list_int = [[4, 3, 2], [1, 2, 3]]
    with pytest.raises(TypeError):
        obj.opt_list_list_int = mlc.Str("wrong")  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.opt_list_list_int = 42  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.opt_list_list_int = [1, 2, 3]  # type: ignore[list-item]
    assert (
        len(obj.opt_list_list_int) == 2  # type: ignore[arg-type]
        and tuple(obj.opt_list_list_int[0]) == (4, 3, 2)  # type: ignore[index]
        and tuple(obj.opt_list_list_int[1]) == (1, 2, 3)  # type: ignore[index]
    )
    obj.opt_list_list_int = None
    assert obj.opt_list_list_int is None


def test_mlc_class_opt_dict_any_any(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.opt_dict_any_any is None
    obj.opt_dict_any_any = {4: 4, "three": "two", 3.0: 3}
    with pytest.raises(TypeError):
        obj.opt_dict_any_any = mlc.Str("wrong")  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.opt_dict_any_any = 42  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.opt_dict_any_any = [1, 2, 3]  # type: ignore[assignment]
    assert (
        len(obj.opt_dict_any_any) == 3  # type: ignore[arg-type]
        and obj.opt_dict_any_any[4] == 4  # type: ignore[index]
        and obj.opt_dict_any_any["three"] == "two"  # type: ignore[index]
        and np.isclose(obj.opt_dict_any_any[3.0], 3)  # type: ignore[index]
    )
    obj.opt_dict_any_any = None
    assert obj.opt_dict_any_any is None


def test_mlc_class_opt_dict_str_any(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.opt_dict_str_any is not None
    assert (
        len(obj.opt_dict_str_any) == 4
        and obj.opt_dict_str_any["1"] == 1.0
        and np.isclose(obj.opt_dict_str_any["2.0"], 2)
        and obj.opt_dict_str_any["three"] == "four"
        and obj.opt_dict_str_any["4"]() == 5
    )
    obj.opt_dict_str_any = {}
    with pytest.raises(TypeError):
        obj.opt_dict_str_any = mlc.Str("wrong")  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.opt_dict_str_any = 42  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.opt_dict_str_any = [1, 2, 3]  # type: ignore[assignment]
    assert len(obj.opt_dict_str_any) == 0  # type: ignore[arg-type]
    obj.opt_dict_str_any = None
    assert obj.opt_dict_str_any is None


def test_mlc_class_opt_dict_any_str(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.opt_dict_any_str is not None
    assert (
        len(obj.opt_dict_any_str) == 4
        and obj.opt_dict_any_str[1] == "1.0"
        and obj.opt_dict_any_str[2.0] == "2"
        and obj.opt_dict_any_str["three"] == "four"
        and obj.opt_dict_any_str[4] == "5"
    )
    obj.opt_dict_any_str = {4: "4", "three": "two", 3.0: "3"}
    with pytest.raises(TypeError):
        obj.opt_dict_any_str = mlc.Str("wrong")  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.opt_dict_any_str = 42  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.opt_dict_any_str = [1, 2, 3]  # type: ignore[assignment]
    assert (
        len(obj.opt_dict_any_str) == 3  # type: ignore[arg-type]
        and obj.opt_dict_any_str[4] == "4"  # type: ignore[index]
        and obj.opt_dict_any_str["three"] == "two"  # type: ignore[index]
        and obj.opt_dict_any_str[3.0] == "3"  # type: ignore[index]
    )
    obj.opt_dict_any_str = None
    assert obj.opt_dict_any_str is None


def test_mlc_class_opt_dict_str_list_int(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.opt_dict_str_list_int is not None
    assert (
        len(obj.opt_dict_str_list_int) == 2
        and len(obj.opt_dict_str_list_int["1"]) == 3
        and obj.opt_dict_str_list_int["1"][0] == 1
        and obj.opt_dict_str_list_int["1"][1] == 2
        and obj.opt_dict_str_list_int["1"][2] == 3
        and len(obj.opt_dict_str_list_int["2"]) == 3
        and obj.opt_dict_str_list_int["2"][0] == 4
        and obj.opt_dict_str_list_int["2"][1] == 5
        and obj.opt_dict_str_list_int["2"][2] == 6
    )
    obj.opt_dict_str_list_int = {"1": [4, 3, 2], "2": [1, 2, 3]}
    with pytest.raises(TypeError):
        obj.opt_dict_str_list_int = mlc.Str("wrong")  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.opt_dict_str_list_int = 42  # type: ignore[assignment]
    with pytest.raises(TypeError):
        obj.opt_dict_str_list_int = [1, 2, 3]  # type: ignore[assignment]
    assert (
        len(obj.opt_dict_str_list_int) == 2  # type: ignore[arg-type]
        and tuple(obj.opt_dict_str_list_int["1"]) == (4, 3, 2)  # type: ignore[index]
        and tuple(obj.opt_dict_str_list_int["2"]) == (1, 2, 3)  # type: ignore[index]
    )
    obj.opt_dict_str_list_int = None
    assert obj.opt_dict_str_list_int is None


def test_mlc_class_mem_fn(mlc_class_for_test: MLCClassForTest) -> None:
    obj = mlc_class_for_test
    assert obj.i64 == 64
    assert obj.i64_plus_one() == 65
