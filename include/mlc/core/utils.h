#ifndef MLC_CORE_UTILS_H_
#define MLC_CORE_UTILS_H_

#include "./func_traits.h" // IWYU pragma: export
#include <iomanip>
#include <mlc/base/all.h>
#include <type_traits>
#include <vector>

#define MLC_SAFE_CALL_BEGIN()                                                                                          \
  try {                                                                                                                \
  (void)0

#define MLC_SAFE_CALL_END(err_ret)                                                                                     \
  return 0;                                                                                                            \
  }                                                                                                                    \
  catch (::mlc::Exception & err) {                                                                                     \
    *(err_ret) = std::move(err.data_);                                                                                 \
    return -2;                                                                                                         \
  }                                                                                                                    \
  catch (std::exception & err) {                                                                                       \
    *(err_ret) = err.what();                                                                                           \
    return -1;                                                                                                         \
  }                                                                                                                    \
  MLC_UNREACHABLE()

namespace mlc {
enum class StructureKind : int32_t {
  kNone = 0,
  kNoBind = 1,
  kBind = 2,
  kVar = 3,
};
enum class StructureFieldKind : int32_t {
  kNoBind = 0,
  kBind = 1,
};
namespace core {
namespace typing {
struct Type;
} // namespace typing
template <typename T> typing::Type ParseType();
} // namespace core
} // namespace mlc

/********** Section 1. Exception *********/

namespace mlc {
struct Exception : public std::exception {
  Exception(Ref<ErrorObj> data);
  Exception(const Exception &other) : data_(other.data_) {}
  Exception(Exception &&other) : data_(std::move(other.data_)) {}
  Exception &operator=(const Exception &other) {
    this->data_ = other.data_;
    return *this;
  }
  Exception &operator=(Exception &&other) {
    this->data_ = std::move(other.data_);
    return *this;
  }
  const ErrorObj *Obj() const { return reinterpret_cast<const ErrorObj *>(data_.get()); }
  const char *what() const noexcept(true) override;
  void FormatExc(std::ostream &os) const;

  Ref<Object> data_;
};
} // namespace mlc

namespace mlc {
namespace core {

template <typename Callable> Any CallableToAny(Callable &&callable); // TODO: move to mlc/base?

/********** Section 2. Nested type checking *********/

struct NestedTypeError : public std::runtime_error {
  explicit NestedTypeError(const char *msg) : std::runtime_error(msg) {}

  struct Frame {
    std::string expected_type;
    std::vector<AnyView> indices;
  };

  NestedTypeError &NewFrame(std::string expected_type) {
    frames.push_back(Frame{expected_type, {}});
    return *this;
  }

  NestedTypeError &NewIndex(AnyView index) {
    frames.back().indices.push_back(index);
    return *this;
  }

  void Format(std::ostream &os, std::string overall_expected) const {
    int32_t num_frames = static_cast<int32_t>(frames.size());
    if (num_frames == 1) {
      os << "Let input be `A: " << overall_expected //
         << "`. Type mismatch on `A";
      for (auto rit = frames[0].indices.rbegin(); rit != frames[0].indices.rend(); ++rit) {
        os << "[" << *rit << "]";
      }
      os << "`: " << this->what();
      return;
    }
    int32_t last_var = num_frames;
    os << "Let input be `A_0: " << overall_expected << "`";
    for (int32_t frame_id = num_frames - 1; frame_id >= 0; --frame_id) {
      const Frame &frame = frames[frame_id];
      if (frame_id == 0 && frame.indices.empty()) {
        last_var = num_frames - 1;
        break;
      }
      os << ", `A_" << (num_frames - frame_id) //
         << ": " << frame.expected_type        //
         << (frame_id == 0 ? " := A_" : " in A_") << (num_frames - frame_id - 1);
      for (auto rit = frame.indices.rbegin(); rit != frame.indices.rend(); ++rit) {
        os << "[" << *rit << "]";
      }
      if (frame_id > 0) {
        os << ".keys()";
      }
      os << "`";
    }
    os << ". Type mismatch on `A_" << last_var << "`: " << this->what();
  }

  std::vector<Frame> frames;
};

template <typename T> struct NestedTypeCheck {
  MLC_INLINE_NO_MSVC static void Run(const MLCAny &any);
};
template <typename T> struct NestedTypeCheck<List<T>> {
  MLC_INLINE_NO_MSVC static void Run(const MLCAny &any);
};
template <typename K, typename V> struct NestedTypeCheck<Dict<K, V>> {
  MLC_INLINE_NO_MSVC static void Run(const MLCAny &any);
};

/********** Section 3. Reflection *********/

struct ReflectionHelper {
  static constexpr int32_t kMemFn = 0;
  static constexpr int32_t kStaticFn = 1;

  explicit ReflectionHelper(int32_t type_index) : type_index(type_index) {}

  template <typename Cls> inline ReflectionHelper &Init() {
    this->func_any_to_ref = CallableToAny(AnyToRef<Cls>);
    return *this;
  }

  template <typename Cls, typename FieldType>
  inline ReflectionHelper &FieldReadOnly(const char *name, FieldType Cls::*field) {
    MLCTypeField f = this->PrepareField<Cls, FieldType>(name, field);
    f.frozen = true;
    this->fields.emplace_back(f);
    return *this;
  }

  template <typename Cls, typename FieldType> inline ReflectionHelper &Field(const char *name, FieldType Cls::*field) {
    MLCTypeField f = this->PrepareField<Cls, FieldType>(name, field);
    f.frozen = false;
    this->fields.emplace_back(f);
    return *this;
  }

  template <typename Callable> inline ReflectionHelper &MemFn(const char *name, Callable &&method) {
    MLCTypeMethod m = this->PrepareMethod(name, std::forward<Callable>(method));
    m.kind = kMemFn;
    this->methods.emplace_back(m);
    return *this;
  }

  template <typename Callable> inline ReflectionHelper &StaticFn(const char *name, Callable &&method) {
    MLCTypeMethod m = this->PrepareMethod(name, std::forward<Callable>(method));
    m.kind = kStaticFn;
    this->methods.emplace_back(m);
    return *this;
  }

  inline ReflectionHelper &Structure(std::vector<std::string> sub_structures, StructureKind kind) {
    this->structure_kind = kind;
    this->sub_structure_indices.clear();
    this->sub_structure_kinds.clear();
    for (std::string name : sub_structures) {
      int32_t sub_kind = 0;
      if (std::size_t pos = name.find(':'); pos != std::string::npos) {
        std::string kind_name = name.substr(pos + 1);
        name = name.substr(0, pos);
        if (kind_name == "bind") {
          sub_kind = 1;
        } else {
          MLC_THROW(InternalError) << "Unknown sub-structure kind: " << kind_name;
        }
      }
      int32_t index = [&]() -> int32_t {
        for (const auto &entry : this->fields) {
          std::string field_name = entry.name;
          if (field_name == name) {
            return entry.index;
          }
        }
        MLC_THROW(InternalError) << "Field not found: " << name;
        MLC_UNREACHABLE();
      }();
      this->sub_structure_indices.push_back(index);
      this->sub_structure_kinds.push_back(sub_kind);
    }
    return *this;
  }

  inline operator int32_t() {
    if (!this->fields.empty() || !this->methods.empty()) {
      auto has_method = [&](const char *name) {
        for (const auto &entry : this->methods) {
          if (std::strcmp(entry.name, name) == 0) {
            return true;
          }
        }
        return false;
      };
      if (!has_method("__str__")) {
        this->MemFn("__str__", &ReflectionHelper::DefaultStrMethod);
      }
      if (!has_method("__any_to_ref__") && func_any_to_ref.defined()) {
        this->methods.emplace_back(MLCTypeMethod{"__any_to_ref__",                                     //
                                                 reinterpret_cast<MLCFunc *>(func_any_to_ref.v.v_obj), //
                                                 kStaticFn});
      }
      MLCTypeRegisterFields(nullptr, this->type_index, this->fields.size(), this->fields.data());
      MLCTypeRegisterStructure(nullptr, this->type_index, static_cast<int32_t>(this->structure_kind),
                               this->sub_structure_indices.size(), this->sub_structure_indices.data(),
                               this->sub_structure_kinds.data());
      for (const MLCTypeMethod &method : this->methods) {
        MLCTypeAddMethod(nullptr, this->type_index, method);
      }
    }
    return 0;
  }

  static inline std::string DefaultStrMethod(AnyView any) { // TODO: maybe move to private?
    std::ostringstream os;
    os << ::mlc::base::TypeIndex2TypeKey(any.type_index) << "@0x" << std::setfill('0') << std::setw(12) << std::hex
       << (uintptr_t)(any.v.v_ptr);
    return os.str();
  }

private:
  template <typename Cls, typename FieldType> constexpr std::ptrdiff_t ReflectOffset(FieldType Cls::*member) {
    return reinterpret_cast<std::ptrdiff_t>(&((Cls *)(nullptr)->*member));
  }

  template <typename TObj> static Ref<TObj> AnyToRef(AnyView src) { return Ref<TObj>(src); }

  template <typename Cls, typename FieldType>
  inline MLCTypeField PrepareField(const char *name, FieldType Cls::*field) {
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

  template <typename Callable> inline MLCTypeMethod PrepareMethod(const char *name, Callable &&method) {
    Any func = CallableToAny(std::forward<Callable>(method));
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

template <typename TObject>
MLC_INLINE MLCTypeInfo *TypeRegister(int32_t parent_type_index, int32_t type_index, const char *type_key) {
  MLCTypeInfo *info = nullptr;
  MLCTypeRegister(nullptr, parent_type_index, type_key, type_index, &info);
  return info;
}

} // namespace core
} // namespace mlc

#endif // MLC_CORE_UTILS_H_
