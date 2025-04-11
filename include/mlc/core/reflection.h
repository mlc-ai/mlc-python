#ifndef MLC_CORE_REFLECTION_H_
#define MLC_CORE_REFLECTION_H_
#include <mlc/base/all.h>
#include <type_traits>
#include <vector>

namespace mlc {
namespace core {
namespace typing {
struct Type;
} // namespace typing
template <typename T> typing::Type ParseType();

struct _Reflect {
  static constexpr int32_t kMemFn = 0;
  static constexpr int32_t kStaticFn = 1;

  explicit _Reflect(int32_t type_index) : type_index(type_index) {}

  template <typename Cls> _Reflect &Init() {
    this->func_any_to_ref = ::mlc::base::CallableToAny(AnyToRef<Cls>);
    return *this;
  }

  template <typename Cls, typename FieldType>
  _Reflect &Field(const char *name, FieldType Cls::*field, bool frozen = false) {
    MLCTypeField f = this->PrepareField<Cls, FieldType>(name, field);
    f.frozen = frozen;
    this->fields.emplace_back(f);
    return *this;
  }

  _Reflect &_Field(const char *name, int64_t field_offset, int32_t num_bytes, bool frozen, Any ty) {
    this->any_pool.push_back(ty);
    int32_t index = static_cast<int32_t>(this->fields.size());
    this->fields.emplace_back(MLCTypeField{name, index, field_offset, num_bytes, frozen, ty.v.v_obj});
    return *this;
  }

  template <typename Callable> _Reflect &MemFn(const char *name, Callable &&method) {
    MLCTypeMethod m = this->PrepareMethod(name, std::forward<Callable>(method));
    m.kind = kMemFn;
    this->methods.emplace_back(m);
    return *this;
  }

  template <typename Callable> _Reflect &StaticFn(const char *name, Callable &&method) {
    MLCTypeMethod m = this->PrepareMethod(name, std::forward<Callable>(method));
    m.kind = kStaticFn;
    this->methods.emplace_back(m);
    return *this;
  }

  _Reflect &Structure(StructureKind kind, std::vector<std::string> sub_structures) {
    this->structure_kind = kind;
    this->sub_structure_indices.clear();
    this->sub_structure_kinds.clear();
    for (std::string name : sub_structures) {
      int32_t sub_kind = 0;
      // Check if `name` ends with ":bind"
      if (name.size() >= 5 && strncmp(name.c_str() + name.size() - 5, ":bind", 5) == 0) {
        name = name.substr(0, name.size() - 5);
        sub_kind = 1;
      }
      int32_t index = [this](const std::string &name) -> int32_t {
        for (const auto &entry : this->fields)
          if (entry.name == name)
            return entry.index;
        MLC_THROW(InternalError) << "Field not found: " << name;
        MLC_UNREACHABLE();
      }(name);
      this->sub_structure_indices.push_back(index);
      this->sub_structure_kinds.push_back(sub_kind);
    }
    return *this;
  }

  operator int32_t() {
    if (!this->fields.empty() || !this->methods.empty()) {
      auto has_method = [&](const char *name) {
        for (const auto &entry : this->methods) {
          if (std::strcmp(entry.name, name) == 0) {
            return true;
          }
        }
        return false;
      };
      if (!has_method("__any_to_ref__") && func_any_to_ref.defined()) {
        this->methods.emplace_back(MLCTypeMethod{"__any_to_ref__",                                     //
                                                 reinterpret_cast<MLCFunc *>(func_any_to_ref.v.v_obj), //
                                                 kStaticFn});
      }
      MLC_CHECK_ERR(::MLCTypeRegisterFields(nullptr, this->type_index, this->fields.size(), this->fields.data()));
      MLC_CHECK_ERR(::MLCTypeRegisterStructure(nullptr, this->type_index, static_cast<int32_t>(this->structure_kind),
                                               this->sub_structure_indices.size(), this->sub_structure_indices.data(),
                                               this->sub_structure_kinds.data()));
      for (const MLCTypeMethod &method : this->methods) {
        MLC_CHECK_ERR(::MLCTypeAddMethod(nullptr, this->type_index, method));
      }
    }
    return 0;
  }

private:
  template <typename Cls, typename FieldType> constexpr std::ptrdiff_t ReflectOffset(FieldType Cls::*member) {
    return reinterpret_cast<std::ptrdiff_t>(&((Cls *)(nullptr)->*member));
  }

  template <typename TObj> static Ref<TObj> AnyToRef(AnyView src) { return src.operator Ref<TObj>(); }

  template <typename Cls, typename FieldType> MLCTypeField PrepareField(const char *name, FieldType Cls::*field) {
    static_assert(!std::is_same_v<std::decay_t<FieldType>, std::string>, //
                  "Not allowed field type: `std::string "
                  "because it doesn't have a stable ABI");
    int32_t index = static_cast<int32_t>(this->fields.size());
    int64_t field_offset = static_cast<int64_t>(ReflectOffset(field));
    int32_t num_bytes = static_cast<int32_t>(sizeof(FieldType));
    int32_t frozen = false;
    Any ty = ParseType<FieldType>();
    this->any_pool.push_back(ty);
    return MLCTypeField{name, index, field_offset, num_bytes, frozen, ty.v.v_obj};
  }

  template <typename Callable> MLCTypeMethod PrepareMethod(const char *name, Callable &&method) {
    Any func = ::mlc::base::CallableToAny(std::forward<Callable>(method));
    this->any_pool.push_back(func);
    return MLCTypeMethod{name, reinterpret_cast<MLCFunc *>(func.v.v_obj), -1};
  }

  int32_t type_index;
  StructureKind structure_kind = StructureKind::kNone;
  std::vector<int32_t> sub_structure_indices = {};
  std::vector<int32_t> sub_structure_kinds = {};
  Any func_any_to_ref{nullptr};
  std::vector<MLCTypeField> fields = {};
  std::vector<MLCTypeMethod> methods = {};
  std::vector<Any> any_pool = {};
};

struct _NoReflect {
  explicit _NoReflect(int32_t) {}
  template <typename Cls> _NoReflect &Init() { return *this; }
  template <typename Cls, typename FieldType> _NoReflect &Field(const char *, FieldType Cls::*, bool = false) {
    return *this;
  }
  _NoReflect &_Field(const char *, int64_t, int32_t, bool, Any) { return *this; }
  template <typename Callable> _NoReflect &MemFn(const char *, Callable &&) { return *this; }
  template <typename Callable> _NoReflect &StaticFn(const char *, Callable &&) { return *this; }
  _NoReflect &Structure(StructureKind, std::vector<std::string>) { return *this; }
  operator int32_t() { return 0; }
};

template <bool Enable> struct Reflect;
template <> struct Reflect<true> : public _Reflect {
  explicit Reflect(int32_t type_index) : _Reflect(type_index) {}
};
template <> struct Reflect<false> : public _NoReflect {
  explicit Reflect(int32_t type_index) : _NoReflect(type_index) {}
};

inline int32_t _ReflectStaticFn(int32_t type_index, const char *name, Any func) {
  return ::MLCTypeAddMethod(nullptr, type_index,
                            MLCTypeMethod{name, reinterpret_cast<MLCFunc *>(func.v.v_obj), _Reflect::kStaticFn});
}

inline int32_t _ReflectMemFn(int32_t type_index, const char *name, Any func) {
  return ::MLCTypeAddMethod(nullptr, type_index,
                            MLCTypeMethod{name, reinterpret_cast<MLCFunc *>(func.v.v_obj), _Reflect::kMemFn});
}

} // namespace core
} // namespace mlc

#endif // MLC_CORE_REFLECTION_H_
