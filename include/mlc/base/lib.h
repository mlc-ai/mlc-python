#ifndef MLC_BASE_LIB_H_
#define MLC_BASE_LIB_H_

#include "./utils.h"

namespace mlc {

struct VTable {
  VTable(const VTable &) = delete;
  VTable &operator=(const VTable &) = delete;
  VTable(VTable &&other) noexcept : self(other.self) { other.self = nullptr; }
  VTable &operator=(VTable &&other) noexcept {
    this->Swap(other);
    return *this;
  }
  ~VTable() { MLC_CHECK_ERR(::MLCVTableDelete(self)); }

  template <typename R, typename... Args> R operator()(Args... args) const;
  template <typename Obj> VTable &Set(Func func);

private:
  friend struct Lib;
  VTable(MLCVTableHandle self) : self(self) {}
  void Swap(VTable &other) { std::swap(self, other.self); }
  MLCVTableHandle self;
};

struct Lib {
  static int32_t FuncSetGlobal(const char *name, FuncObj *func, bool allow_override = false);
  static FuncObj *FuncGetGlobal(const char *name, bool allow_missing = false);
  static ::mlc::Str CxxStr(AnyView obj);
  static ::mlc::Str Str(AnyView obj);
  static int64_t StructuralHash(AnyView obj);
  static bool StructuralEqual(AnyView a, AnyView b, bool bind_free_vars = true, bool assert_mode = false);
  static Any IRPrint(AnyView obj, AnyView printer, AnyView path);
  static const char *DeviceTypeToStr(int32_t device_type);
  static int32_t DeviceTypeFromStr(const char *source);
  static void DeviceTypeRegister(const char *name);
  static const char *DataTypeCodeToStr(int32_t dtype_code);
  static DLDataType DataTypeFromStr(const char *source);
  static void DataTypeRegister(const char *name, int32_t dtype_bits);

  static FuncObj *_init(int32_t type_index) { return VTableGetFunc(init, type_index, "__init__"); }
  static VTable MakeVTable(const char *name) {
    MLCVTableHandle vtable = nullptr;
    MLC_CHECK_ERR(::MLCVTableCreate(_lib, name, &vtable));
    return VTable(vtable);
  }
  MLC_INLINE static MLCTypeInfo *GetTypeInfo(int32_t type_index) {
    MLCTypeInfo *type_info = nullptr;
    MLC_CHECK_ERR(::MLCTypeIndex2Info(_lib, type_index, &type_info));
    return type_info;
  }
  MLC_INLINE static MLCTypeInfo *GetTypeInfo(const char *type_key) {
    MLCTypeInfo *type_info = nullptr;
    MLC_CHECK_ERR(::MLCTypeKey2Info(_lib, type_key, &type_info));
    return type_info;
  }
  MLC_INLINE static const char *GetTypeKey(int32_t type_index) {
    if (MLCTypeInfo *type_info = GetTypeInfo(type_index)) {
      return type_info->type_key;
    }
    return "(undefined)";
  }
  MLC_INLINE static const char *GetTypeKey(const MLCAny *self) {
    if (self == nullptr) {
      return "None";
    } else if (MLCTypeInfo *type_info = GetTypeInfo(self->type_index)) {
      return type_info->type_key;
    }
    return "(undefined)";
  }
  MLC_INLINE static int32_t GetTypeIndex(const char *type_key) {
    if (MLCTypeInfo *type_info = GetTypeInfo(type_key)) {
      return type_info->type_index;
    }
    MLC_THROW(TypeError) << "Cannot find type with key: " << type_key;
    MLC_UNREACHABLE();
  }
  MLC_INLINE static MLCTypeInfo *TypeRegister(int32_t parent_type_index, int32_t type_index, const char *type_key) {
    MLCTypeInfo *info = nullptr;
    MLC_CHECK_ERR(::MLCTypeRegister(_lib, parent_type_index, type_key, type_index, &info));
    return info;
  }

private:
  static FuncObj *VTableGetFunc(MLCVTableHandle vtable, int32_t type_index, const char *vtable_name) {
    MLCAny func{};
    MLC_CHECK_ERR(::MLCVTableGetFunc(vtable, type_index, true, &func));
    if (!::mlc::base::IsTypeIndexPOD(func.type_index)) {
      ::mlc::base::DecRef(func.v.v_obj);
    }
    FuncObj *ret = reinterpret_cast<FuncObj *>(func.v.v_obj);
    if (func.type_index == kMLCNone) {
      MLC_THROW(TypeError) << "Function `" << vtable_name << "` for type: " << GetTypeKey(type_index)
                           << " is not defined in the vtable";
    } else if (func.type_index != kMLCFunc) {
      MLC_THROW(TypeError) << "Function `" << vtable_name << "` for type: " << GetTypeKey(type_index)
                           << " is not callable. Its type is " << GetTypeKey(func.type_index);
    }
    return ret;
  }
  static MLCVTableHandle VTableGetGlobal(const char *name) {
    MLCVTableHandle ret = nullptr;
    MLC_CHECK_ERR(::MLCVTableGetGlobal(_lib, name, &ret));
    return ret;
  }
  static MLC_SYMBOL_HIDE inline MLCTypeTableHandle _lib = []() {
    MLCTypeTableHandle ret = nullptr;
    MLC_CHECK_ERR(::MLCHandleGetGlobal(&ret));
    return ret;
  }();
  static MLC_SYMBOL_HIDE inline MLCVTableHandle cxx_str = VTableGetGlobal("__cxx_str__");
  static MLC_SYMBOL_HIDE inline MLCVTableHandle str = VTableGetGlobal("__str__");
  static MLC_SYMBOL_HIDE inline MLCVTableHandle ir_print = VTableGetGlobal("__ir_print__");
  static MLC_SYMBOL_HIDE inline MLCVTableHandle init = VTableGetGlobal("__init__");
};

} // namespace mlc
#endif // MLC_BASE_LIB_H_
