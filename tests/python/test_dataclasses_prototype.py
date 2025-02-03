import mlc.dataclasses as mlcd
from mlc.testing.dataclasses import PyClassForTest


def test_prototype_py() -> None:
    expected = """
@mlc.dataclasses.c_class('mlc.testing.py_class')
class py_class:
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
  udict: dict[Any, Any]
  str_: str
  str_readonly: str
  list_any: list[Any]
  list_list_int: list[list[int]]
  dict_any_any: dict[Any, Any]
  dict_str_any: dict[str, Any]
  dict_any_str: dict[Any, str]
  dict_str_list_int: dict[str, list[int]]
  opt_bool: bool | None
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
    actual = mlcd.prototype(PyClassForTest, lang="py").strip()
    assert actual == expected


def test_prototype_cxx() -> None:
    expected = """
namespace mlc {
namespace testing {
struct py_classObj {
  MLCAny _mlc_header;
  bool bool_;
  int64_t i8;
  int64_t i16;
  int64_t i32;
  int64_t i64;
  double f32;
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
  ::mlc::Optional<bool> opt_bool;
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
  explicit py_classObj(bool bool_, int64_t i8, int64_t i16, int64_t i32, int64_t i64, double f32, double f64, void* raw_ptr, DLDataType dtype, DLDevice device, ::mlc::Any any, ::mlc::Func func, ::mlc::List<::mlc::Any> ulist, ::mlc::Dict<::mlc::Any, ::mlc::Any> udict, ::mlc::Str str_, ::mlc::Str str_readonly, ::mlc::List<::mlc::Any> list_any, ::mlc::List<::mlc::List<int64_t>> list_list_int, ::mlc::Dict<::mlc::Any, ::mlc::Any> dict_any_any, ::mlc::Dict<::mlc::Str, ::mlc::Any> dict_str_any, ::mlc::Dict<::mlc::Any, ::mlc::Str> dict_any_str, ::mlc::Dict<::mlc::Str, ::mlc::List<int64_t>> dict_str_list_int, ::mlc::Optional<bool> opt_bool, ::mlc::Optional<int64_t> opt_i64, ::mlc::Optional<double> opt_f64, ::mlc::Optional<void*> opt_raw_ptr, ::mlc::Optional<DLDataType> opt_dtype, ::mlc::Optional<DLDevice> opt_device, ::mlc::Optional<::mlc::Func> opt_func, ::mlc::Optional<::mlc::List<::mlc::Any>> opt_ulist, ::mlc::Optional<::mlc::Dict<::mlc::Any, ::mlc::Any>> opt_udict, ::mlc::Optional<::mlc::Str> opt_str, ::mlc::Optional<::mlc::List<::mlc::Any>> opt_list_any, ::mlc::Optional<::mlc::List<::mlc::List<int64_t>>> opt_list_list_int, ::mlc::Optional<::mlc::Dict<::mlc::Any, ::mlc::Any>> opt_dict_any_any, ::mlc::Optional<::mlc::Dict<::mlc::Str, ::mlc::Any>> opt_dict_str_any, ::mlc::Optional<::mlc::Dict<::mlc::Any, ::mlc::Str>> opt_dict_any_str, ::mlc::Optional<::mlc::Dict<::mlc::Str, ::mlc::List<int64_t>>> opt_dict_str_list_int): _mlc_header{}, bool_(bool_), i8(i8), i16(i16), i32(i32), i64(i64), f32(f32), f64(f64), raw_ptr(raw_ptr), dtype(dtype), device(device), any(any), func(func), ulist(ulist), udict(udict), str_(str_), str_readonly(str_readonly), list_any(list_any), list_list_int(list_list_int), dict_any_any(dict_any_any), dict_str_any(dict_str_any), dict_any_str(dict_any_str), dict_str_list_int(dict_str_list_int), opt_bool(opt_bool), opt_i64(opt_i64), opt_f64(opt_f64), opt_raw_ptr(opt_raw_ptr), opt_dtype(opt_dtype), opt_device(opt_device), opt_func(opt_func), opt_ulist(opt_ulist), opt_udict(opt_udict), opt_str(opt_str), opt_list_any(opt_list_any), opt_list_list_int(opt_list_list_int), opt_dict_any_any(opt_dict_any_any), opt_dict_str_any(opt_dict_str_any), opt_dict_any_str(opt_dict_any_str), opt_dict_str_list_int(opt_dict_str_list_int) {}
  MLC_DEF_DYN_TYPE(_EXPORTS, py_classObj, ::mlc::Object, "mlc.testing.py_class");
};  // struct py_classObj

struct py_class : public ::mlc::ObjectRef {
  MLC_DEF_OBJ_REF(_EXPORTS, py_class, py_classObj, ::mlc::ObjectRef)
    .Field("bool_", &py_classObj::bool_)
    .Field("i8", &py_classObj::i8)
    .Field("i16", &py_classObj::i16)
    .Field("i32", &py_classObj::i32)
    .Field("i64", &py_classObj::i64)
    .Field("f32", &py_classObj::f32)
    .Field("f64", &py_classObj::f64)
    .Field("raw_ptr", &py_classObj::raw_ptr)
    .Field("dtype", &py_classObj::dtype)
    .Field("device", &py_classObj::device)
    .Field("any", &py_classObj::any)
    .Field("func", &py_classObj::func)
    .Field("ulist", &py_classObj::ulist)
    .Field("udict", &py_classObj::udict)
    .Field("str_", &py_classObj::str_)
    .Field("str_readonly", &py_classObj::str_readonly)
    .Field("list_any", &py_classObj::list_any)
    .Field("list_list_int", &py_classObj::list_list_int)
    .Field("dict_any_any", &py_classObj::dict_any_any)
    .Field("dict_str_any", &py_classObj::dict_str_any)
    .Field("dict_any_str", &py_classObj::dict_any_str)
    .Field("dict_str_list_int", &py_classObj::dict_str_list_int)
    .Field("opt_bool", &py_classObj::opt_bool)
    .Field("opt_i64", &py_classObj::opt_i64)
    .Field("opt_f64", &py_classObj::opt_f64)
    .Field("opt_raw_ptr", &py_classObj::opt_raw_ptr)
    .Field("opt_dtype", &py_classObj::opt_dtype)
    .Field("opt_device", &py_classObj::opt_device)
    .Field("opt_func", &py_classObj::opt_func)
    .Field("opt_ulist", &py_classObj::opt_ulist)
    .Field("opt_udict", &py_classObj::opt_udict)
    .Field("opt_str", &py_classObj::opt_str)
    .Field("opt_list_any", &py_classObj::opt_list_any)
    .Field("opt_list_list_int", &py_classObj::opt_list_list_int)
    .Field("opt_dict_any_any", &py_classObj::opt_dict_any_any)
    .Field("opt_dict_str_any", &py_classObj::opt_dict_str_any)
    .Field("opt_dict_any_str", &py_classObj::opt_dict_any_str)
    .Field("opt_dict_str_list_int", &py_classObj::opt_dict_str_list_int)
    .StaticFn("__init__", ::mlc::InitOf<py_classObj, bool, int64_t, int64_t, int64_t, int64_t, double, double, void*, DLDataType, DLDevice, ::mlc::Any, ::mlc::Func, ::mlc::List<::mlc::Any>, ::mlc::Dict<::mlc::Any, ::mlc::Any>, ::mlc::Str, ::mlc::Str, ::mlc::List<::mlc::Any>, ::mlc::List<::mlc::List<int64_t>>, ::mlc::Dict<::mlc::Any, ::mlc::Any>, ::mlc::Dict<::mlc::Str, ::mlc::Any>, ::mlc::Dict<::mlc::Any, ::mlc::Str>, ::mlc::Dict<::mlc::Str, ::mlc::List<int64_t>>, ::mlc::Optional<bool>, ::mlc::Optional<int64_t>, ::mlc::Optional<double>, ::mlc::Optional<void*>, ::mlc::Optional<DLDataType>, ::mlc::Optional<DLDevice>, ::mlc::Optional<::mlc::Func>, ::mlc::Optional<::mlc::List<::mlc::Any>>, ::mlc::Optional<::mlc::Dict<::mlc::Any, ::mlc::Any>>, ::mlc::Optional<::mlc::Str>, ::mlc::Optional<::mlc::List<::mlc::Any>>, ::mlc::Optional<::mlc::List<::mlc::List<int64_t>>>, ::mlc::Optional<::mlc::Dict<::mlc::Any, ::mlc::Any>>, ::mlc::Optional<::mlc::Dict<::mlc::Str, ::mlc::Any>>, ::mlc::Optional<::mlc::Dict<::mlc::Any, ::mlc::Str>>, ::mlc::Optional<::mlc::Dict<::mlc::Str, ::mlc::List<int64_t>>>>);
};  // struct py_class
}  // namespace testing
}  // namespace mlc
""".strip()
    actual = mlcd.prototype(PyClassForTest, lang="c++").strip()
    assert actual == expected
