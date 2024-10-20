#ifndef MLC_CORE_UTILS_H_
#define MLC_CORE_UTILS_H_

#include "./func_traits.h" // IWYU pragma: export
#include <iomanip>
#include <mlc/base/all.h>

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
  explicit ReflectionHelper(int32_t type_index) : type_index(type_index) {}

  template <typename Super, typename FieldType>
  inline ReflectionHelper &FieldReadOnly(const char *name, FieldType Super::*field) {
    MLCTypeField f = this->PrepareField<Super, FieldType>(name, field);
    f.is_read_only = true;
    this->fields.emplace_back(f);
    return *this;
  }

  template <typename Super, typename FieldType>
  inline ReflectionHelper &Field(const char *name, FieldType Super::*field) {
    MLCTypeField f = this->PrepareField<Super, FieldType>(name, field);
    this->fields.emplace_back(f);
    return *this;
  }

  template <typename Callable> inline ReflectionHelper &MemFn(const char *name, Callable &&method) {
    constexpr int32_t MemFn = 0;
    MLCTypeMethod m = this->PrepareMethod(name, std::forward<Callable>(method));
    m.kind = MemFn;
    this->methods.emplace_back(m);
    return *this;
  }

  template <typename Callable> inline ReflectionHelper &StaticFn(const char *name, Callable &&method) {
    constexpr int32_t StaticFn = 1;
    MLCTypeMethod m = this->PrepareMethod(name, std::forward<Callable>(method));
    m.kind = StaticFn;
    this->methods.emplace_back(m);
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
      MLCTypeDefReflection(nullptr, this->type_index, this->fields.size(), this->fields.data(), this->methods.size(),
                           this->methods.data());
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

  template <typename Super, typename FieldType>
  inline MLCTypeField PrepareField(const char *name, FieldType Super::*field) {
    int64_t field_offset = static_cast<int64_t>(ReflectOffset(field));
    int32_t num_bytes = static_cast<int32_t>(sizeof(FieldType));
    int32_t is_read_only = false;
    Any ty = ParseType<FieldType>();
    this->any_pool.push_back(ty);
    return MLCTypeField{name, field_offset, num_bytes, is_read_only, ty.v.v_obj};
  }

  template <typename Callable> inline MLCTypeMethod PrepareMethod(const char *name, Callable &&method) {
    Any func = CallableToAny(std::forward<Callable>(method));
    this->any_pool.push_back(func);
    return MLCTypeMethod{name, reinterpret_cast<MLCFunc *>(func.v.v_obj), -1};
  }

  int32_t type_index;
  std::vector<MLCTypeField> fields = {};
  std::vector<MLCTypeMethod> methods = {};
  std::vector<Any> any_pool = {};
};

} // namespace core
} // namespace mlc

#endif // MLC_CORE_UTILS_H_
