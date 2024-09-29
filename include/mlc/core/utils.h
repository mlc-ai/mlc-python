#ifndef MLC_CORE_UTILS_H_
#define MLC_CORE_UTILS_H_

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

#define MLC_DEF_TYPE_COMMON_(SelfType, ParentType, TypeIndex, TypeKey)                                                 \
public:                                                                                                                \
  template <typename, typename> friend struct ::mlc::base::DefaultObjectAllocator;                                     \
  template <typename> friend struct ::mlc::Ref;                                                                        \
  friend struct ::mlc::Any;                                                                                            \
  friend struct ::mlc::AnyView;                                                                                        \
  template <typename DerivedType> MLC_INLINE bool IsInstance() const {                                                 \
    return ::mlc::base::IsInstanceOf<DerivedType, SelfType>(reinterpret_cast<const MLCAny *>(this));                   \
  }                                                                                                                    \
  MLC_INLINE const char *GetTypeKey() const {                                                                          \
    return ::mlc::base::TypeIndex2TypeKey(reinterpret_cast<const MLCAny *>(this));                                     \
  }                                                                                                                    \
  [[maybe_unused]] static constexpr const char *_type_key = TypeKey;                                                   \
  [[maybe_unused]] static inline MLCTypeInfo *_type_info =                                                             \
      ::mlc::base::TypeRegister(static_cast<int32_t>(ParentType::_type_index), /**/                                    \
                                static_cast<int32_t>(TypeIndex), TypeKey,      /**/                                    \
                                &::mlc::core::ObjPtrGetter<SelfType>,          /**/                                    \
                                &::mlc::core::ObjPtrSetter<SelfType>);                                                 \
  [[maybe_unused]] static inline int32_t *_type_ancestors = _type_info->type_ancestors;                                \
  [[maybe_unused]] static constexpr int32_t _type_depth = ParentType::_type_depth + 1;                                 \
  using _type_parent [[maybe_unused]] = ParentType;                                                                    \
  static_assert(sizeof(::mlc::Ref<SelfType>) == sizeof(MLCObjPtr), "Size mismatch")

#define MLC_DEF_STATIC_TYPE(SelfType, ParentType, TypeIndex, TypeKey)                                                  \
public:                                                                                                                \
  MLC_DEF_TYPE_COMMON_(SelfType, ParentType, TypeIndex, TypeKey);                                                      \
  [[maybe_unused]] static constexpr int32_t _type_index = static_cast<int32_t>(TypeIndex);                             \
  MLC_DEF_REFLECTION(SelfType)

#define MLC_DEF_DYN_TYPE(SelfType, ParentType, TypeKey)                                                                \
public:                                                                                                                \
  MLC_DEF_TYPE_COMMON_(SelfType, ParentType, -1, TypeKey);                                                             \
  [[maybe_unused]] static inline int32_t _type_index = _type_info->type_index;                                         \
  MLC_DEF_REFLECTION(SelfType)

namespace mlc {

struct Exception : public std::exception {
  Exception(Ref<ErrorObj> data) : data_(data) {}
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

  Ref<Object> data_;
};

} // namespace mlc

namespace mlc {
namespace core {

template <typename SelfType> MLC_INLINE int32_t ObjPtrGetter(MLCTypeField *, void *addr, MLCAny *ret) {
  using RefT = Ref<SelfType>;
  ::mlc::base::PtrToAnyView<SelfType>(static_cast<RefT *>(addr)->get(), ret);
  return 0;
}

template <typename SelfType> MLC_INLINE int32_t ObjPtrSetter(MLCTypeField *, void *addr, MLCAny *src) {
  using RefT = Ref<SelfType>;
  static_assert(sizeof(RefT) == sizeof(MLCObjPtr), "Size mismatch");
  try {
    *static_cast<RefT *>(addr) = ::mlc::base::ObjPtrTraits<SelfType>::AnyToOwnedPtr(src);
  } catch (const ::mlc::base::TemporaryTypeError &) {
    std::ostringstream oss;
    oss << "Cannot convert from type `" << ::mlc::base::TypeIndex2TypeKey(src->type_index) << "` to `"
        << SelfType::_type_key << " *`";
    *static_cast<::mlc::Any *>(src) = MLC_MAKE_ERROR_HERE(TypeError, oss.str());
    return -2;
  }
  return 0;
}

template <typename FieldType> struct ReflectGetterSetter {
  static int32_t Getter(MLCTypeField *, void *addr, MLCAny *ret) {
    MLC_SAFE_CALL_BEGIN();
    *static_cast<Any *>(ret) = *static_cast<FieldType *>(addr);
    MLC_SAFE_CALL_END(static_cast<Any *>(ret));
  }
  static int32_t Setter(MLCTypeField *, void *addr, MLCAny *src) {
    MLC_SAFE_CALL_BEGIN();
    *static_cast<FieldType *>(addr) = (static_cast<Any *>(src))->operator FieldType();
    MLC_SAFE_CALL_END(static_cast<Any *>(src));
  }
};

template <> struct ReflectGetterSetter<char *> : public ReflectGetterSetter<const char *> {};

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
