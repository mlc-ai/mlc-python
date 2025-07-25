import copy

import mlc
import pytest
from mlc.testing.dataclasses import PyClassForTest


@mlc.py_class(init=False)
class CustomInit:
    a: int
    b: str

    def __init__(self, *, b: str, a: int) -> None:
        self.a = a
        self.b = b


@pytest.fixture
def test_obj() -> CustomInit:
    return CustomInit(a=1, b="hello")


@pytest.fixture
def mlc_class_for_test() -> PyClassForTest:
    return PyClassForTest(
        bool_=True,
        i8=8,
        i16=16,
        i32=32,
        i64=64,
        f32=2,
        f64=2.5,
        raw_ptr=mlc.Ptr(0xDEADBEEF),
        dtype="float8",  # type: ignore[arg-type]
        device="cuda:0",  # type: ignore[arg-type]
        any="hello",
        func=lambda x: x + 1,  # type: ignore[arg-type]
        ulist=[1, 2.0, "three", lambda: 4],
        udict={"1": 1, "2": 2.0, "3": "three", "4": lambda: 4},
        str_="world",
        str_readonly="world",
        ###
        list_any=[1, 2.0, "three", lambda: 4],
        list_list_int=[[1, 2, 3], [4, 5, 6]],
        dict_any_any={1: 1.0, 2.0: 2, "three": "four", 4: lambda: 5},
        dict_str_any={"1": 1.0, "2.0": 2, "three": "four", "4": lambda: 5},
        dict_any_str={1: "1.0", 2.0: "2", "three": "four", 4: "5"},
        dict_str_list_int={"1": [1, 2, 3], "2": [4, 5, 6]},
        ###
        opt_bool=None,
        opt_i64=-64,
        opt_f64=-2.5,
        opt_raw_ptr=mlc.Ptr(0xBEEFDEAD),
        opt_dtype="float16",  # type: ignore[arg-type]
        opt_device="cuda:0",  # type: ignore[arg-type]
        opt_func=lambda x: x - 1,  # type: ignore[arg-type]
        opt_ulist=[1, 2.0, "three", lambda: 4],
        opt_udict={"1": 1, "2": 2.0, "3": "three", "4": lambda: 4},
        opt_str="world",
        ###
        opt_list_any=[1, 2.0, "three", lambda: 4],
        opt_list_list_int=[[1, 2, 3], [4, 5, 6]],
        opt_dict_any_any={1: 1.0, 2.0: 2, "three": "four", 4: lambda: 5},
        opt_dict_str_any={"1": 1.0, "2.0": 2, "three": "four", "4": lambda: 5},
        opt_dict_any_str={1: "1.0", 2.0: "2", "three": "four", 4: "5"},
        opt_dict_str_list_int={"1": [1, 2, 3], "2": [4, 5, 6]},
    )


def test_copy_shallow(mlc_class_for_test: PyClassForTest) -> None:
    src = mlc_class_for_test
    dst = copy.copy(src)
    assert src != dst
    assert src.bool_ == dst.bool_
    assert src.i8 == dst.i8
    assert src.i16 == dst.i16
    assert src.i32 == dst.i32
    assert src.i64 == dst.i64
    assert src.f32 == dst.f32
    assert src.f64 == dst.f64
    assert src.raw_ptr.value == dst.raw_ptr.value
    assert src.dtype == dst.dtype
    assert src.device == dst.device
    assert src.any == dst.any
    assert src.func(1) == dst.func(1)
    assert mlc.eq_ptr(src.ulist, dst.ulist)  # type: ignore
    assert mlc.eq_ptr(src.udict, dst.udict)  # type: ignore
    assert src.str_ == dst.str_
    assert mlc.eq_ptr(src.list_any, dst.list_any)  # type: ignore
    assert mlc.eq_ptr(src.list_list_int, dst.list_list_int)  # type: ignore
    assert mlc.eq_ptr(src.dict_any_any, dst.dict_any_any)  # type: ignore
    assert mlc.eq_ptr(src.dict_str_any, dst.dict_str_any)  # type: ignore
    assert mlc.eq_ptr(src.dict_any_str, dst.dict_any_str)  # type: ignore
    assert mlc.eq_ptr(src.dict_str_list_int, dst.dict_str_list_int)  # type: ignore
    assert src.opt_bool == dst.opt_bool
    assert src.opt_i64 == dst.opt_i64
    assert src.opt_f64 == dst.opt_f64
    assert src.opt_raw_ptr.value == dst.opt_raw_ptr.value  # type: ignore[union-attr]
    assert src.opt_dtype == dst.opt_dtype
    assert src.opt_device == dst.opt_device
    assert src.opt_func(2) == dst.opt_func(2)  # type: ignore[misc]
    assert mlc.eq_ptr(src.opt_ulist, dst.opt_ulist)  # type: ignore
    assert mlc.eq_ptr(src.opt_udict, dst.opt_udict)  # type: ignore
    assert src.opt_str == dst.opt_str
    assert mlc.eq_ptr(src.opt_list_any, dst.opt_list_any)  # type: ignore
    assert mlc.eq_ptr(src.opt_list_list_int, dst.opt_list_list_int)  # type: ignore
    assert mlc.eq_ptr(src.opt_dict_any_any, dst.opt_dict_any_any)  # type: ignore
    assert mlc.eq_ptr(src.opt_dict_str_any, dst.opt_dict_str_any)  # type: ignore
    assert mlc.eq_ptr(src.opt_dict_any_str, dst.opt_dict_any_str)  # type: ignore
    assert mlc.eq_ptr(src.opt_dict_str_list_int, dst.opt_dict_str_list_int)  # type: ignore


def test_copy_deep(mlc_class_for_test: PyClassForTest) -> None:
    src = mlc_class_for_test
    dst = copy.deepcopy(src)
    assert src != dst
    assert src.bool_ == dst.bool_
    assert src.i8 == dst.i8
    assert src.i16 == dst.i16
    assert src.i32 == dst.i32
    assert src.i64 == dst.i64
    assert src.f32 == dst.f32
    assert src.f64 == dst.f64
    assert src.raw_ptr.value == dst.raw_ptr.value
    assert src.dtype == dst.dtype
    assert src.device == dst.device
    assert src.any == dst.any
    assert src.func(1) == dst.func(1)
    assert (
        not mlc.eq_ptr(src.ulist, dst.ulist)  # type: ignore
        and len(src.ulist) == len(dst.ulist)
        and src.ulist[0] == dst.ulist[0]
        and src.ulist[1] == dst.ulist[1]
        and src.ulist[2] == dst.ulist[2]
        and src.ulist[3]() == dst.ulist[3]()
    )
    assert (
        not mlc.eq_ptr(src.udict, dst.udict)  # type: ignore
        and len(src.udict) == len(dst.udict)
        and src.udict["1"] == dst.udict["1"]
        and src.udict["2"] == dst.udict["2"]
        and src.udict["3"] == dst.udict["3"]
        and src.udict["4"]() == dst.udict["4"]()
    )
    assert src.str_ == dst.str_
    assert (
        not mlc.eq_ptr(src.list_any, dst.list_any)  # type: ignore
        and len(src.list_any) == len(dst.list_any)
        and src.list_any[0] == dst.list_any[0]
        and src.list_any[1] == dst.list_any[1]
        and src.list_any[2] == dst.list_any[2]
        and src.list_any[3]() == dst.list_any[3]()
    )
    assert (
        not mlc.eq_ptr(src.list_list_int, dst.list_list_int)  # type: ignore
        and len(src.list_list_int) == len(dst.list_list_int)
        and tuple(src.list_list_int[0]) == tuple(dst.list_list_int[0])
        and tuple(src.list_list_int[1]) == tuple(dst.list_list_int[1])
    )
    assert (
        not mlc.eq_ptr(src.dict_any_any, dst.dict_any_any)  # type: ignore
        and len(src.dict_any_any) == len(dst.dict_any_any)
        and src.dict_any_any[1] == dst.dict_any_any[1]
        and src.dict_any_any[2.0] == dst.dict_any_any[2.0]
        and src.dict_any_any["three"] == dst.dict_any_any["three"]
        and src.dict_any_any[4]() == dst.dict_any_any[4]()
    )
    assert (
        not mlc.eq_ptr(src.dict_str_any, dst.dict_str_any)  # type: ignore
        and len(src.dict_str_any) == len(dst.dict_str_any)
        and src.dict_str_any["1"] == dst.dict_str_any["1"]
        and src.dict_str_any["2.0"] == dst.dict_str_any["2.0"]
        and src.dict_str_any["three"] == dst.dict_str_any["three"]
        and src.dict_str_any["4"]() == dst.dict_str_any["4"]()
    )
    assert (
        not mlc.eq_ptr(src.dict_any_str, dst.dict_any_str)  # type: ignore
        and len(src.dict_any_str) == len(dst.dict_any_str)
        and src.dict_any_str[1] == dst.dict_any_str[1]
        and src.dict_any_str[2.0] == dst.dict_any_str[2.0]
        and src.dict_any_str["three"] == dst.dict_any_str["three"]
        and src.dict_any_str[4] == dst.dict_any_str[4]
    )
    assert (
        not mlc.eq_ptr(src.dict_str_list_int, dst.dict_str_list_int)  # type: ignore
        and len(src.dict_str_list_int) == len(dst.dict_str_list_int)
        and tuple(src.dict_str_list_int["1"]) == tuple(dst.dict_str_list_int["1"])
        and tuple(src.dict_str_list_int["2"]) == tuple(dst.dict_str_list_int["2"])
    )
    assert src.opt_i64 == dst.opt_i64
    assert src.opt_f64 == dst.opt_f64
    assert src.opt_raw_ptr.value == dst.opt_raw_ptr.value  # type: ignore[union-attr]
    assert src.opt_dtype == dst.opt_dtype
    assert src.opt_device == dst.opt_device
    assert src.opt_func(2) == dst.opt_func(2)  # type: ignore[misc]
    assert (
        not mlc.eq_ptr(src.opt_ulist, dst.opt_ulist)  # type: ignore
        and len(src.opt_ulist) == len(dst.opt_ulist)  # type: ignore[arg-type]
        and src.opt_ulist[0] == dst.opt_ulist[0]  # type: ignore[index]
        and src.opt_ulist[1] == dst.opt_ulist[1]  # type: ignore[index]
        and src.opt_ulist[2] == dst.opt_ulist[2]  # type: ignore[index]
        and src.opt_ulist[3]() == dst.opt_ulist[3]()  # type: ignore[index]
    )
    assert (
        not mlc.eq_ptr(src.opt_udict, dst.opt_udict)  # type: ignore
        and len(src.opt_udict) == len(dst.opt_udict)  # type: ignore[arg-type]
        and src.opt_udict["1"] == dst.opt_udict["1"]  # type: ignore[index]
        and src.opt_udict["2"] == dst.opt_udict["2"]  # type: ignore[index]
        and src.opt_udict["3"] == dst.opt_udict["3"]  # type: ignore[index]
        and src.opt_udict["4"]() == dst.opt_udict["4"]()  # type: ignore[index]
    )
    assert src.opt_str == dst.opt_str
    assert (
        not mlc.eq_ptr(src.opt_list_any, dst.opt_list_any)  # type: ignore
        and len(src.opt_list_any) == len(dst.opt_list_any)  # type: ignore[arg-type]
        and src.opt_list_any[0] == dst.opt_list_any[0]  # type: ignore[index]
        and src.opt_list_any[1] == dst.opt_list_any[1]  # type: ignore[index]
        and src.opt_list_any[2] == dst.opt_list_any[2]  # type: ignore[index]
        and src.opt_list_any[3]() == dst.opt_list_any[3]()  # type: ignore[index]
    )
    assert (
        not mlc.eq_ptr(src.opt_list_list_int, dst.opt_list_list_int)  # type: ignore
        and len(src.opt_list_list_int) == len(dst.opt_list_list_int)  # type: ignore[arg-type]
        and tuple(src.opt_list_list_int[0]) == tuple(dst.opt_list_list_int[0])  # type: ignore[index]
        and tuple(src.opt_list_list_int[1]) == tuple(dst.opt_list_list_int[1])  # type: ignore[index]
    )
    assert (
        not mlc.eq_ptr(src.opt_dict_any_any, dst.opt_dict_any_any)  # type: ignore
        and len(src.opt_dict_any_any) == len(dst.opt_dict_any_any)  # type: ignore[arg-type]
        and src.opt_dict_any_any[1] == dst.opt_dict_any_any[1]  # type: ignore[index]
        and src.opt_dict_any_any[2.0] == dst.opt_dict_any_any[2.0]  # type: ignore[index]
        and src.opt_dict_any_any["three"] == dst.opt_dict_any_any["three"]  # type: ignore[index]
        and src.opt_dict_any_any[4]() == dst.opt_dict_any_any[4]()  # type: ignore[index]
    )
    assert (
        not mlc.eq_ptr(src.opt_dict_str_any, dst.opt_dict_str_any)  # type: ignore
        and len(src.opt_dict_str_any) == len(dst.opt_dict_str_any)  # type: ignore[arg-type]
        and src.opt_dict_str_any["1"] == dst.opt_dict_str_any["1"]  # type: ignore[index]
        and src.opt_dict_str_any["2.0"] == dst.opt_dict_str_any["2.0"]  # type: ignore[index]
        and src.opt_dict_str_any["three"] == dst.opt_dict_str_any["three"]  # type: ignore[index]
        and src.opt_dict_str_any["4"]() == dst.opt_dict_str_any["4"]()  # type: ignore[index]
    )
    assert (
        not mlc.eq_ptr(src.opt_dict_any_str, dst.opt_dict_any_str)  # type: ignore
        and len(src.opt_dict_any_str) == len(dst.opt_dict_any_str)  # type: ignore[arg-type]
        and src.opt_dict_any_str[1] == dst.opt_dict_any_str[1]  # type: ignore[index]
        and src.opt_dict_any_str[2.0] == dst.opt_dict_any_str[2.0]  # type: ignore[index]
        and src.opt_dict_any_str["three"] == dst.opt_dict_any_str["three"]  # type: ignore[index]
        and src.opt_dict_any_str[4] == dst.opt_dict_any_str[4]  # type: ignore[index]
    )
    assert (
        not mlc.eq_ptr(src.opt_dict_str_list_int, dst.opt_dict_str_list_int)  # type: ignore
        and len(src.opt_dict_str_list_int) == len(dst.opt_dict_str_list_int)  # type: ignore[arg-type]
        and tuple(src.opt_dict_str_list_int["1"]) == tuple(dst.opt_dict_str_list_int["1"])  # type: ignore[index]
        and tuple(src.opt_dict_str_list_int["2"]) == tuple(dst.opt_dict_str_list_int["2"])  # type: ignore[index]
    )


def test_copy_shallow_dataclass(test_obj: CustomInit) -> None:
    src = test_obj
    dst = copy.copy(src)
    assert src != dst
    assert src.a == dst.a
    assert src.b == dst.b


def test_copy_deep_dataclass(test_obj: CustomInit) -> None:
    src = test_obj
    dst = copy.deepcopy(src)
    assert src != dst
    assert src.a == dst.a
    assert src.b == dst.b


def test_copy_replace_dataclass(test_obj: CustomInit) -> None:
    src = test_obj
    dst = mlc.dataclasses.replace(src, a=2)
    assert src != dst
    assert src.a != dst.a
    assert src.b == dst.b
    assert src.a == 1
    assert src.b == "hello"
    assert dst.a == 2
    assert dst.b == "hello"
