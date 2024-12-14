#include <mlc/all.h>

namespace mlc {
namespace {

/**************** FFI ****************/

MLC_REGISTER_FUNC("mlc.testing.cxx_none").set_body([]() -> void { return; });
MLC_REGISTER_FUNC("mlc.testing.cxx_null").set_body([]() -> void * { return nullptr; });
MLC_REGISTER_FUNC("mlc.testing.cxx_int").set_body([](int x) -> int { return x; });
MLC_REGISTER_FUNC("mlc.testing.cxx_float").set_body([](double x) -> double { return x; });
MLC_REGISTER_FUNC("mlc.testing.cxx_ptr").set_body([](void *x) -> void * { return x; });
MLC_REGISTER_FUNC("mlc.testing.cxx_dtype").set_body([](DLDataType x) { return x; });
MLC_REGISTER_FUNC("mlc.testing.cxx_device").set_body([](DLDevice x) { return x; });
MLC_REGISTER_FUNC("mlc.testing.cxx_raw_str").set_body([](const char *x) { return x; });

/**************** Reflection ****************/

struct TestingCClassObj : public Object {
  int8_t i8;
  int16_t i16;
  int32_t i32;
  int64_t i64;
  float f32;
  double f64;
  void *raw_ptr;
  DLDataType dtype;
  DLDevice device;
  Any any;
  Func func;
  UList ulist;
  UDict udict;
  Str str_;
  Str str_readonly;

  List<Any> list_any;
  List<List<int>> list_list_int;
  Dict<Any, Any> dict_any_any;
  Dict<Str, Any> dict_str_any;
  Dict<Any, Str> dict_any_str;
  Dict<Str, List<int>> dict_str_list_int;

  Optional<int64_t> opt_i64;
  Optional<double> opt_f64;
  Optional<void *> opt_raw_ptr;
  Optional<DLDataType> opt_dtype;
  Optional<DLDevice> opt_device;
  Optional<Func> opt_func;
  Optional<UList> opt_ulist;
  Optional<UDict> opt_udict;
  Optional<Str> opt_str;

  Optional<List<Any>> opt_list_any;
  Optional<List<List<int>>> opt_list_list_int;
  Optional<Dict<Any, Any>> opt_dict_any_any;
  Optional<Dict<Str, Any>> opt_dict_str_any;
  Optional<Dict<Any, Str>> opt_dict_any_str;
  Optional<Dict<Str, List<int>>> opt_dict_str_list_int;

  explicit TestingCClassObj(int8_t i8, int16_t i16, int32_t i32, int64_t i64, float f32, double f64, void *raw_ptr,
                            DLDataType dtype, DLDevice device, Any any, Func func, UList ulist, UDict udict, Str str_,
                            Str str_readonly, List<Any> list_any, List<List<int>> list_list_int,
                            Dict<Any, Any> dict_any_any, Dict<Str, Any> dict_str_any, Dict<Any, Str> dict_any_str,
                            Dict<Str, List<int>> dict_str_list_int, Optional<int64_t> opt_i64, Optional<double> opt_f64,
                            Optional<void *> opt_raw_ptr, Optional<DLDataType> opt_dtype, Optional<DLDevice> opt_device,
                            Optional<Func> opt_func, Optional<UList> opt_ulist, Optional<UDict> opt_udict,
                            Optional<Str> opt_str, Optional<List<Any>> opt_list_any,
                            Optional<List<List<int>>> opt_list_list_int, Optional<Dict<Any, Any>> opt_dict_any_any,
                            Optional<Dict<Str, Any>> opt_dict_str_any, Optional<Dict<Any, Str>> opt_dict_any_str,
                            Optional<Dict<Str, List<int>>> opt_dict_str_list_int)
      : i8(i8), i16(i16), i32(i32), i64(i64), f32(f32), f64(f64), raw_ptr(raw_ptr), dtype(dtype), device(device),
        any(any), func(func), ulist(ulist), udict(udict), str_(str_), str_readonly(str_readonly), list_any(list_any),
        list_list_int(list_list_int), dict_any_any(dict_any_any), dict_str_any(dict_str_any),
        dict_any_str(dict_any_str), dict_str_list_int(dict_str_list_int), opt_i64(opt_i64), opt_f64(opt_f64),
        opt_raw_ptr(opt_raw_ptr), opt_dtype(opt_dtype), opt_device(opt_device), opt_func(opt_func),
        opt_ulist(opt_ulist), opt_udict(opt_udict), opt_str(opt_str), opt_list_any(opt_list_any),
        opt_list_list_int(opt_list_list_int), opt_dict_any_any(opt_dict_any_any), opt_dict_str_any(opt_dict_str_any),
        opt_dict_any_str(opt_dict_any_str), opt_dict_str_list_int(opt_dict_str_list_int) {}

  int64_t i64_plus_one() const { return i64 + 1; }

  MLC_DEF_DYN_TYPE(TestingCClassObj, Object, "mlc.testing.c_class");
};

struct TestingCClass : public ObjectRef {
  MLC_DEF_OBJ_REF(TestingCClass, TestingCClassObj, ObjectRef)
      .Field("i8", &TestingCClassObj::i8)
      .Field("i16", &TestingCClassObj::i16)
      .Field("i32", &TestingCClassObj::i32)
      .Field("i64", &TestingCClassObj::i64)
      .Field("f32", &TestingCClassObj::f32)
      .Field("f64", &TestingCClassObj::f64)
      .Field("raw_ptr", &TestingCClassObj::raw_ptr)
      .Field("dtype", &TestingCClassObj::dtype)
      .Field("device", &TestingCClassObj::device)
      .Field("any", &TestingCClassObj::any)
      .Field("func", &TestingCClassObj::func)
      .Field("ulist", &TestingCClassObj::ulist)
      .Field("udict", &TestingCClassObj::udict)
      .Field("str_", &TestingCClassObj::str_)
      .FieldReadOnly("str_readonly", &TestingCClassObj::str_readonly)
      .Field("list_any", &TestingCClassObj::list_any)
      .Field("list_list_int", &TestingCClassObj::list_list_int)
      .Field("dict_any_any", &TestingCClassObj::dict_any_any)
      .Field("dict_str_any", &TestingCClassObj::dict_str_any)
      .Field("dict_any_str", &TestingCClassObj::dict_any_str)
      .Field("dict_str_list_int", &TestingCClassObj::dict_str_list_int)
      .Field("opt_i64", &TestingCClassObj::opt_i64)
      .Field("opt_f64", &TestingCClassObj::opt_f64)
      .Field("opt_raw_ptr", &TestingCClassObj::opt_raw_ptr)
      .Field("opt_dtype", &TestingCClassObj::opt_dtype)
      .Field("opt_device", &TestingCClassObj::opt_device)
      .Field("opt_func", &TestingCClassObj::opt_func)
      .Field("opt_ulist", &TestingCClassObj::opt_ulist)
      .Field("opt_udict", &TestingCClassObj::opt_udict)
      .Field("opt_str", &TestingCClassObj::opt_str)
      .Field("opt_list_any", &TestingCClassObj::opt_list_any)
      .Field("opt_list_list_int", &TestingCClassObj::opt_list_list_int)
      .Field("opt_dict_any_any", &TestingCClassObj::opt_dict_any_any)
      .Field("opt_dict_str_any", &TestingCClassObj::opt_dict_str_any)
      .Field("opt_dict_any_str", &TestingCClassObj::opt_dict_any_str)
      .Field("opt_dict_str_list_int", &TestingCClassObj::opt_dict_str_list_int)
      .MemFn("i64_plus_one", &TestingCClassObj::i64_plus_one)
      .StaticFn("__init__",
                InitOf<TestingCClassObj, int8_t, int16_t, int32_t, int64_t, float, double, void *, DLDataType, DLDevice,
                       Any, Func, UList, UDict, Str, Str, List<Any>, List<List<int>>, Dict<Any, Any>, Dict<Str, Any>,
                       Dict<Any, Str>, Dict<Str, List<int>>, Optional<int64_t>, Optional<double>, Optional<void *>,
                       Optional<DLDataType>, Optional<DLDevice>, Optional<Func>, Optional<UList>, Optional<UDict>,
                       Optional<Str>, Optional<List<Any>>, Optional<List<List<int>>>, Optional<Dict<Any, Any>>,
                       Optional<Dict<Str, Any>>, Optional<Dict<Any, Str>>, Optional<Dict<Str, List<int>>>>);
};

/**************** Traceback ****************/

MLC_REGISTER_FUNC("mlc.testing.throw_exception_from_c").set_body([]() {
  // Simply throw an exception in C++
  MLC_THROW(ValueError) << "This is an error message";
});

MLC_REGISTER_FUNC("mlc.testing.throw_exception_from_ffi_in_c").set_body([](FuncObj *func) {
  // call a Python function which throws an exception
  (*func)();
});

MLC_REGISTER_FUNC("mlc.testing.throw_exception_from_ffi").set_body([](FuncObj *func) {
  // Call a Python function `func` which throws an exception and returns it
  Any ret;
  try {
    (*func)();
  } catch (Exception &error) {
    ret = std::move(error.data_);
  }
  return ret;
});

/**************** Type checking ****************/

MLC_REGISTER_FUNC("mlc.testing.nested_type_checking_list").set_body([](Str name) {
  if (name == "list") {
    using Type = UList;
    return Func([](Type v) { return v; });
  }
  if (name == "list[Any]") {
    using Type = List<Any>;
    return Func([](Type v) { return v; });
  }
  if (name == "list[list[int]]") {
    using Type = List<List<int>>;
    return Func([](Type v) { return v; });
  }
  if (name == "dict") {
    using Type = UDict;
    return Func([](Type v) { return v; });
  }
  if (name == "dict[str, Any]") {
    using Type = Dict<Str, Any>;
    return Func([](Type v) { return v; });
  }
  if (name == "dict[Any, str]") {
    using Type = Dict<Any, Str>;
    return Func([](Type v) { return v; });
  }
  if (name == "dict[Any, Any]") {
    using Type = Dict<Any, Any>;
    return Func([](Type v) { return v; });
  }
  if (name == "dict[str, list[int]]") {
    using Type = Dict<Str, List<int>>;
    return Func([](Type v) { return v; });
  }
  MLC_UNREACHABLE();
});

} // namespace
} // namespace mlc
