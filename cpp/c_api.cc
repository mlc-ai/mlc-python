#include "./registry.h"
#include <mlc/core/all.h>

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

MLC_API int32_t MLCVTableGetGlobal(MLCTypeTableHandle _self, const char *key, MLCVTableHandle *ret) {
  MLC_SAFE_CALL_BEGIN();
  *ret = TypeTable::Get(_self)->GetGlobalVTable(key);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCVTableGetFunc(MLCVTableHandle vtable, int32_t type_index, int32_t allow_ancestor, MLCAny *ret) {
  using ::mlc::registry::VTable;
  MLC_SAFE_CALL_BEGIN();
  *static_cast<Any *>(ret) = static_cast<VTable *>(vtable)->GetFunc(type_index, allow_ancestor);
  MLC_SAFE_CALL_END(&last_error);
}

MLC_API int32_t MLCVTableSetFunc(MLCVTableHandle vtable, int32_t type_index, MLCFunc *func, int32_t override_mode) {
  using ::mlc::registry::VTable;
  MLC_SAFE_CALL_BEGIN();
  static_cast<VTable *>(vtable)->Set(type_index, static_cast<FuncObj *>(func), override_mode);
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
