import copy
from typing import Any, Optional

import mlc
import pytest


@mlc.py_class
class PyClassForTest(mlc.PyClass):
    bool_: bool
    i64: int
    f64: float
    raw_ptr: mlc.Ptr
    dtype: mlc.DataType
    device: mlc.Device
    any: Any
    func: mlc.Func
    ulist: list[Any]
    udict: dict
    str_: str
    ###
    list_any: list[Any]
    list_list_int: list[list[int]]
    dict_any_any: dict[Any, Any]
    dict_str_any: dict[str, Any]
    dict_any_str: dict[Any, str]
    dict_str_list_int: dict[str, list[int]]
    ###
    opt_bool: Optional[bool]
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


@pytest.fixture
def mlc_class_for_test() -> PyClassForTest:
    return PyClassForTest(
        bool_=True,
        i64=64,
        f64=2.5,
        raw_ptr=mlc.Ptr(0xDEADBEEF),
        dtype="float8",
        device="cuda:0",
        any="hello",
        func=lambda x: x + 1,
        ulist=[1, 2.0, "three", lambda: 4],
        udict={"1": 1, "2": 2.0, "3": "three", "4": lambda: 4},
        str_="world",
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
        opt_dtype="float16",
        opt_device="cuda:0",
        opt_func=lambda x: x - 1,
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
    assert src.i64 == dst.i64
    assert src.f64 == dst.f64
    assert src.raw_ptr.value == dst.raw_ptr.value
    assert src.dtype == dst.dtype
    assert src.device == dst.device
    assert src.any == dst.any
    assert src.func(1) == dst.func(1)
    assert src.ulist == dst.ulist
    assert src.udict == dst.udict
    assert src.str_ == dst.str_
    assert src.list_any == dst.list_any
    assert src.list_list_int == dst.list_list_int
    assert src.dict_any_any == dst.dict_any_any
    assert src.dict_str_any == dst.dict_str_any
    assert src.dict_any_str == dst.dict_any_str
    assert src.dict_str_list_int == dst.dict_str_list_int
    assert src.opt_bool == dst.opt_bool
    assert src.opt_i64 == dst.opt_i64
    assert src.opt_f64 == dst.opt_f64
    assert src.opt_raw_ptr.value == dst.opt_raw_ptr.value  # type: ignore[union-attr]
    assert src.opt_dtype == dst.opt_dtype
    assert src.opt_device == dst.opt_device
    assert src.opt_func(2) == dst.opt_func(2)  # type: ignore[misc]
    assert src.opt_ulist == dst.opt_ulist
    assert src.opt_udict == dst.opt_udict
    assert src.opt_str == dst.opt_str
    assert src.opt_list_any == dst.opt_list_any
    assert src.opt_list_list_int == dst.opt_list_list_int
    assert src.opt_dict_any_any == dst.opt_dict_any_any
    assert src.opt_dict_str_any == dst.opt_dict_str_any
    assert src.opt_dict_any_str == dst.opt_dict_any_str
    assert src.opt_dict_str_list_int == dst.opt_dict_str_list_int


def test_copy_deep(mlc_class_for_test: PyClassForTest) -> None:
    src = mlc_class_for_test
    dst = copy.deepcopy(src)
    assert src != dst
    assert src.i64 == dst.i64
    assert src.f64 == dst.f64
    assert src.raw_ptr.value == dst.raw_ptr.value
    assert src.dtype == dst.dtype
    assert src.device == dst.device
    assert src.any == dst.any
    assert src.func(1) == dst.func(1)
    assert (
        src.ulist != dst.ulist
        and len(src.ulist) == len(dst.ulist)
        and src.ulist[0] == dst.ulist[0]
        and src.ulist[1] == dst.ulist[1]
        and src.ulist[2] == dst.ulist[2]
        and src.ulist[3]() == dst.ulist[3]()
    )
    assert (
        src.udict != dst.udict
        and len(src.udict) == len(dst.udict)
        and src.udict["1"] == dst.udict["1"]
        and src.udict["2"] == dst.udict["2"]
        and src.udict["3"] == dst.udict["3"]
        and src.udict["4"]() == dst.udict["4"]()
    )
    assert src.str_ == dst.str_
    assert (
        src.list_any != dst.list_any
        and len(src.list_any) == len(dst.list_any)
        and src.list_any[0] == dst.list_any[0]
        and src.list_any[1] == dst.list_any[1]
        and src.list_any[2] == dst.list_any[2]
        and src.list_any[3]() == dst.list_any[3]()
    )
    assert (
        src.list_list_int != dst.list_list_int
        and len(src.list_list_int) == len(dst.list_list_int)
        and tuple(src.list_list_int[0]) == tuple(dst.list_list_int[0])
        and tuple(src.list_list_int[1]) == tuple(dst.list_list_int[1])
    )
    assert (
        src.dict_any_any != dst.dict_any_any
        and len(src.dict_any_any) == len(dst.dict_any_any)
        and src.dict_any_any[1] == dst.dict_any_any[1]
        and src.dict_any_any[2.0] == dst.dict_any_any[2.0]
        and src.dict_any_any["three"] == dst.dict_any_any["three"]
        and src.dict_any_any[4]() == dst.dict_any_any[4]()
    )
    assert (
        src.dict_str_any != dst.dict_str_any
        and len(src.dict_str_any) == len(dst.dict_str_any)
        and src.dict_str_any["1"] == dst.dict_str_any["1"]
        and src.dict_str_any["2.0"] == dst.dict_str_any["2.0"]
        and src.dict_str_any["three"] == dst.dict_str_any["three"]
        and src.dict_str_any["4"]() == dst.dict_str_any["4"]()
    )
    assert (
        src.dict_any_str != dst.dict_any_str
        and len(src.dict_any_str) == len(dst.dict_any_str)
        and src.dict_any_str[1] == dst.dict_any_str[1]
        and src.dict_any_str[2.0] == dst.dict_any_str[2.0]
        and src.dict_any_str["three"] == dst.dict_any_str["three"]
        and src.dict_any_str[4] == dst.dict_any_str[4]
    )
    assert (
        src.dict_str_list_int != dst.dict_str_list_int
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
        src.opt_ulist != dst.opt_ulist
        and len(src.opt_ulist) == len(dst.opt_ulist)  # type: ignore[arg-type]
        and src.opt_ulist[0] == dst.opt_ulist[0]  # type: ignore[index]
        and src.opt_ulist[1] == dst.opt_ulist[1]  # type: ignore[index]
        and src.opt_ulist[2] == dst.opt_ulist[2]  # type: ignore[index]
        and src.opt_ulist[3]() == dst.opt_ulist[3]()  # type: ignore[index]
    )
    assert (
        src.opt_udict != dst.opt_udict
        and len(src.opt_udict) == len(dst.opt_udict)  # type: ignore[arg-type]
        and src.opt_udict["1"] == dst.opt_udict["1"]  # type: ignore[index]
        and src.opt_udict["2"] == dst.opt_udict["2"]  # type: ignore[index]
        and src.opt_udict["3"] == dst.opt_udict["3"]  # type: ignore[index]
        and src.opt_udict["4"]() == dst.opt_udict["4"]()  # type: ignore[index]
    )
    assert src.opt_str == dst.opt_str
    assert (
        src.opt_list_any != dst.opt_list_any
        and len(src.opt_list_any) == len(dst.opt_list_any)  # type: ignore[arg-type]
        and src.opt_list_any[0] == dst.opt_list_any[0]  # type: ignore[index]
        and src.opt_list_any[1] == dst.opt_list_any[1]  # type: ignore[index]
        and src.opt_list_any[2] == dst.opt_list_any[2]  # type: ignore[index]
        and src.opt_list_any[3]() == dst.opt_list_any[3]()  # type: ignore[index]
    )
    assert (
        src.opt_list_list_int != dst.opt_list_list_int
        and len(src.opt_list_list_int) == len(dst.opt_list_list_int)  # type: ignore[arg-type]
        and tuple(src.opt_list_list_int[0]) == tuple(dst.opt_list_list_int[0])  # type: ignore[index]
        and tuple(src.opt_list_list_int[1]) == tuple(dst.opt_list_list_int[1])  # type: ignore[index]
    )
    assert (
        src.opt_dict_any_any != dst.opt_dict_any_any
        and len(src.opt_dict_any_any) == len(dst.opt_dict_any_any)  # type: ignore[arg-type]
        and src.opt_dict_any_any[1] == dst.opt_dict_any_any[1]  # type: ignore[index]
        and src.opt_dict_any_any[2.0] == dst.opt_dict_any_any[2.0]  # type: ignore[index]
        and src.opt_dict_any_any["three"] == dst.opt_dict_any_any["three"]  # type: ignore[index]
        and src.opt_dict_any_any[4]() == dst.opt_dict_any_any[4]()  # type: ignore[index]
    )
    assert (
        src.opt_dict_str_any != dst.opt_dict_str_any
        and len(src.opt_dict_str_any) == len(dst.opt_dict_str_any)  # type: ignore[arg-type]
        and src.opt_dict_str_any["1"] == dst.opt_dict_str_any["1"]  # type: ignore[index]
        and src.opt_dict_str_any["2.0"] == dst.opt_dict_str_any["2.0"]  # type: ignore[index]
        and src.opt_dict_str_any["three"] == dst.opt_dict_str_any["three"]  # type: ignore[index]
        and src.opt_dict_str_any["4"]() == dst.opt_dict_str_any["4"]()  # type: ignore[index]
    )
    assert (
        src.opt_dict_any_str != dst.opt_dict_any_str
        and len(src.opt_dict_any_str) == len(dst.opt_dict_any_str)  # type: ignore[arg-type]
        and src.opt_dict_any_str[1] == dst.opt_dict_any_str[1]  # type: ignore[index]
        and src.opt_dict_any_str[2.0] == dst.opt_dict_any_str[2.0]  # type: ignore[index]
        and src.opt_dict_any_str["three"] == dst.opt_dict_any_str["three"]  # type: ignore[index]
        and src.opt_dict_any_str[4] == dst.opt_dict_any_str[4]  # type: ignore[index]
    )
    assert (
        src.opt_dict_str_list_int != dst.opt_dict_str_list_int
        and len(src.opt_dict_str_list_int) == len(dst.opt_dict_str_list_int)  # type: ignore[arg-type]
        and tuple(src.opt_dict_str_list_int["1"]) == tuple(dst.opt_dict_str_list_int["1"])  # type: ignore[index]
        and tuple(src.opt_dict_str_list_int["2"]) == tuple(dst.opt_dict_str_list_int["2"])  # type: ignore[index]
    )
