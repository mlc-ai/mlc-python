from typing import Any, Optional

import mlc


@mlc.c_class("mlc.testing.c_class")
class CClassForTest(mlc.Object):
    bool_: bool
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
        return type(self)._C(b"i64_plus_one", self)


@mlc.py_class("mlc.testing.py_class")
class PyClassForTest(mlc.PyClass):
    bool_: bool
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


def visit_fields(obj: mlc.Object) -> list[tuple[str, str, Any]]:
    types, names, values = _C_VisitFields(obj)
    return list(zip(types, names, values))


def field_get(obj: mlc.Object, name: str) -> Any:
    return _C_FieldGet(obj, name)


def field_set(obj: mlc.Object, name: str, value: Any) -> None:
    _C_FieldSet(obj, name, value)


_C_VisitFields = mlc.Func.get("mlc.testing.VisitFields")
_C_FieldGet = mlc.Func.get("mlc.testing.FieldGet")
_C_FieldSet = mlc.Func.get("mlc.testing.FieldSet")
