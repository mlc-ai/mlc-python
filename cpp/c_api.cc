#include "./registry.h"
#include "mlc/core/str.h"

namespace mlc {
namespace registry {
TypeTable *TypeTable::Global() {
  static TypeTable *instance = TypeTable::New();
  return instance;
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
MLC_REGISTER_FUNC("mlc.ffi.LoadDSO").set_body([](std::string name) { TypeTable::Get(nullptr)->LoadDSO(name); });
MLC_REGISTER_FUNC("mlc.core.JSONParse").set_body([](AnyView json_str) {
  if (json_str.type_index == kMLCRawStr) {
    return ::mlc::core::ParseJSON(json_str.operator const char *());
  } else {
    ::mlc::Str str = json_str;
    return ::mlc::core::ParseJSON(str);
  }
});
MLC_REGISTER_FUNC("mlc.core.JSONSerialize").set_body(::mlc::core::Serialize);
MLC_REGISTER_FUNC("mlc.core.JSONDeserialize").set_body([](AnyView json_str) {
  if (json_str.type_index == kMLCRawStr) {
    return ::mlc::core::Deserialize(json_str.operator const char *());
  } else {
    return ::mlc::core::Deserialize(json_str.operator ::mlc::Str());
  }
});
} // namespace

MLC_API MLCAny MLCGetLastError() {
  MLCAny ret;
  *static_cast<Any *>(&ret) = std::move(last_error);
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

MLC_API int32_t MLCTypeDefReflection(MLCTypeTableHandle self, int32_t type_index, int64_t num_fields,
                                     MLCTypeField *fields, int64_t num_methods, MLCTypeMethod *methods) {
  MLC_SAFE_CALL_BEGIN();
  TypeTable::Get(self)->TypeDefReflection(type_index, num_fields, fields, num_methods, methods);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCVTableSet(MLCTypeTableHandle _self, int32_t type_index, const char *key, MLCAny *value) {
  MLC_SAFE_CALL_BEGIN();
  TypeTable::Get(_self)->SetVTable(type_index, key, static_cast<AnyView *>(value));
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCVTableGet(MLCTypeTableHandle _self, int32_t type_index, const char *key, MLCAny *value) {
  MLC_SAFE_CALL_BEGIN();
  *static_cast<Any *>(value) = TypeTable::Get(_self)->GetVTable(type_index, key);
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
  ::mlc::core::DeleteExternObject(static_cast<MLCAny *>(objptr));
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API void MLCExtObjDelete(void *objptr) {
  if (int32_t error_code = _MLCExtObjDeleteImpl(objptr)) {
    std::cerr << "Error code (" << error_code << ") when deleting external object: " << last_error << std::endl;
    std::abort();
  }
}
