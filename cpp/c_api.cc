#include "./registry.h"
#include <mlc/core/all.h>

namespace mlc {
namespace registry {
TypeTable *TypeTable::Global() {
  static TypeTable *instance = TypeTable::New();
  return instance;
}
UDict BuildInfo() {
  static UDict build_info = []() {
    UDict ret;
#ifdef MLC_VERSION_GIT
    ret["VERSION_GIT"] = MLC_VERSION_GIT;
#endif
#ifdef MLC_VERSION_MAJOR
    ret["VERSION_MAJOR"] = MLC_VERSION_MAJOR;
#endif
#ifdef MLC_VERSION_MINOR
    ret["VERSION_MINOR"] = MLC_VERSION_MINOR;
#endif
#ifdef MLC_VERSION_PATCH
    ret["VERSION_PATCH"] = MLC_VERSION_PATCH;
#endif
#ifdef MLC_VERSION_COMMIT_NUM
    ret["VERSION_COMMIT_NUM"] = MLC_VERSION_COMMIT_NUM;
#endif
#ifdef MLC_VERSION_COMMIT_SHA
    ret["VERSION_COMMIT_SHA"] = MLC_VERSION_COMMIT_SHA;
#endif
#ifdef MLC_BUILD_TIME
    ret["BUILD_TIME"] = MLC_BUILD_TIME;
#endif
#ifdef MLC_IS_BIG_ENDIAN
    ret["IS_BIG_ENDIAN"] = MLC_IS_BIG_ENDIAN;
#endif
    return ret;
  }();
  return build_info;
}
} // namespace registry
} // namespace mlc

using ::mlc::Any;
using ::mlc::AnyView;
using ::mlc::ErrorObj;
using ::mlc::FuncObj;
using ::mlc::Ref;
using ::mlc::registry::TypeTable;

namespace {
thread_local Any last_error;
} // namespace

MLC_API MLCAny MLCGetLastError() {
  MLCAny ret = static_cast<MLCAny &>(last_error);
  static_cast<MLCAny &>(last_error) = MLCAny();
  return ret;
}

MLC_API int32_t MLCTypeRegister(MLCTypeTableHandle _self, int32_t parent_type_index, const char *type_key,
                                int32_t type_index, MLCTypeInfo **out_type_info) {
  MLC_SAFE_CALL_BEGIN();
  *out_type_info = TypeTable::Get(_self)->TypeRegister(parent_type_index, type_index, type_key);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCTypeIndex2Info(MLCTypeTableHandle _self, int32_t type_index, MLCTypeInfo **ret) {
  MLC_SAFE_CALL_BEGIN();
  *ret = TypeTable::Get(_self)->GetTypeInfo(type_index);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCTypeKey2Info(MLCTypeTableHandle _self, const char *type_key, MLCTypeInfo **ret) {
  MLC_SAFE_CALL_BEGIN();
  *ret = TypeTable::Get(_self)->GetTypeInfo(type_key);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCTypeRegisterFields(MLCTypeTableHandle _self, int32_t type_index, int64_t num_fields,
                                      MLCTypeField *fields) {
  MLC_SAFE_CALL_BEGIN();
  TypeTable::Get(_self)->SetFields(type_index, num_fields, fields);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCTypeRegisterStructure(MLCTypeTableHandle _self, int32_t type_index, int32_t structure_kind,
                                         int64_t num_sub_structures, int32_t *sub_structure_indices,
                                         int32_t *sub_structure_kinds) {
  MLC_SAFE_CALL_BEGIN();
  TypeTable::Get(_self)->SetStructure(type_index, structure_kind, num_sub_structures, sub_structure_indices,
                                      sub_structure_kinds);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCTypeAddMethod(MLCTypeTableHandle _self, int32_t type_index, MLCTypeMethod method) {
  MLC_SAFE_CALL_BEGIN();
  TypeTable::Get(_self)->AddMethod(type_index, method);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCVTableCreate(MLCTypeTableHandle _self, const char *key, MLCVTableHandle *ret) {
  MLC_SAFE_CALL_BEGIN();
  *ret = new mlc::registry::MLCVTable(TypeTable::Get(_self), key);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCVTableDelete(MLCVTableHandle self) {
  MLC_SAFE_CALL_BEGIN();
  if (self) {
    delete static_cast<mlc::registry::MLCVTable *>(self);
  }
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCVTableGetGlobal(MLCTypeTableHandle _self, const char *key, MLCVTableHandle *ret) {
  MLC_SAFE_CALL_BEGIN();
  *ret = TypeTable::Get(_self)->GetGlobalVTable(key);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCVTableGetFunc(MLCVTableHandle vtable, int32_t type_index, int32_t allow_ancestor, MLCAny *ret) {
  using ::mlc::registry::MLCVTable;
  MLC_SAFE_CALL_BEGIN();
  *static_cast<Any *>(ret) = static_cast<MLCVTable *>(vtable)->GetFunc(type_index, allow_ancestor);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCVTableSetFunc(MLCVTableHandle vtable, int32_t type_index, MLCFunc *func, int32_t override_mode) {
  using ::mlc::registry::MLCVTable;
  MLC_SAFE_CALL_BEGIN();
  static_cast<MLCVTable *>(vtable)->Set(type_index, static_cast<FuncObj *>(func), override_mode);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCVTableCall(MLCVTableHandle vtable, int32_t num_args, MLCAny *args, MLCAny *ret) {
  using ::mlc::registry::MLCVTable;
  MLC_SAFE_CALL_BEGIN();
  static_cast<MLCVTable *>(vtable)->Call(num_args, args, ret);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCDynTypeTypeTableCreate(MLCTypeTableHandle *ret) {
  MLC_SAFE_CALL_BEGIN();
  *ret = TypeTable::New();
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCDynTypeTypeTableDestroy(MLCTypeTableHandle handle) {
  MLC_SAFE_CALL_BEGIN();
  delete static_cast<TypeTable *>(handle);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCAnyIncRef(MLCAny *any) {
  MLC_SAFE_CALL_BEGIN();
  if (!::mlc::base::IsTypeIndexPOD(any->type_index)) {
    ::mlc::base::IncRef(any->v.v_obj);
  }
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCAnyDecRef(MLCAny *any) {
  MLC_SAFE_CALL_BEGIN();
  if (!::mlc::base::IsTypeIndexPOD(any->type_index)) {
    ::mlc::base::DecRef(any->v.v_obj);
  }
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCAnyInplaceViewToOwned(MLCAny *any) {
  MLC_SAFE_CALL_BEGIN();
  MLCAny tmp{};
  static_cast<Any &>(tmp) = static_cast<AnyView &>(*any);
  *any = tmp;
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCFuncSetGlobal(MLCTypeTableHandle _self, const char *name, MLCAny func, int allow_override) {
  MLC_SAFE_CALL_BEGIN();
  TypeTable::Get(_self)->SetFunc(name, static_cast<AnyView &>(func), allow_override);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCFuncGetGlobal(MLCTypeTableHandle _self, const char *name, MLCAny *ret) {
  MLC_SAFE_CALL_BEGIN();
  *static_cast<Any *>(ret) = TypeTable::Get(_self)->GetFunc(name);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCFuncSafeCall(MLCFunc *func, int32_t num_args, MLCAny *args, MLCAny *ret) {
#if defined(__APPLE__)
  if (func->_mlc_header.type_index != MLCTypeIndex::kMLCFunc) {
    std::cout << "func->type_index: " << func->_mlc_header.type_index << std::endl;
  }
#endif
  return func->safe_call(func, num_args, args, ret);
}

MLC_API int32_t MLCFuncCreate(void *self, MLCDeleterType deleter, MLCFuncSafeCallType safe_call, MLCAny *ret) {
  MLC_SAFE_CALL_BEGIN();
  *static_cast<Any *>(ret) = FuncObj::FromForeign(self, deleter, safe_call);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCErrorCreate(const char *kind, int64_t num_bytes, const char *bytes, MLCAny *ret) {
  MLC_SAFE_CALL_BEGIN();
  *static_cast<Any *>(ret) = Ref<ErrorObj>::New(kind, num_bytes, bytes);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCErrorGetInfo(MLCAny error, int32_t *num_strs, const char ***strs) {
  MLC_SAFE_CALL_BEGIN();
  thread_local std::vector<const char *> ret;
  static_cast<AnyView &>(error).operator Ref<ErrorObj>()->GetInfo(&ret);
  *num_strs = static_cast<int32_t>(ret.size());
  *strs = ret.data();
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCExtObjCreate(int32_t num_bytes, int32_t type_index, MLCAny *ret) {
  MLC_SAFE_CALL_BEGIN();
  *static_cast<Any *>(ret) = mlc::AllocExternObject(type_index, num_bytes);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t _MLCExtObjDeleteImpl(void *objptr) {
  MLC_SAFE_CALL_BEGIN();
  ::mlc::core::DeleteExternObject(static_cast<::mlc::Object *>(objptr));
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API void MLCExtObjDelete(void *objptr) {
  if (int32_t error_code = _MLCExtObjDeleteImpl(objptr)) {
    std::cerr << "Error code (" << error_code << ") when deleting external object: " << last_error << std::endl;
    std::abort();
  }
}

MLC_API int32_t MLCHandleGetGlobal(MLCTypeTableHandle *self) {
  MLC_SAFE_CALL_BEGIN();
  *self = TypeTable::Global();
  MLC_SAFE_CALL_END(&last_error);
}

//////////////////////////////////////// Testing ////////////////////////////////////////

namespace mlc {
namespace {

/**************** FFI ****************/

MLC_REGISTER_FUNC("mlc.testing.cxx_none").set_body([]() -> void { return; });
MLC_REGISTER_FUNC("mlc.testing.cxx_null").set_body([]() -> void * { return nullptr; });
MLC_REGISTER_FUNC("mlc.testing.cxx_bool").set_body([](bool x) -> bool { return x; });
MLC_REGISTER_FUNC("mlc.testing.cxx_int").set_body([](int x) -> int { return x; });
MLC_REGISTER_FUNC("mlc.testing.cxx_float").set_body([](double x) -> double { return x; });
MLC_REGISTER_FUNC("mlc.testing.cxx_ptr").set_body([](void *x) -> void * { return x; });
MLC_REGISTER_FUNC("mlc.testing.cxx_dtype").set_body([](DLDataType x) { return x; });
MLC_REGISTER_FUNC("mlc.testing.cxx_device").set_body([](DLDevice x) { return x; });
MLC_REGISTER_FUNC("mlc.testing.cxx_raw_str").set_body([](const char *x) { return x; });
MLC_REGISTER_FUNC("mlc.testing.cxx_obj").set_body([](Object *x) { return x; });

/**************** Reflection ****************/

struct TestingCClassObj : public Object {
  bool bool_;
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

  Optional<bool> opt_bool;
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

  explicit TestingCClassObj(bool bool_, int8_t i8, int16_t i16, int32_t i32, int64_t i64, float f32, double f64,
                            void *raw_ptr, DLDataType dtype, DLDevice device, Any any, Func func, UList ulist,
                            UDict udict, Str str_, Str str_readonly, List<Any> list_any, List<List<int>> list_list_int,
                            Dict<Any, Any> dict_any_any, Dict<Str, Any> dict_str_any, Dict<Any, Str> dict_any_str,
                            Dict<Str, List<int>> dict_str_list_int, Optional<bool> opt_bool, Optional<int64_t> opt_i64,
                            Optional<double> opt_f64, Optional<void *> opt_raw_ptr, Optional<DLDataType> opt_dtype,
                            Optional<DLDevice> opt_device, Optional<Func> opt_func, Optional<UList> opt_ulist,
                            Optional<UDict> opt_udict, Optional<Str> opt_str, Optional<List<Any>> opt_list_any,
                            Optional<List<List<int>>> opt_list_list_int, Optional<Dict<Any, Any>> opt_dict_any_any,
                            Optional<Dict<Str, Any>> opt_dict_str_any, Optional<Dict<Any, Str>> opt_dict_any_str,
                            Optional<Dict<Str, List<int>>> opt_dict_str_list_int)
      : bool_(bool_), i8(i8), i16(i16), i32(i32), i64(i64), f32(f32), f64(f64), raw_ptr(raw_ptr), dtype(dtype),
        device(device), any(any), func(func), ulist(ulist), udict(udict), str_(str_), str_readonly(str_readonly),
        list_any(list_any), list_list_int(list_list_int), dict_any_any(dict_any_any), dict_str_any(dict_str_any),
        dict_any_str(dict_any_str), dict_str_list_int(dict_str_list_int), opt_bool(opt_bool), opt_i64(opt_i64),
        opt_f64(opt_f64), opt_raw_ptr(opt_raw_ptr), opt_dtype(opt_dtype), opt_device(opt_device), opt_func(opt_func),
        opt_ulist(opt_ulist), opt_udict(opt_udict), opt_str(opt_str), opt_list_any(opt_list_any),
        opt_list_list_int(opt_list_list_int), opt_dict_any_any(opt_dict_any_any), opt_dict_str_any(opt_dict_str_any),
        opt_dict_any_str(opt_dict_any_str), opt_dict_str_list_int(opt_dict_str_list_int) {}

  int64_t i64_plus_one() const { return i64 + 1; }

  MLC_DEF_DYN_TYPE(MLC_EXPORTS, TestingCClassObj, Object, "mlc.testing.c_class");
};

struct TestingCClass : public ObjectRef {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, TestingCClass, TestingCClassObj, ObjectRef)
      .Field("bool_", &TestingCClassObj::bool_)
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
      .Field("str_readonly", &TestingCClassObj::str_readonly, /*frozen=*/true)
      .Field("list_any", &TestingCClassObj::list_any)
      .Field("list_list_int", &TestingCClassObj::list_list_int)
      .Field("dict_any_any", &TestingCClassObj::dict_any_any)
      .Field("dict_str_any", &TestingCClassObj::dict_str_any)
      .Field("dict_any_str", &TestingCClassObj::dict_any_str)
      .Field("dict_str_list_int", &TestingCClassObj::dict_str_list_int)
      .Field("opt_bool", &TestingCClassObj::opt_bool)
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
      .StaticFn("__init__", InitOf<TestingCClassObj, bool, int8_t, int16_t, int32_t, int64_t, float, double, void *,
                                   DLDataType, DLDevice, Any, Func, UList, UDict, Str, Str, List<Any>, List<List<int>>,
                                   Dict<Any, Any>, Dict<Str, Any>, Dict<Any, Str>, Dict<Str, List<int>>, Optional<bool>,
                                   Optional<int64_t>, Optional<double>, Optional<void *>, Optional<DLDataType>,
                                   Optional<DLDevice>, Optional<Func>, Optional<UList>, Optional<UDict>, Optional<Str>,
                                   Optional<List<Any>>, Optional<List<List<int>>>, Optional<Dict<Any, Any>>,
                                   Optional<Dict<Str, Any>>, Optional<Dict<Any, Str>>, Optional<Dict<Str, List<int>>>>);
  MLC_DEF_OBJ_REF_FWD_NEW(TestingCClass)
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

/**************** Visitor ****************/

MLC_REGISTER_FUNC("mlc.testing.VisitFields").set_body([](ObjectRef root) {
  struct Visitor {
    void operator()(MLCTypeField *f, const Any *any) { Push("Any", f->name, *any); }
    void operator()(MLCTypeField *f, ObjectRef *obj) { Push("ObjectRef", f->name, *obj); }
    void operator()(MLCTypeField *f, Optional<ObjectRef> *opt) { Push("Optional<ObjectRef>", f->name, *opt); }
    void operator()(MLCTypeField *f, Optional<bool> *opt) { Push("Optional<bool>", f->name, *opt); }
    void operator()(MLCTypeField *f, Optional<int64_t> *opt) { Push("Optional<int64_t>", f->name, *opt); }
    void operator()(MLCTypeField *f, Optional<double> *opt) { Push("Optional<double>", f->name, *opt); }
    void operator()(MLCTypeField *f, Optional<DLDevice> *opt) { Push("Optional<DLDevice>", f->name, *opt); }
    void operator()(MLCTypeField *f, Optional<DLDataType> *opt) { Push("Optional<DLDataType>", f->name, *opt); }
    void operator()(MLCTypeField *f, bool *v) { Push("bool", f->name, *v); }
    void operator()(MLCTypeField *f, int8_t *v) { Push("int8_t", f->name, *v); }
    void operator()(MLCTypeField *f, int16_t *v) { Push("int16_t", f->name, *v); }
    void operator()(MLCTypeField *f, int32_t *v) { Push("int32_t", f->name, *v); }
    void operator()(MLCTypeField *f, int64_t *v) { Push("int64_t", f->name, *v); }
    void operator()(MLCTypeField *f, float *v) { Push("float", f->name, *v); }
    void operator()(MLCTypeField *f, double *v) { Push("double", f->name, *v); }
    void operator()(MLCTypeField *f, DLDataType *v) { Push("DLDataType", f->name, *v); }
    void operator()(MLCTypeField *f, DLDevice *v) { Push("DLDevice", f->name, *v); }
    void operator()(MLCTypeField *f, Optional<void *> *v) { Push("Optional<void *>", f->name, *v); }
    void operator()(MLCTypeField *f, void **v) { Push("void *", f->name, *v); }
    void operator()(MLCTypeField *f, const char **v) { Push("const char *", f->name, *v); }

    void Push(const char *ty, const char *name, Any value) {
      types->push_back(ty);
      names->push_back(name);
      values->push_back(value);
    }
    List<Str> *types;
    List<Str> *names;
    UList *values;
  };
  List<Str> types;
  List<Str> names;
  UList values;
  MLCTypeInfo *info = ::mlc::Lib::GetTypeInfo(root.GetTypeIndex());
  ::mlc::core::VisitFields(root.get(), info, Visitor{&types, &names, &values});
  return UList{types, names, values};
});

struct FieldFoundException : public ::std::exception {};

struct FieldGetter {
  void operator()(MLCTypeField *f, const Any *any) { Check(f->name, any); }
  void operator()(MLCTypeField *f, ObjectRef *obj) { Check(f->name, obj); }
  void operator()(MLCTypeField *f, Optional<ObjectRef> *opt) { Check(f->name, opt); }
  void operator()(MLCTypeField *f, Optional<bool> *opt) { Check(f->name, opt); }
  void operator()(MLCTypeField *f, Optional<int64_t> *opt) { Check(f->name, opt); }
  void operator()(MLCTypeField *f, Optional<double> *opt) { Check(f->name, opt); }
  void operator()(MLCTypeField *f, Optional<DLDevice> *opt) { Check(f->name, opt); }
  void operator()(MLCTypeField *f, Optional<DLDataType> *opt) { Check(f->name, opt); }
  void operator()(MLCTypeField *f, bool *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, int8_t *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, int16_t *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, int32_t *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, int64_t *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, float *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, double *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, DLDataType *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, DLDevice *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, Optional<void *> *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, void **v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, const char **v) { Check(f->name, v); }

  template <typename T> void Check(const char *name, T *v) {
    if (std::strcmp(name, target_name) == 0) {
      *ret = Any(*v);
      throw FieldFoundException();
    }
  }
  const char *target_name;
  Any *ret;
};

struct FieldSetter {
  void operator()(MLCTypeField *f, Any *any) { Check(f->name, any); }
  void operator()(MLCTypeField *f, ObjectRef *obj) { Check(f->name, obj); }
  void operator()(MLCTypeField *f, Optional<ObjectRef> *opt) { Check(f->name, opt); }
  void operator()(MLCTypeField *f, Optional<bool> *opt) { Check(f->name, opt); }
  void operator()(MLCTypeField *f, Optional<int64_t> *opt) { Check(f->name, opt); }
  void operator()(MLCTypeField *f, Optional<double> *opt) { Check(f->name, opt); }
  void operator()(MLCTypeField *f, Optional<DLDevice> *opt) { Check(f->name, opt); }
  void operator()(MLCTypeField *f, Optional<DLDataType> *opt) { Check(f->name, opt); }
  void operator()(MLCTypeField *f, bool *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, int8_t *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, int16_t *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, int32_t *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, int64_t *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, float *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, double *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, DLDataType *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, DLDevice *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, Optional<void *> *v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, void **v) { Check(f->name, v); }
  void operator()(MLCTypeField *f, const char **v) { Check(f->name, v); }

  template <typename T> void Check(const char *name, T *v) {
    if (std::strcmp(name, target_name) == 0) {
      if constexpr (std::is_same_v<T, Any>) {
        *v = src;
      } else {
        *v = src.operator T();
      }
      throw FieldFoundException();
    }
  }
  const char *target_name;
  Any src;
};

MLC_REGISTER_FUNC("mlc.testing.FieldGet").set_body([](ObjectRef root, const char *target_name) {
  Any ret;
  try {
    ::mlc::core::VisitFields(root.get(), nullptr, FieldGetter{target_name, &ret});
  } catch (FieldFoundException &) {
    return ret;
  }
  MLC_THROW(ValueError) << "Field not found: " << target_name;
  MLC_UNREACHABLE();
});

MLC_REGISTER_FUNC("mlc.testing.FieldSet").set_body([](ObjectRef root, const char *target_name, Any src) {
  try {
    ::mlc::core::VisitFields(root.get(), nullptr, FieldSetter{target_name, src});
  } catch (FieldFoundException &) {
    return;
  }
  MLC_THROW(ValueError) << "Field not found: " << target_name;
  MLC_UNREACHABLE();
});

} // namespace
} // namespace mlc
