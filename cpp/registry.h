#ifndef MLC_CPP_REGISTRY_H_
#define MLC_CPP_REGISTRY_H_
#ifdef _MSC_VER
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>
#include <mlc/core/all.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace mlc {
namespace registry {

struct DSOLibrary {
  ~DSOLibrary() { Unload(); }

#ifdef _MSC_VER
  DSOLibrary(std::string name) {
    std::wstring wname(name.begin(), name.end());
    lib_handle_ = LoadLibraryW(wname.c_str());
    if (lib_handle_ == nullptr) {
      MLC_THROW(ValueError) << "Failed to load dynamic shared library " << name;
    }
  }

  void Unload() {
    if (lib_handle_ != nullptr) {
      FreeLibrary(lib_handle_);
      lib_handle_ = nullptr;
    }
  }

  void *GetSymbol_(const char *name) {
    return reinterpret_cast<void *>(GetProcAddress(lib_handle_, (LPCSTR)name)); // NOLINT(*)
  }

  HMODULE lib_handle_{nullptr};
#else
  DSOLibrary(std::string name) {
    lib_handle_ = dlopen(name.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (lib_handle_ == nullptr) {
      MLC_THROW(ValueError) << "Failed to load dynamic shared library " << name << " " << dlerror();
    }
  }

  void Unload() {
    if (lib_handle_ != nullptr) {
      dlclose(lib_handle_);
      lib_handle_ = nullptr;
    }
  }

  void *GetSymbol(const char *name) { return dlsym(lib_handle_, name); }

  void *lib_handle_{nullptr};
#endif
};

struct ResourcePool {
  using ObjPtr = std::unique_ptr<MLCAny, void (*)(MLCAny *)>;

  void AddObj(void *ptr) {
    if (ptr != nullptr) {
      MLCAny *ptr_cast = reinterpret_cast<MLCAny *>(ptr);
      ::mlc::base::IncRef(ptr_cast);
      this->objects.insert({ptr, ObjPtr(ptr_cast, ::mlc::base::DecRef)});
    }
  }

  void DelObj(void *ptr) {
    if (ptr != nullptr) {
      this->objects.erase(this->objects.find(ptr));
    }
  }

  template <typename PODType> PODType *NewPODArray(int64_t size) {
    return static_cast<PODType *>(this->NewPODArrayImpl(size, sizeof(PODType)));
  }

  const char *NewStr(const char *source) {
    if (source == nullptr) {
      return nullptr;
    }
    size_t len = strlen(source);
    char *ptr = this->NewPODArray<char>(len + 1);
    std::memcpy(ptr, source, len + 1);
    return ptr;
  }

  void DelPODArray(const void *ptr) {
    if (ptr != nullptr) {
      this->pod_array.erase(ptr);
    }
  }

private:
  MLC_INLINE void *NewPODArrayImpl(int64_t size, size_t pod_size) {
    if (size == 0) {
      return nullptr;
    }
    ::mlc::base::PODArray owned(static_cast<void *>(std::malloc(size * pod_size)), std::free);
    void *ptr = owned.get();
    auto [it, success] = this->pod_array.emplace(ptr, std::move(owned));
    if (!success) {
      std::cerr << "Array already registered: " << ptr;
      std::abort();
    }
    return ptr;
  }

  std::unordered_map<const void *, ::mlc::base::PODArray> pod_array;
  std::unordered_multimap<const void *, ObjPtr> objects;
};

struct TypeTable;

struct VTable {
  explicit VTable(TypeTable *type_table, std::string name) : type_table(type_table), name(std::move(name)), data() {}
  void Set(int32_t type_index, FuncObj *func, int32_t override_mode);
  FuncObj *GetFunc(int32_t type_index, bool allow_ancestor) const;

private:
  TypeTable *type_table;
  std::string name;
  std::unordered_map<int32_t, FuncObj *> data;
};

struct TypeInfoWrapper {
  MLCTypeInfo info{};
  ResourcePool *pool = nullptr;
  int64_t num_fields = 0;
  std::vector<MLCTypeMethod> methods = {};

  ~TypeInfoWrapper() { this->Reset(); }

  void Reset() {
    if (this->pool) {
      this->pool->DelPODArray(this->info.type_key);
      this->pool->DelPODArray(this->info.type_ancestors);
      this->ResetFields();
      this->ResetMethods();
      this->info.type_key = nullptr;
      this->info.type_ancestors = nullptr;
      this->pool = nullptr;
    }
  }

  void ResetFields() {
    if (this->num_fields > 0) {
      MLCTypeField *&fields = this->info.fields;
      for (int32_t i = 0; i < this->num_fields; i++) {
        this->pool->DelPODArray(fields[i].name);
      }
      this->pool->DelPODArray(fields);
      this->info.fields = nullptr;
      this->num_fields = 0;
    }
  }

  void ResetMethods() {
    if (!this->methods.empty()) {
      for (MLCTypeMethod &method : this->methods) {
        if (method.name != nullptr) {
          this->pool->DelPODArray(method.name);
          this->pool->DelObj(method.func);
        }
      }
      this->info.methods = nullptr;
      this->methods.clear();
    }
  }

  void ResetStructure() {
    if (this->info.sub_structure_indices) {
      this->pool->DelPODArray(this->info.sub_structure_indices);
      this->info.sub_structure_indices = nullptr;
    }
    if (this->info.sub_structure_kinds) {
      this->pool->DelPODArray(this->info.sub_structure_kinds);
      this->info.sub_structure_kinds = nullptr;
    }
  }

  void SetFields(int64_t new_num_fields, MLCTypeField *fields) {
    this->ResetFields();
    this->num_fields = new_num_fields;
    MLCTypeField *dsts = this->info.fields = this->pool->NewPODArray<MLCTypeField>(new_num_fields + 1);
    for (int64_t i = 0; i < num_fields; i++) {
      MLCTypeField *dst = dsts + i;
      *dst = fields[i];
      this->pool->AddObj(dst->ty);
      dst->name = this->pool->NewStr(dst->name);
      if (dst->index != i) {
        MLC_THROW(ValueError) << "Field index mismatch: " << i << " vs " << dst->index;
      }
    }
    dsts[num_fields] = MLCTypeField{};
    std::sort(dsts, dsts + num_fields,
              [](const MLCTypeField &a, const MLCTypeField &b) { return a.offset < b.offset; });
  }

  void AddMethod(MLCTypeMethod method) {
    for (MLCTypeMethod &m : this->methods) {
      if (m.name && std::strcmp(m.name, method.name) == 0) {
        this->pool->DelObj(m.func);
        this->pool->AddObj(reinterpret_cast<FuncObj *>(method.func));
        m.func = method.func;
        m.kind = method.kind;
        return;
      }
    }
    if (this->methods.empty()) {
      this->methods.emplace_back();
    }
    MLCTypeMethod &dst = this->methods.back();
    dst = method;
    dst.name = this->pool->NewStr(dst.name);
    this->pool->AddObj(reinterpret_cast<FuncObj *>(dst.func));
    this->methods.emplace_back();
    this->info.methods = this->methods.data();
  }

  void SetStructure(int32_t structure_kind, int64_t num_sub_structures, int32_t *sub_structure_indices,
                    int32_t *sub_structure_kinds) {
    this->ResetStructure();
    this->info.structure_kind = structure_kind;
    if (num_sub_structures > 0) {
      this->info.sub_structure_indices = this->pool->NewPODArray<int32_t>(num_sub_structures + 1);
      this->info.sub_structure_kinds = this->pool->NewPODArray<int32_t>(num_sub_structures + 1);
      std::memcpy(this->info.sub_structure_indices, sub_structure_indices, num_sub_structures * sizeof(int32_t));
      std::memcpy(this->info.sub_structure_kinds, sub_structure_kinds, num_sub_structures * sizeof(int32_t));
      std::reverse(this->info.sub_structure_indices, this->info.sub_structure_indices + num_sub_structures);
      std::reverse(this->info.sub_structure_kinds, this->info.sub_structure_kinds + num_sub_structures);
      this->info.sub_structure_indices[num_sub_structures] = -1;
      this->info.sub_structure_kinds[num_sub_structures] = -1;
    } else {
      this->info.sub_structure_indices = nullptr;
      this->info.sub_structure_kinds = nullptr;
    }
  }
};

struct TypeTable {
  using ObjPtr = std::unique_ptr<MLCAny, void (*)(MLCAny *)>;

  int32_t num_types;
  std::vector<std::unique_ptr<TypeInfoWrapper>> type_table;
  std::unordered_map<std::string, MLCTypeInfo *> type_key_to_info;
  std::unordered_map<std::string, FuncObj *> global_funcs;
  std::unordered_map<std::string, std::unique_ptr<VTable>> global_vtables;
  std::unordered_map<std::string, std::unique_ptr<DSOLibrary>> dso_libraries;
  ResourcePool pool;

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

  FuncObj *GetVTable(int32_t type_index, const char *attr_key, bool allow_ancestor) {
    if (auto it = this->global_vtables.find(attr_key); it != this->global_vtables.end()) {
      VTable *vtable = it->second.get();
      return vtable->GetFunc(type_index, allow_ancestor);
    } else {
      return nullptr;
    }
  }

  VTable *GetGlobalVTable(const char *name) {
    if (auto it = this->global_vtables.find(name); it != this->global_vtables.end()) {
      return it->second.get();
    } else {
      std::unique_ptr<VTable> &vtable = this->global_vtables[name] = std::make_unique<VTable>(this, name);
      return vtable.get();
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

  MLCTypeInfo *TypeRegister(int32_t parent_type_index, int32_t type_index, const char *type_key) {
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
    info->type_key = this->pool.NewStr(type_key);
    info->type_key_hash = ::mlc::base::StrHash(type_key, std::strlen(type_key));
    info->type_depth = (parent == nullptr) ? 0 : (parent->type_depth + 1);
    info->type_ancestors = this->pool.NewPODArray<int32_t>(info->type_depth);
    if (parent) {
      std::copy(parent->type_ancestors, parent->type_ancestors + parent->type_depth, info->type_ancestors);
      info->type_ancestors[parent->type_depth] = parent_type_index;
    }
    info->fields = nullptr;
    info->methods = nullptr;
    info->structure_kind = 0;
    info->sub_structure_indices = nullptr;
    info->sub_structure_kinds = nullptr;
    wrapper->pool = &this->pool;
    return info;
  }

  void SetFunc(const char *name, FuncObj *func, bool allow_override) {
    auto [it, success] = this->global_funcs.try_emplace(std::string(name), nullptr);
    if (!success && !allow_override) {
      MLC_THROW(KeyError) << "Global function already registered: " << name;
    } else {
      it->second = func;
    }
    this->pool.AddObj(func);
  }

  TypeInfoWrapper *GetTypeInfoWrapper(int32_t type_index) {
    TypeInfoWrapper *wrapper = nullptr;
    try {
      wrapper = this->type_table.at(type_index).get();
    } catch (const std::out_of_range &) {
    }
    if (wrapper == nullptr || wrapper->pool != &this->pool) {
      MLC_THROW(KeyError) << "Type index `" << type_index << "` not registered";
    }
    return wrapper;
  }

  void SetFields(int32_t type_index, int64_t num_fields, MLCTypeField *fields) {
    this->GetTypeInfoWrapper(type_index)->SetFields(num_fields, fields);
  }

  void SetStructure(int32_t type_index, int32_t structure_kind, int64_t num_sub_structures,
                    int32_t *sub_structure_indices, int32_t *sub_structure_kinds) {
    this->GetTypeInfoWrapper(type_index)
        ->SetStructure(structure_kind, num_sub_structures, sub_structure_indices, sub_structure_kinds);
  }

  void AddMethod(int32_t type_index, MLCTypeMethod method) {
    // TODO: check `override_mode`
    this->GetGlobalVTable(method.name)->Set(type_index, reinterpret_cast<FuncObj *>(method.func), 0);
    this->GetTypeInfoWrapper(type_index)->AddMethod(method);
  }

  void LoadDSO(std::string name) {
    auto [it, success] = this->dso_libraries.try_emplace(name, nullptr);
    if (!success) {
      return;
    }
    it->second = std::make_unique<DSOLibrary>(name);
  }
};

struct _POD_REG {
  inline static const int32_t _none = //
      ::mlc::core::ReflectionHelper(static_cast<int32_t>(MLCTypeIndex::kMLCNone))
          .MemFn("__str__", &::mlc::base::TypeTraits<std::nullptr_t>::__str__);
  inline static const int32_t _int = //
      ::mlc::core::ReflectionHelper(static_cast<int32_t>(MLCTypeIndex::kMLCInt))
          .StaticFn("__init__", [](AnyView value) { return value.operator int64_t(); })
          .StaticFn("__new_ref__",
                    [](void *dst, Optional<int64_t> value) { *reinterpret_cast<Optional<int64_t> *>(dst) = value; })
          .MemFn("__str__", &::mlc::base::TypeTraits<int64_t>::__str__);
  inline static const int32_t _float =
      ::mlc::core::ReflectionHelper(static_cast<int32_t>(MLCTypeIndex::kMLCFloat))
          .StaticFn("__new_ref__",
                    [](void *dst, Optional<double> value) { *reinterpret_cast<Optional<double> *>(dst) = value; })
          .MemFn("__str__", &::mlc::base::TypeTraits<double>::__str__);
  inline static const int32_t _ptr =
      ::mlc::core::ReflectionHelper(static_cast<int32_t>(MLCTypeIndex::kMLCPtr))
          .StaticFn("__new_ref__",
                    [](void *dst, Optional<void *> value) { *reinterpret_cast<Optional<void *> *>(dst) = value; })
          .MemFn("__str__", &::mlc::base::TypeTraits<void *>::__str__);
  inline static const int32_t _device =
      ::mlc::core::ReflectionHelper(static_cast<int32_t>(MLCTypeIndex::kMLCDevice))
          .StaticFn("__init__", [](AnyView device) { return device.operator DLDevice(); })
          .StaticFn("__new_ref__",
                    [](void *dst, Optional<DLDevice> value) { *reinterpret_cast<Optional<DLDevice> *>(dst) = value; })
          .MemFn("__str__", &::mlc::base::TypeTraits<DLDevice>::__str__);
  inline static const int32_t _dtype =
      ::mlc::core::ReflectionHelper(static_cast<int32_t>(MLCTypeIndex::kMLCDataType))
          .StaticFn("__init__", [](AnyView dtype) { return dtype.operator DLDataType(); })
          .StaticFn(
              "__new_ref__",
              [](void *dst, Optional<DLDataType> value) { *reinterpret_cast<Optional<DLDataType> *>(dst) = value; })
          .MemFn("__str__", &::mlc::base::TypeTraits<DLDataType>::__str__);
  inline static const int32_t _str = //
      ::mlc::core::ReflectionHelper(static_cast<int32_t>(MLCTypeIndex::kMLCRawStr))
          .MemFn("__str__", &::mlc::base::TypeTraits<const char *>::__str__);
};

inline TypeTable *TypeTable::New() {
  TypeTable *self = new TypeTable();
  self->type_table.resize(1024);
  self->type_key_to_info.reserve(1024);
  self->num_types = static_cast<int32_t>(MLCTypeIndex::kMLCDynObjectBegin);
#define MLC_TYPE_TABLE_INIT_TYPE(UnderlyingType, Self)                                                                 \
  {                                                                                                                    \
    using Traits = ::mlc::base::TypeTraits<UnderlyingType>;                                                            \
    MLCTypeInfo *info = Self->TypeRegister(-1, Traits::type_index, Traits::type_str);                                  \
    (void)info;                                                                                                        \
  }
  MLC_TYPE_TABLE_INIT_TYPE(std::nullptr_t, self);
  MLC_TYPE_TABLE_INIT_TYPE(int64_t, self);
  MLC_TYPE_TABLE_INIT_TYPE(double, self);
  MLC_TYPE_TABLE_INIT_TYPE(void *, self);
  MLC_TYPE_TABLE_INIT_TYPE(DLDevice, self);
  MLC_TYPE_TABLE_INIT_TYPE(DLDataType, self);
  MLC_TYPE_TABLE_INIT_TYPE(const char *, self);
#undef MLC_TYPE_TABLE_INIT_TYPE
  return self;
}

inline void VTable::Set(int32_t type_index, FuncObj *func, int32_t override_mode) {
  auto [it, success] = this->data.try_emplace(type_index, nullptr);
  if (!success) {
    if (override_mode == 0) {
      // No override
      return;
    } else if (override_mode == 1) {
      // Allow override
      this->type_table->pool.DelObj(it->second);
    } else if (override_mode == 2) {
      // TODO: throw exception
      MLCTypeInfo *type_info = this->type_table->GetTypeInfo(type_index);
      if (type_info && !name.empty()) {
        MLC_THROW(KeyError) << "VTable `" << name << "` already registered for type: " << type_info->type_key;
      } else if (type_info && name.empty()) {
        MLC_THROW(KeyError) << "VTable already registered for type: " << type_info->type_key;
      } else if (!type_info && !name.empty()) {
        MLC_THROW(KeyError) << "VTable `" << name << "` already registered for type index: " << type_index;
      } else {
        MLC_THROW(KeyError) << "VTable already registered for type index: " << type_index;
      }
      MLC_UNREACHABLE();
    }
  }
  it->second = func;
  this->type_table->pool.AddObj(func);
}

inline FuncObj *VTable::GetFunc(int32_t type_index, bool allow_ancestor) const {
  if (auto it = this->data.find(type_index); it != this->data.end()) {
    return it->second;
  }
  if (!allow_ancestor) {
    return nullptr;
  }
  MLCTypeInfo *info = this->type_table->GetTypeInfo(type_index);
  for (int32_t i = info->type_depth - 1; i >= 0; i--) {
    type_index = info->type_ancestors[i];
    if (auto it = this->data.find(type_index); it != this->data.end()) {
      return it->second;
    }
  }
  return nullptr;
}

} // namespace registry
} // namespace mlc

#endif // MLC_CPP_REGISTRY_H_
