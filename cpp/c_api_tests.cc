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

struct ReflectionTestObj : public Object {
  Str x_mutable;
  int32_t y_immutable;

  ReflectionTestObj(std::string x, int32_t y) : x_mutable(x), y_immutable(y) {}
  int32_t YPlusOne() { return y_immutable + 1; }

  MLC_DEF_DYN_TYPE(ReflectionTestObj, Object, "mlc.testing.ReflectionTestObj");
};

struct ReflectionTest : public ObjectRef {
  MLC_DEF_OBJ_REF(ReflectionTest, ReflectionTestObj, ObjectRef)
      .Field("x_mutable", &ReflectionTestObj::x_mutable)
      .FieldReadOnly("y_immutable", &ReflectionTestObj::y_immutable)
      .StaticFn("__init__", InitOf<ReflectionTestObj, std::string, int32_t>)
      .MemFn("YPlusOne", &ReflectionTestObj::YPlusOne);
};

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

  explicit TestingCClassObj(int8_t i8, int16_t i16, int32_t i32, int64_t i64, float f32, double f64, void *raw_ptr,
                            DLDataType dtype, DLDevice device, Any any, Func func, UList ulist, UDict udict, Str str_)
      : i8(i8), i16(i16), i32(i32), i64(i64), f32(f32), f64(f64), raw_ptr(raw_ptr), dtype(dtype), device(device),
        any(any), func(func), ulist(ulist), udict(udict), str_(str_) {}

  MLC_DEF_DYN_TYPE(ReflectionTestObj, Object, "mlc.testing.c_class");
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
      .StaticFn("__init__", InitOf<TestingCClassObj, int8_t, int16_t, int32_t, int64_t, float, double, void *,
                                   DLDataType, DLDevice, Any, Func, UList, UDict, Str>);
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
