from typing import Any, Optional

import mlc
import mlc.dataclasses as mlcd


@mlcd.py_class
class PyClassForTest(mlcd.PyClass):
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


def test_prototype_py() -> None:
    expected = """
@mlc.dataclasses.c_class('test_dataclasses_prototype.PyClassForTest')
class PyClassForTest:
  i64: int
  f64: float
  raw_ptr: mlc.Ptr
  dtype: mlc.DataType
  device: mlc.Device
  any: Any
  func: mlc.Func
  ulist: list[Any]
  udict: dict[Any, Any]
  str_: str
  str_readonly: str
  list_any: list[Any]
  list_list_int: list[list[int]]
  dict_any_any: dict[Any, Any]
  dict_str_any: dict[str, Any]
  dict_any_str: dict[Any, str]
  dict_str_list_int: dict[str, list[int]]
  opt_i64: int | None
  opt_f64: float | None
  opt_raw_ptr: mlc.Ptr | None
  opt_dtype: mlc.DataType | None
  opt_device: mlc.Device | None
  opt_func: mlc.Func | None
  opt_ulist: list[Any] | None
  opt_udict: dict[Any, Any] | None
  opt_str: str | None
  opt_list_any: list[Any] | None
  opt_list_list_int: list[list[int]] | None
  opt_dict_any_any: dict[Any, Any] | None
  opt_dict_str_any: dict[str, Any] | None
  opt_dict_any_str: dict[Any, str] | None
  opt_dict_str_list_int: dict[str, list[int]] | None
""".strip()
    actual = mlcd.prototype_py(PyClassForTest).strip()
    assert actual == expected


def test_prototype_cxx() -> None:
    expected = """
namespace test_dataclasses_prototype {
struct PyClassForTestObj : public ::mlc::Object {
  int64_t i64;
  double f64;
  void* raw_ptr;
  DLDataType dtype;
  DLDevice device;
  ::mlc::Any any;
  ::mlc::Func func;
  ::mlc::List<::mlc::Any> ulist;
  ::mlc::Dict<::mlc::Any, ::mlc::Any> udict;
  ::mlc::Str str_;
  ::mlc::Str str_readonly;
  ::mlc::List<::mlc::Any> list_any;
  ::mlc::List<::mlc::List<int64_t>> list_list_int;
  ::mlc::Dict<::mlc::Any, ::mlc::Any> dict_any_any;
  ::mlc::Dict<::mlc::Str, ::mlc::Any> dict_str_any;
  ::mlc::Dict<::mlc::Any, ::mlc::Str> dict_any_str;
  ::mlc::Dict<::mlc::Str, ::mlc::List<int64_t>> dict_str_list_int;
  ::mlc::Optional<int64_t> opt_i64;
  ::mlc::Optional<double> opt_f64;
  ::mlc::Optional<void*> opt_raw_ptr;
  ::mlc::Optional<DLDataType> opt_dtype;
  ::mlc::Optional<DLDevice> opt_device;
  ::mlc::Optional<::mlc::Func> opt_func;
  ::mlc::Optional<::mlc::List<::mlc::Any>> opt_ulist;
  ::mlc::Optional<::mlc::Dict<::mlc::Any, ::mlc::Any>> opt_udict;
  ::mlc::Optional<::mlc::Str> opt_str;
  ::mlc::Optional<::mlc::List<::mlc::Any>> opt_list_any;
  ::mlc::Optional<::mlc::List<::mlc::List<int64_t>>> opt_list_list_int;
  ::mlc::Optional<::mlc::Dict<::mlc::Any, ::mlc::Any>> opt_dict_any_any;
  ::mlc::Optional<::mlc::Dict<::mlc::Str, ::mlc::Any>> opt_dict_str_any;
  ::mlc::Optional<::mlc::Dict<::mlc::Any, ::mlc::Str>> opt_dict_any_str;
  ::mlc::Optional<::mlc::Dict<::mlc::Str, ::mlc::List<int64_t>>> opt_dict_str_list_int;
  explicit PyClassForTestObj(int64_t i64, double f64, void* raw_ptr, DLDataType dtype, DLDevice device, ::mlc::Any any, ::mlc::Func func, ::mlc::List<::mlc::Any> ulist, ::mlc::Dict<::mlc::Any, ::mlc::Any> udict, ::mlc::Str str_, ::mlc::Str str_readonly, ::mlc::List<::mlc::Any> list_any, ::mlc::List<::mlc::List<int64_t>> list_list_int, ::mlc::Dict<::mlc::Any, ::mlc::Any> dict_any_any, ::mlc::Dict<::mlc::Str, ::mlc::Any> dict_str_any, ::mlc::Dict<::mlc::Any, ::mlc::Str> dict_any_str, ::mlc::Dict<::mlc::Str, ::mlc::List<int64_t>> dict_str_list_int, ::mlc::Optional<int64_t> opt_i64, ::mlc::Optional<double> opt_f64, ::mlc::Optional<void*> opt_raw_ptr, ::mlc::Optional<DLDataType> opt_dtype, ::mlc::Optional<DLDevice> opt_device, ::mlc::Optional<::mlc::Func> opt_func, ::mlc::Optional<::mlc::List<::mlc::Any>> opt_ulist, ::mlc::Optional<::mlc::Dict<::mlc::Any, ::mlc::Any>> opt_udict, ::mlc::Optional<::mlc::Str> opt_str, ::mlc::Optional<::mlc::List<::mlc::Any>> opt_list_any, ::mlc::Optional<::mlc::List<::mlc::List<int64_t>>> opt_list_list_int, ::mlc::Optional<::mlc::Dict<::mlc::Any, ::mlc::Any>> opt_dict_any_any, ::mlc::Optional<::mlc::Dict<::mlc::Str, ::mlc::Any>> opt_dict_str_any, ::mlc::Optional<::mlc::Dict<::mlc::Any, ::mlc::Str>> opt_dict_any_str, ::mlc::Optional<::mlc::Dict<::mlc::Str, ::mlc::List<int64_t>>> opt_dict_str_list_int): i64(i64), f64(f64), raw_ptr(raw_ptr), dtype(dtype), device(device), any(any), func(func), ulist(ulist), udict(udict), str_(str_), str_readonly(str_readonly), list_any(list_any), list_list_int(list_list_int), dict_any_any(dict_any_any), dict_str_any(dict_str_any), dict_any_str(dict_any_str), dict_str_list_int(dict_str_list_int), opt_i64(opt_i64), opt_f64(opt_f64), opt_raw_ptr(opt_raw_ptr), opt_dtype(opt_dtype), opt_device(opt_device), opt_func(opt_func), opt_ulist(opt_ulist), opt_udict(opt_udict), opt_str(opt_str), opt_list_any(opt_list_any), opt_list_list_int(opt_list_list_int), opt_dict_any_any(opt_dict_any_any), opt_dict_str_any(opt_dict_str_any), opt_dict_any_str(opt_dict_any_str), opt_dict_str_list_int(opt_dict_str_list_int) {}
  MLC_DEF_DYN_TYPE(_EXPORTS, PyClassForTestObj, ::mlc::Object, "test_dataclasses_prototype.PyClassForTest");
};  // struct PyClassForTestObj

struct PyClassForTest : public ::mlc::ObjectRef {
  MLC_DEF_OBJ_REF(_EXPORTS, PyClassForTest, PyClassForTestObj, ::mlc::ObjectRef)
    .Field("i64", &PyClassForTestObj::i64)
    .Field("f64", &PyClassForTestObj::f64)
    .Field("raw_ptr", &PyClassForTestObj::raw_ptr)
    .Field("dtype", &PyClassForTestObj::dtype)
    .Field("device", &PyClassForTestObj::device)
    .Field("any", &PyClassForTestObj::any)
    .Field("func", &PyClassForTestObj::func)
    .Field("ulist", &PyClassForTestObj::ulist)
    .Field("udict", &PyClassForTestObj::udict)
    .Field("str_", &PyClassForTestObj::str_)
    .Field("str_readonly", &PyClassForTestObj::str_readonly)
    .Field("list_any", &PyClassForTestObj::list_any)
    .Field("list_list_int", &PyClassForTestObj::list_list_int)
    .Field("dict_any_any", &PyClassForTestObj::dict_any_any)
    .Field("dict_str_any", &PyClassForTestObj::dict_str_any)
    .Field("dict_any_str", &PyClassForTestObj::dict_any_str)
    .Field("dict_str_list_int", &PyClassForTestObj::dict_str_list_int)
    .Field("opt_i64", &PyClassForTestObj::opt_i64)
    .Field("opt_f64", &PyClassForTestObj::opt_f64)
    .Field("opt_raw_ptr", &PyClassForTestObj::opt_raw_ptr)
    .Field("opt_dtype", &PyClassForTestObj::opt_dtype)
    .Field("opt_device", &PyClassForTestObj::opt_device)
    .Field("opt_func", &PyClassForTestObj::opt_func)
    .Field("opt_ulist", &PyClassForTestObj::opt_ulist)
    .Field("opt_udict", &PyClassForTestObj::opt_udict)
    .Field("opt_str", &PyClassForTestObj::opt_str)
    .Field("opt_list_any", &PyClassForTestObj::opt_list_any)
    .Field("opt_list_list_int", &PyClassForTestObj::opt_list_list_int)
    .Field("opt_dict_any_any", &PyClassForTestObj::opt_dict_any_any)
    .Field("opt_dict_str_any", &PyClassForTestObj::opt_dict_str_any)
    .Field("opt_dict_any_str", &PyClassForTestObj::opt_dict_any_str)
    .Field("opt_dict_str_list_int", &PyClassForTestObj::opt_dict_str_list_int)
    .StaticFn("__init__", ::mlc::InitOf<PyClassForTestObj, int64_t, double, void*, DLDataType, DLDevice, ::mlc::Any, ::mlc::Func, ::mlc::List<::mlc::Any>, ::mlc::Dict<::mlc::Any, ::mlc::Any>, ::mlc::Str, ::mlc::Str, ::mlc::List<::mlc::Any>, ::mlc::List<::mlc::List<int64_t>>, ::mlc::Dict<::mlc::Any, ::mlc::Any>, ::mlc::Dict<::mlc::Str, ::mlc::Any>, ::mlc::Dict<::mlc::Any, ::mlc::Str>, ::mlc::Dict<::mlc::Str, ::mlc::List<int64_t>>, ::mlc::Optional<int64_t>, ::mlc::Optional<double>, ::mlc::Optional<void*>, ::mlc::Optional<DLDataType>, ::mlc::Optional<DLDevice>, ::mlc::Optional<::mlc::Func>, ::mlc::Optional<::mlc::List<::mlc::Any>>, ::mlc::Optional<::mlc::Dict<::mlc::Any, ::mlc::Any>>, ::mlc::Optional<::mlc::Str>, ::mlc::Optional<::mlc::List<::mlc::Any>>, ::mlc::Optional<::mlc::List<::mlc::List<int64_t>>>, ::mlc::Optional<::mlc::Dict<::mlc::Any, ::mlc::Any>>, ::mlc::Optional<::mlc::Dict<::mlc::Str, ::mlc::Any>>, ::mlc::Optional<::mlc::Dict<::mlc::Any, ::mlc::Str>>, ::mlc::Optional<::mlc::Dict<::mlc::Str, ::mlc::List<int64_t>>>>);
};  // struct PyClassForTest
}  // namespace test_dataclasses_prototype
""".strip()
    actual = mlcd.prototype_cxx(PyClassForTest).strip()
    assert actual == expected
