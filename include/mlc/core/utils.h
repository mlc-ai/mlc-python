#ifndef MLC_CORE_UTILS_H_
#define MLC_CORE_UTILS_H_

#include "./func_traits.h" // IWYU pragma: export
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

struct ReflectionHelper {
  explicit ReflectionHelper(int32_t type_index);
  template <typename Super, typename FieldType>
  ReflectionHelper &FieldReadOnly(const char *name, FieldType Super::*field);
  template <typename Super, typename FieldType> ReflectionHelper &Field(const char *name, FieldType Super::*field);
  template <typename Callable> ReflectionHelper &MemFn(const char *name, Callable &&method);
  template <typename Callable> ReflectionHelper &StaticFn(const char *name, Callable &&method);
  operator int32_t();
  static std::string DefaultStrMethod(AnyView any);

private:
  template <typename Cls, typename FieldType> constexpr std::ptrdiff_t ReflectOffset(FieldType Cls::*member) {
    return reinterpret_cast<std::ptrdiff_t>(&((Cls *)(nullptr)->*member));
  }
  template <typename Super, typename FieldType> MLCTypeField PrepareField(const char *name, FieldType Super::*field);
  template <typename Callable> MLCTypeMethod PrepareMethod(const char *name, Callable &&method);

  int32_t type_index;
  std::vector<MLCTypeField> fields;
  std::vector<MLCTypeMethod> methods;
  std::vector<Any> method_pool;
  std::vector<std::vector<MLCTypeInfo *>> type_annotation_pool;
};

template <typename SelfType> MLC_INLINE int32_t ObjPtrGetter(MLCTypeField *, void *addr, MLCAny *ret) {
  using RefT = Ref<SelfType>;
  const SelfType *v = static_cast<RefT *>(addr)->get();
  if (v == nullptr) {
    ret->type_index = static_cast<int32_t>(MLCTypeIndex::kMLCNone);
    ret->v_obj = nullptr;
  } else {
    ret->type_index = v->_mlc_header.type_index;
    ret->v_obj = const_cast<MLCAny *>(reinterpret_cast<const MLCAny *>(v));
  }
  return 0;
}

template <typename SelfType> MLC_INLINE int32_t ObjPtrSetter(MLCTypeField *, void *addr, MLCAny *src) {
  using namespace ::mlc::base;
  using RefT = Ref<SelfType>;
  static_assert(sizeof(RefT) == sizeof(MLCObjPtr), "Size mismatch");
  try {
    *static_cast<RefT *>(addr) = TypeTraits<SelfType *>::AnyToTypeOwned(src);
  } catch (const TemporaryTypeError &) {
    std::ostringstream oss;
    oss << "Cannot convert from type `" << TypeIndex2TypeKey(src->type_index) << "` to `" << SelfType::_type_key
        << " *`";
    *static_cast<::mlc::Any *>(src) = MLC_MAKE_ERROR_HERE(TypeError, oss.str());
    return -2;
  }
  return 0;
}

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

} // namespace core
} // namespace mlc

#endif // MLC_CORE_UTILS_H_
