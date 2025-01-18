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
#include <mlc/printer/all.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace mlc { // C++ APIs
Any JSONLoads(AnyView json_str);
Any JSONDeserialize(AnyView json_str);
Str JSONSerialize(AnyView source);
bool StructuralEqual(AnyView lhs, AnyView rhs, bool bind_free_vars, bool assert_mode);
int64_t StructuralHash(AnyView root);
Any CopyShallow(AnyView root);
Any CopyDeep(AnyView root);
Str DocToPythonScript(mlc::printer::Node node, mlc::printer::PrinterConfig cfg);
} // namespace mlc

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
  std::unordered_map<std::string, DLDataType> dtype_presets{
      {"void", {kDLOpaqueHandle, 0, 0}},
      {"bool", {kDLUInt, 1, 1}},
      {"int2", {kDLInt, 2, 1}},
      {"int4", {kDLInt, 4, 1}},
      {"int8", {kDLInt, 8, 1}},
      {"int16", {kDLInt, 16, 1}},
      {"int32", {kDLInt, 32, 1}},
      {"int64", {kDLInt, 64, 1}},
      {"uint2", {kDLUInt, 2, 1}},
      {"uint4", {kDLUInt, 4, 1}},
      {"uint8", {kDLUInt, 8, 1}},
      {"uint16", {kDLUInt, 16, 1}},
      {"uint32", {kDLUInt, 32, 1}},
      {"uint64", {kDLUInt, 64, 1}},
      {"float16", {kDLFloat, 16, 1}},
      {"float32", {kDLFloat, 32, 1}},
      {"float64", {kDLFloat, 64, 1}},
      // bfloat16
      {"bfloat16", {kDLBfloat, 16, 1}},
      // 8-bit floating point representations
      {"float8_e3m4", {kDLDataTypeFloat8E3M4, 8, 1}},
      {"float8_e4m3", {kDLDataTypeFloat8E4M3, 8, 1}},
      {"float8_e4m3b11fnuz", {kDLDataTypeFloat8E4M3B11FNUZ, 8, 1}},
      {"float8_e4m3fn", {kDLDataTypeFloat8E4M3FN, 8, 1}},
      {"float8_e4m3fnuz", {kDLDataTypeFloat8E4M3FNUZ, 8, 1}},
      {"float8_e5m2", {kDLDataTypeFloat8E5M2, 8, 1}},
      {"float8_e5m2fnuz", {kDLDataTypeFloat8E5M2FNUZ, 8, 1}},
      {"float8_e8m0fnu", {kDLDataTypeFloat8E8M0FNU, 8, 1}},
      // Microscaling (MX) sub-byte floating point representations
      {"float4_e2m1fn", {kDLDataTypeFloat4E2M1FN, 4, 1}}, // higher 4 bits are unused
      {"float6_e2m3fn", {kDLDataTypeFloat6E2M3FN, 6, 1}}, // higher 2 bits are unused
      {"float6_e3m2fn", {kDLDataTypeFloat6E3M2FN, 6, 1}}, // higher 2 bits are unused
  };
  std::unordered_map<int32_t, std::string> dtype_code_to_str{
      {kDLInt, "int"},
      {kDLUInt, "uint"},
      {kDLFloat, "float"},
      {kDLOpaqueHandle, "ptr"},
      {kDLBfloat, "bfloat"},
      {kDLComplex, "complex"},
      {kDLBool, "bool"},
      /* 8-bit floating point representations */
      {kDLDataTypeFloat8E3M4, "float8_e3m4"},
      {kDLDataTypeFloat8E4M3, "float8_e4m3"},
      {kDLDataTypeFloat8E4M3B11FNUZ, "float8_e4m3b11fnuz"},
      {kDLDataTypeFloat8E4M3FN, "float8_e4m3fn"},
      {kDLDataTypeFloat8E4M3FNUZ, "float8_e4m3fnuz"},
      {kDLDataTypeFloat8E5M2, "float8_e5m2"},
      {kDLDataTypeFloat8E5M2FNUZ, "float8_e5m2fnuz"},
      {kDLDataTypeFloat8E8M0FNU, "float8_e8m0fnu"},
      /* Microscaling (MX) sub-byte floating point representations */
      {kDLDataTypeFloat4E2M1FN, "float4_e2m1fn"},
      {kDLDataTypeFloat6E2M3FN, "float6_e2m3fn"},
      {kDLDataTypeFloat6E3M2FN, "float6_e3m2fn"},
  };
  std::unordered_map<std::string, int32_t> dtype_code_from_str{
      {"int", kDLInt},
      {"uint", kDLUInt},
      {"float", kDLFloat},
      {"ptr", kDLOpaqueHandle},
      {"bfloat", kDLBfloat},
      {"complex", kDLComplex},
      {"bool", kDLBool},
      /* 8-bit floating point representations */
      {"float8_e3m4", kDLDataTypeFloat8E3M4},
      {"float8_e4m3", kDLDataTypeFloat8E4M3},
      {"float8_e4m3b11fnuz", kDLDataTypeFloat8E4M3B11FNUZ},
      {"float8_e4m3fn", kDLDataTypeFloat8E4M3FN},
      {"float8_e4m3fnuz", kDLDataTypeFloat8E4M3FNUZ},
      {"float8_e5m2", kDLDataTypeFloat8E5M2},
      {"float8_e5m2fnuz", kDLDataTypeFloat8E5M2FNUZ},
      {"float8_e8m0fnu", kDLDataTypeFloat8E8M0FNU},
      /* Microscaling (MX) sub-byte floating point representations */
      {"float4_e2m1fn", kDLDataTypeFloat4E2M1FN},
      {"float6_e2m3fn", kDLDataTypeFloat6E2M3FN},
      {"float6_e3m2fn", kDLDataTypeFloat6E3M2FN},
  };
  std::unordered_map<int32_t, std::string> device_type_code_to_str = {
      {0, "meta"},
      {kDLCPU, "cpu"},
      {kDLCUDA, "cuda"},
      {kDLCUDAHost, "cuda_host"},
      {kDLOpenCL, "opencl"},
      {kDLVulkan, "vulkan"},
      {kDLMetal, "mps"},
      {kDLVPI, "vpi"},
      {kDLROCM, "rocm"},
      {kDLROCMHost, "rocm_host"},
      {kDLExtDev, "ext_dev"},
      {kDLCUDAManaged, "cuda_managed"},
      {kDLOneAPI, "oneapi"},
      {kDLWebGPU, "webgpu"},
      {kDLHexagon, "hexagon"},
      {kDLMAIA, "maia"},
  };
  std::unordered_map<std::string, int32_t> device_type_str_to_code = {
      {"meta", 0},
      {"cpu", kDLCPU},
      {"cuda", kDLCUDA},
      {"cuda_host", kDLCUDAHost},
      {"opencl", kDLOpenCL},
      {"vulkan", kDLVulkan},
      {"mps", kDLMetal},
      {"vpi", kDLVPI},
      {"rocm", kDLROCM},
      {"rocm_host", kDLROCMHost},
      {"ext_dev", kDLExtDev},
      {"cuda_managed", kDLCUDAManaged},
      {"oneapi", kDLOneAPI},
      {"webgpu", kDLWebGPU},
      {"hexagon", kDLHexagon},
      {"maia", kDLMAIA},
      // aliases
      {"llvm", kDLCPU},
      {"nvptx", kDLCUDA},
      {"cl", kDLOpenCL},
      {"sdaccel", kDLOpenCL},
      {"metal", kDLMetal},
  };
  int32_t dtype_code_counter = kMLCExtension_DLDataTypeCode_End;
  int32_t device_type_code_counter = kMLCExtension_DLDeviceType_End;
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

  void SetFunc(const char *name, FuncObj *func, bool allow_override = false) {
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
    this->GetGlobalVTable(method.name)->Set(type_index, reinterpret_cast<FuncObj *>(method.func), 2);
    this->GetTypeInfoWrapper(type_index)->AddMethod(method);
  }

  void LoadDSO(std::string name) {
    auto [it, success] = this->dso_libraries.try_emplace(name, nullptr);
    if (!success) {
      return;
    }
    it->second = std::make_unique<DSOLibrary>(name);
  }

  void *DeviceTypeToStr(int32_t device_type) const {
    auto it = this->device_type_code_to_str.find(device_type);
    const char *ret = it == this->device_type_code_to_str.end() ? "unknown" : it->second.c_str();
    return const_cast<char *>(ret);
  }

  int32_t DeviceTypeFromStr(const char *str) const {
    auto it = this->device_type_str_to_code.find(str);
    return it == this->device_type_str_to_code.end() ? -1 : it->second;
  }

  void *DataTypeCodeToStr(int32_t dtype_code) const {
    auto it = this->dtype_code_to_str.find(dtype_code);
    const char *ret = it == this->dtype_code_to_str.end() ? "unknown" : it->second.c_str();
    return const_cast<char *>(ret);
  }

  DLDataType DataTypeFromStr(const char *str) const {
    constexpr int64_t u16_max = 65535;
    constexpr int64_t u8_max = 255;
    const auto &preset = this->dtype_presets;
    std::string source = str;
    if (auto it = preset.find(source); it != preset.end()) {
      return it->second;
    }
    try {
      int64_t dtype_lanes = 1;
      std::string dtype_str;
      if (size_t x_pos = source.rfind('x'); x_pos != std::string::npos) {
        dtype_str = source.substr(0, x_pos);
        dtype_lanes = ::mlc::base::StrToInt(source, x_pos + 1);
        if (dtype_lanes < 0 || dtype_lanes > u16_max) {
          throw std::runtime_error("Invalid DLDataType");
        }
      } else {
        dtype_str = source;
      }
      if (auto it = preset.find(dtype_str); it != preset.end()) {
        DLDataType dtype = it->second;
        dtype.lanes = static_cast<uint16_t>(dtype_lanes);
        return dtype;
      }
#define MLC_DTYPE_PARSE_(str, prefix, prefix_len, dtype_code)                                                          \
  if (str.length() >= prefix_len && str.compare(0, prefix_len, prefix) == 0) {                                         \
    int64_t dtype_bits = ::mlc::base::StrToInt(str, prefix_len);                                                       \
    if (dtype_bits < 0 || dtype_bits > u8_max) {                                                                       \
      throw std::runtime_error("Invalid DLDataType");                                                                  \
    }                                                                                                                  \
    return {static_cast<uint8_t>(dtype_code), static_cast<uint8_t>(dtype_bits), static_cast<uint16_t>(dtype_lanes)};   \
  }
      MLC_DTYPE_PARSE_(dtype_str, "int", 3, kDLInt)
      MLC_DTYPE_PARSE_(dtype_str, "uint", 4, kDLUInt)
      MLC_DTYPE_PARSE_(dtype_str, "float", 5, kDLFloat)
      MLC_DTYPE_PARSE_(dtype_str, "ptr", 3, kDLOpaqueHandle)
      MLC_DTYPE_PARSE_(dtype_str, "bfloat", 6, kDLBfloat)
      MLC_DTYPE_PARSE_(dtype_str, "complex", 7, kDLComplex)
#undef MLC_DTYPE_PARSE_
    } catch (...) {
    }
    MLC_THROW(ValueError) << "Cannot convert to `dtype` from string: " << source;
    MLC_UNREACHABLE();
  }

  int32_t DataTypeRegister(const char *name, int32_t dtype_bits) {
    int32_t dtype_code = ++this->dtype_code_counter;
    this->dtype_presets[name] = {static_cast<uint8_t>(dtype_code), static_cast<uint8_t>(dtype_bits), 1};
    this->dtype_code_to_str[dtype_code] = name;
    this->dtype_code_from_str[name] = dtype_code;
    return dtype_code;
  }

  int32_t DeviceTypeRegister(const char *name) {
    int32_t device_type = ++this->device_type_code_counter;
    this->device_type_code_to_str[device_type] = name;
    this->device_type_str_to_code[name] = device_type;
    return device_type;
  }
};

inline TypeTable *TypeTable::New() {
#define MLC_TYPE_TABLE_INIT_TYPE_BEGIN(UnderlyingType, Self)                                                           \
  {                                                                                                                    \
    using Traits = TypeTraits<UnderlyingType>;                                                                         \
    Self->TypeRegister(-1, Traits::type_index, Traits::type_str);                                                      \
    auto method_static = [self = Self](const char *name, Func func) {                                                  \
      self->AddMethod(Traits::type_index, MLCTypeMethod{name, reinterpret_cast<MLCFunc *>(func.get()), 1});            \
    };                                                                                                                 \
    auto method_member = [self = Self](const char *name, Func func) {                                                  \
      self->AddMethod(Traits::type_index, MLCTypeMethod{name, reinterpret_cast<MLCFunc *>(func.get()), 0});            \
    };                                                                                                                 \
    (void)method_static;                                                                                               \
    (void)method_member;

#define MLC_TYPE_TABLE_INIT_TYPE_END() }

  using namespace mlc::base;
  TypeTable *self = new TypeTable();
  self->type_table.resize(1024);
  self->type_key_to_info.reserve(1024);
  self->num_types = static_cast<int32_t>(MLCTypeIndex::kMLCDynObjectBegin);
  self->SetFunc("mlc.ffi.LoadDSO", Func([self](std::string name) { self->LoadDSO(name); }).get());
  self->SetFunc("mlc.base.DeviceTypeToStr",
                Func([self](int32_t device_type) { return self->DeviceTypeToStr(device_type); }).get());
  self->SetFunc("mlc.base.DeviceTypeFromStr",
                Func([self](const char *str) { return self->DeviceTypeFromStr(str); }).get());
  self->SetFunc("mlc.base.DataTypeRegister", Func([self](const char *name, int32_t dtype_bits) {
                                               return self->DataTypeRegister(name, dtype_bits);
                                             }).get());
  self->SetFunc("mlc.base.DataTypeCodeToStr",
                Func([self](int32_t dtype_code) { return self->DataTypeCodeToStr(dtype_code); }).get());
  self->SetFunc("mlc.base.DataTypeFromStr", Func([self](const char *str) { return self->DataTypeFromStr(str); }).get());
  self->SetFunc("mlc.base.DeviceTypeRegister",
                Func([self](const char *name) { return self->DeviceTypeRegister(name); }).get());
  self->SetFunc("mlc.core.JSONLoads", Func(::mlc::JSONLoads).get());
  self->SetFunc("mlc.core.JSONSerialize", Func(::mlc::JSONSerialize).get());
  self->SetFunc("mlc.core.JSONDeserialize", Func(::mlc::JSONDeserialize).get());
  self->SetFunc("mlc.core.StructuralEqual", Func(::mlc::StructuralEqual).get());
  self->SetFunc("mlc.core.StructuralHash", Func(::mlc::StructuralHash).get());
  self->SetFunc("mlc.core.CopyShallow", Func(::mlc::CopyShallow).get());
  self->SetFunc("mlc.core.CopyDeep", Func(::mlc::CopyDeep).get());
  self->SetFunc("mlc.printer.DocToPythonScript", Func(::mlc::DocToPythonScript).get());
  self->SetFunc("mlc.printer.ToPython", Func(::mlc::printer::ToPython).get());

  MLC_TYPE_TABLE_INIT_TYPE_BEGIN(std::nullptr_t, self);
  method_member("__str__", &TypeTraits<std::nullptr_t>::__str__);
  MLC_TYPE_TABLE_INIT_TYPE_END()

  MLC_TYPE_TABLE_INIT_TYPE_BEGIN(bool, self);
  method_static("__init__", [](AnyView value) { return value.operator bool(); });
  method_static("__new_ref__",
                [](void *dst, Optional<bool> value) { *reinterpret_cast<Optional<bool> *>(dst) = value; });
  method_member("__str__", &TypeTraits<bool>::__str__);
  MLC_TYPE_TABLE_INIT_TYPE_END()

  MLC_TYPE_TABLE_INIT_TYPE_BEGIN(int64_t, self);
  method_static("__init__", [](AnyView value) { return value.operator int64_t(); });
  method_static("__new_ref__",
                [](void *dst, Optional<int64_t> value) { *reinterpret_cast<Optional<int64_t> *>(dst) = value; });
  method_member("__str__", &TypeTraits<int64_t>::__str__);
  MLC_TYPE_TABLE_INIT_TYPE_END()

  MLC_TYPE_TABLE_INIT_TYPE_BEGIN(double, self);
  method_static("__init__", [](AnyView value) { return value.operator double(); });
  method_static("__new_ref__",
                [](void *dst, Optional<double> value) { *reinterpret_cast<Optional<double> *>(dst) = value; });
  method_member("__str__", &TypeTraits<double>::__str__);
  MLC_TYPE_TABLE_INIT_TYPE_END()

  MLC_TYPE_TABLE_INIT_TYPE_BEGIN(void *, self);
  method_static("__init__", [](AnyView value) { return value.operator void *(); });
  method_static("__new_ref__",
                [](void *dst, Optional<void *> value) { *reinterpret_cast<Optional<void *> *>(dst) = value; });
  method_member("__str__", &TypeTraits<void *>::__str__);
  MLC_TYPE_TABLE_INIT_TYPE_END()

  MLC_TYPE_TABLE_INIT_TYPE_BEGIN(DLDevice, self);
  method_static("__init__", [](AnyView device) { return device.operator DLDevice(); });
  method_static("__new_ref__",
                [](void *dst, Optional<DLDevice> value) { *reinterpret_cast<Optional<DLDevice> *>(dst) = value; });
  method_member("__str__", &TypeTraits<DLDevice>::__str__);
  MLC_TYPE_TABLE_INIT_TYPE_END()

  MLC_TYPE_TABLE_INIT_TYPE_BEGIN(DLDataType, self);
  method_static("__init__", [](AnyView dtype) { return dtype.operator DLDataType(); });
  method_static("__new_ref__",
                [](void *dst, Optional<DLDataType> value) { *reinterpret_cast<Optional<DLDataType> *>(dst) = value; });
  method_member("__str__", &TypeTraits<DLDataType>::__str__);
  MLC_TYPE_TABLE_INIT_TYPE_END()

  MLC_TYPE_TABLE_INIT_TYPE_BEGIN(const char *, self);
  method_member("__str__", &TypeTraits<const char *>::__str__);
  MLC_TYPE_TABLE_INIT_TYPE_END()

  return self;
}
#undef MLC_TYPE_TABLE_INIT_TYPE_BEGIN
#undef MLC_TYPE_TABLE_INIT_TYPE_END

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
