#ifndef MLC_REGISTRY_H_
#define MLC_REGISTRY_H_

#include "./dso_loader.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mlc {
namespace registry {

struct TypeTable;

struct TypeInfoWrapper {
  MLCTypeInfo info{};
  TypeTable *table = nullptr;
  int64_t num_fields = 0;
  int64_t num_methods = 0;

  void Reset();
  void ResetFields();
  void ResetMethods();
  void SetFields(int64_t new_num_fields, MLCTypeField *fields);
  void SetMethods(int64_t new_num_methods, MLCTypeMethod *methods);
  ~TypeInfoWrapper() { this->Reset(); }
};

template <typename T> struct PODGetterSetter {
  static int32_t Getter(void *addr, MLCAny *ret) {
    MLC_SAFE_CALL_BEGIN();
    *static_cast<Any *>(ret) = *static_cast<T *>(addr);
    MLC_SAFE_CALL_END(static_cast<Any *>(ret));
  }
  static int32_t Setter(void *addr, MLCAny *src) {
    MLC_SAFE_CALL_BEGIN();
    *static_cast<T *>(addr) = (static_cast<Any *>(src))->operator T();
    MLC_SAFE_CALL_END(static_cast<Any *>(src));
  }
};

template <> struct PODGetterSetter<std::nullptr_t> {
  static int32_t Getter(void *, MLCAny *ret) {
    MLC_SAFE_CALL_BEGIN();
    *static_cast<Any *>(ret) = nullptr;
    MLC_SAFE_CALL_END(static_cast<Any *>(ret));
  }
  static int32_t Setter(void *addr, MLCAny *src) {
    MLC_SAFE_CALL_BEGIN();
    *static_cast<void **>(addr) = nullptr;
    MLC_SAFE_CALL_END(static_cast<Any *>(src));
  }
};

struct TypeTable {
  using ObjPtr = std::unique_ptr<MLCObject, void (*)(MLCObject *)>;

  int32_t num_types;
  std::vector<std::unique_ptr<TypeInfoWrapper>> type_table;
  std::unordered_map<std::string, MLCTypeInfo *> type_key_to_info;
  std::unordered_map<std::string, std::unordered_map<int32_t, FuncObj *>> vtable;
  std::unordered_map<std::string, FuncObj *> global_funcs;
  std::unordered_map<const void *, ::mlc::base::PODArray> pool_pod_array;
  std::unordered_map<const void *, ObjPtr> pool_obj_ptr;
  std::unordered_map<std::string, std::unique_ptr<DSOLibrary>> dso_library;

  template <typename PODType> inline PODType *NewArray(int64_t size) {
    if (size == 0) {
      return nullptr;
    }
    ::mlc::base::PODArray owned(static_cast<void *>(std::malloc(size * sizeof(PODType))), std::free);
    PODType *ptr = reinterpret_cast<PODType *>(owned.get());
    auto [it, success] = this->pool_pod_array.emplace(ptr, std::move(owned));
    if (!success) {
      std::cerr << "Array already registered: " << owned.get();
      std::abort();
    }
    return ptr;
  }

  inline const char *NewArray(const char *source) {
    if (source == nullptr) {
      return nullptr;
    }
    size_t len = strlen(source);
    char *ptr = this->NewArray<char>(len + 1);
    std::memcpy(ptr, source, len + 1);
    return ptr;
  }

  template <typename T> void NewObjPtr(T **dst, T *source) {
    if (*dst != nullptr) {
      this->pool_obj_ptr.erase(*dst);
      *dst = nullptr;
    }
    *dst = source;
    if (source != nullptr) {
      auto [it, success] = this->pool_obj_ptr.emplace(source, ObjPtr(nullptr, nullptr));
      if (!success) {
        std::cerr << "Object already exists in the memory pool: " << source;
        std::abort();
      }
      MLCObject *source_casted = reinterpret_cast<MLCObject *>(source);
      ::mlc::base::IncRef(source_casted);
      it->second = ObjPtr(source_casted, ::mlc::base::DecRef);
    }
  }

  void DelArray(const void *ptr) {
    if (ptr != nullptr) {
      this->pool_pod_array.erase(ptr);
    }
  }

  MLCTypeInfo *GetTypeInfo(int32_t type_index) {
    if (type_index < 0 || type_index >= static_cast<int32_t>(this->type_table.size())) {
      return nullptr;
    } else {
      return &this->type_table.at(type_index)->info;
    }
  }

  MLCTypeInfo *GetTypeInfo(const char *type_key) {
    auto ret = this->type_key_to_info.find(type_key);
    if (ret == this->type_key_to_info.end()) {
      return nullptr;
    } else {
      return ret->second;
    }
  }

  FuncObj *GetVTable(int32_t type_index, const char *attr_key) {
    std::unordered_map<int32_t, FuncObj *> &attrs = this->vtable[attr_key];
    if (auto it = attrs.find(type_index); it != attrs.end()) {
      return it->second;
    } else {
      return nullptr;
    }
  }

  FuncObj *GetFunc(const char *name) {
    if (auto it = this->global_funcs.find(name); it != this->global_funcs.end()) {
      return it->second;
    } else {
      return nullptr;
    }
  }

  static TypeTable *New();
  static TypeTable *Global();
  static TypeTable *Get(MLCTypeTableHandle self) {
    return (self == nullptr) ? TypeTable::Global() : static_cast<TypeTable *>(self);
  }

  MLCTypeInfo *TypeRegister(int32_t parent_type_index, int32_t type_index, const char *type_key,
                            MLCAttrGetterSetter getter, MLCAttrGetterSetter setter) {
    // Step 1.Check if the type is already registered
    if (auto it = this->type_key_to_info.find(type_key); it != this->type_key_to_info.end()) {
      MLCTypeInfo *ret = it->second;
      if (type_index != -1 && type_index != ret->type_index) {
        MLC_THROW(KeyError) << "Type `" << type_key << "` registered with type index `" << ret->type_index
                            << "`, but re-registered with type index: " << type_index;
      }
      return ret;
    }
    // Step 2. Manipulate type table
    if (type_index == -1) {
      type_index = this->num_types++;
    }
    if (type_index >= static_cast<int32_t>(this->type_table.size())) {
      this->type_table.resize((type_index / 1024 + 1) * 1024);
    }
    std::unique_ptr<TypeInfoWrapper> &wrapper = this->type_table.at(type_index) = std::make_unique<TypeInfoWrapper>();
    this->type_key_to_info[type_key] = &wrapper->info;
    // Step 3. Initialize type info
    MLCTypeInfo *parent = parent_type_index == -1 ? nullptr : this->GetTypeInfo(parent_type_index);
    MLCTypeInfo *info = &wrapper->info;
    info->type_index = type_index;
    info->type_key = this->NewArray(type_key);
    info->type_depth = (parent == nullptr) ? 0 : (parent->type_depth + 1);
    info->getter = getter;
    info->setter = setter;
    info->fields = nullptr;
    info->methods = nullptr;
    info->type_ancestors = this->NewArray<int32_t>(info->type_depth);
    if (parent) {
      std::copy(parent->type_ancestors, parent->type_ancestors + parent->type_depth, info->type_ancestors);
      info->type_ancestors[parent->type_depth] = parent_type_index;
    }
    wrapper->table = this;
    return info;
  }

  void SetFunc(const char *name, AnyView any_func, bool allow_override) {
    auto [it, success] = this->global_funcs.try_emplace(std::string(name), nullptr);
    if (!success && !allow_override) {
      MLC_THROW(KeyError) << "Global function already registered: " << name;
    }
    FuncObj *func = any_func;
    this->NewObjPtr(&it->second, func);
  }

  void SetVTable(int32_t type_index, const char *key, AnyView *value) {
    std::unordered_map<int32_t, FuncObj *> &attrs = this->vtable[key];
    auto [it, success] = attrs.try_emplace(type_index, nullptr);
    (void)success; // Already registered
    FuncObj *func = *value;
    this->NewObjPtr(&it->second, func);
  }

  void TypeDefReflection(int32_t type_index,                       //
                         int64_t num_fields, MLCTypeField *fields, //
                         int64_t num_methods, MLCTypeMethod *methods) {
    TypeInfoWrapper *wrapper = nullptr;
    try {
      wrapper = this->type_table.at(type_index).get();
    } catch (const std::out_of_range &) {
    }
    if (wrapper == nullptr || wrapper->table != this) {
      MLC_THROW(KeyError) << "Type index `" << type_index << "` not registered";
    }
    wrapper->SetFields(num_fields, fields);
    wrapper->SetMethods(num_methods, methods);
  }

  void LoadDSO(std::string name) {
    auto [it, success] = this->dso_library.try_emplace(name, nullptr);
    if (!success) {
      return;
    }
    it->second = std::make_unique<DSOLibrary>(name);
  }
};

struct _POD_REG {
  inline static const int32_t _none = base::ReflectionHelper(static_cast<int32_t>(MLCTypeIndex::kMLCNone))
                                          .Method("__str__", &base::PODTraits<std::nullptr_t>::__str__);
  inline static const int32_t _int = base::ReflectionHelper(static_cast<int32_t>(MLCTypeIndex::kMLCInt))
                                         .Method("__str__", &base::PODTraits<int64_t>::__str__);
  inline static const int32_t _float = base::ReflectionHelper(static_cast<int32_t>(MLCTypeIndex::kMLCFloat))
                                           .Method("__str__", &base::PODTraits<double>::__str__);
  inline static const int32_t _ptr = base::ReflectionHelper(static_cast<int32_t>(MLCTypeIndex::kMLCPtr))
                                         .Method("__str__", &base::PODTraits<void *>::__str__);
  inline static const int32_t _device =
      base::ReflectionHelper(static_cast<int32_t>(MLCTypeIndex::kMLCDevice))
          .Method("__str__", &base::PODTraits<DLDevice>::__str__)
          .Method("__init__", [](AnyView device) { return device.operator DLDevice(); });
  inline static const int32_t _dtype =
      base::ReflectionHelper(static_cast<int32_t>(MLCTypeIndex::kMLCDataType))
          .Method("__str__", &base::PODTraits<DLDataType>::__str__)
          .Method("__init__", [](AnyView dtype) { return dtype.operator DLDataType(); });
  inline static const int32_t _str = base::ReflectionHelper(static_cast<int32_t>(MLCTypeIndex::kMLCRawStr))
                                         .Method("__str__", &base::PODTraits<const char *>::__str__);
};

inline TypeTable *TypeTable::New() {
  TypeTable *self = new TypeTable();
  self->type_table.resize(1024);
  self->type_key_to_info.reserve(1024);
  self->num_types = static_cast<int32_t>(MLCTypeIndex::kMLCDynObjectBegin);
#define MLC_TYPE_TABLE_INIT_TYPE(TypeIndex, UnderlyingType, Self)                                                      \
  Self->TypeRegister(-1, static_cast<int32_t>(TypeIndex), ::mlc::base::PODTraits<UnderlyingType>::Type2Str(),          \
                     PODGetterSetter<UnderlyingType>::Getter, PODGetterSetter<UnderlyingType>::Setter);

  MLC_TYPE_TABLE_INIT_TYPE(MLCTypeIndex::kMLCNone, std::nullptr_t, self);
  MLC_TYPE_TABLE_INIT_TYPE(MLCTypeIndex::kMLCInt, int64_t, self);
  MLC_TYPE_TABLE_INIT_TYPE(MLCTypeIndex::kMLCFloat, double, self);
  MLC_TYPE_TABLE_INIT_TYPE(MLCTypeIndex::kMLCPtr, void *, self);
  MLC_TYPE_TABLE_INIT_TYPE(MLCTypeIndex::kMLCDevice, DLDevice, self);
  MLC_TYPE_TABLE_INIT_TYPE(MLCTypeIndex::kMLCDataType, DLDataType, self);
  MLC_TYPE_TABLE_INIT_TYPE(MLCTypeIndex::kMLCRawStr, const char *, self);
#undef MLC_TYPE_TABLE_INIT_TYPE
  return self;
}

inline void TypeInfoWrapper::Reset() {
  if (this->table) {
    this->table->DelArray(this->info.type_key);
    this->info.type_key = nullptr;
    this->table->DelArray(this->info.type_ancestors);
    this->info.type_ancestors = nullptr;
    this->ResetFields();
    this->ResetMethods();
    this->table = nullptr;
  }
}

inline void TypeInfoWrapper::ResetFields() {
  if (this->num_fields > 0) {
    MLCTypeField *&fields = this->info.fields;
    for (int32_t i = 0; i < this->num_fields; i++) {
      this->table->DelArray(fields[i].name);
    }
    this->table->DelArray(fields);
    this->info.fields = nullptr;
    this->num_fields = 0;
  }
}

inline void TypeInfoWrapper::ResetMethods() {
  if (this->num_methods > 0) {
    MLCTypeMethod *&methods = this->info.methods;
    for (int32_t i = 0; i < this->num_methods; i++) {
      this->table->DelArray(methods[i].name);
      this->table->NewObjPtr<MLCFunc>(&methods[i].func, nullptr);
    }
    this->table->DelArray(methods);
    this->info.methods = nullptr;
    this->num_methods = 0;
  }
}

inline void TypeInfoWrapper::SetFields(int64_t new_num_fields, MLCTypeField *fields) {
  this->ResetFields();
  this->num_fields = new_num_fields;
  MLCTypeField *dst = this->info.fields = this->table->NewArray<MLCTypeField>(new_num_fields + 1);
  for (int64_t i = 0; i < num_fields; i++) {
    dst[i] = fields[i];
    dst[i].name = this->table->NewArray(fields[i].name);
  }
  dst[num_fields] = MLCTypeField{};
  std::sort(dst, dst + num_fields, [](const MLCTypeField &a, const MLCTypeField &b) { return a.offset < b.offset; });
}

inline void TypeInfoWrapper::SetMethods(int64_t new_num_methods, MLCTypeMethod *methods) {
  this->ResetMethods();
  this->num_methods = new_num_methods;
  int32_t type_index = this->info.type_index;
  MLCTypeMethod *dst = this->info.methods = this->table->NewArray<MLCTypeMethod>(new_num_methods + 1);
  for (int64_t i = 0; i < num_methods; i++) {
    dst[i] = methods[i];
    dst[i].name = this->table->NewArray(methods[i].name);
    auto [it, success] = this->table->vtable[dst[i].name].try_emplace(type_index, nullptr);
    (void)success; // Already registered
    this->table->NewObjPtr(&it->second, reinterpret_cast<FuncObj *>(methods[i].func));
  }
  dst[num_methods] = MLCTypeMethod{};
  std::sort(dst, dst + num_methods,
            [](const MLCTypeMethod &a, const MLCTypeMethod &b) { return std::strcmp(a.name, b.name) < 0; });
}

} // namespace registry
} // namespace mlc

#endif // MLC_REGISTRY_H_
