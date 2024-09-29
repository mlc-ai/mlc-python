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
  std::string x_mutable;
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
