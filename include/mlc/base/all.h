#ifndef MLC_CORE_H_
#define MLC_CORE_H_

#include "./any.h"
#include "./ref.h"
#include "./traits_device.h"
#include "./traits_dtype.h"
#include "./traits_object.h"
#include "./traits_scalar.h"
#include "./traits_str.h"
#include <cstring>
#include <vector>

/*********** AnyView ***********/

namespace mlc {
MLC_INLINE AnyView::AnyView(const Any &src) : MLCAny(static_cast<const MLCAny &>(src)) {}
MLC_INLINE AnyView::AnyView(Any &&src) : MLCAny(static_cast<const MLCAny &>(src)) {}
MLC_INLINE AnyView &AnyView::operator=(const Any &src) {
  *static_cast<MLCAny *>(this) = static_cast<const MLCAny &>(src);
  return *this;
}
MLC_INLINE AnyView &AnyView::operator=(Any &&src) {
  *static_cast<MLCAny *>(this) = static_cast<const MLCAny &>(src);
  return *this;
}
MLC_INLINE AnyView::AnyView(::mlc::base::tag::ObjPtr, const MLCObjPtr &src) : MLCAny() {
  if (src.ptr != nullptr) {
    this->type_index = src.ptr->type_index;
    this->v_obj = src.ptr;
  }
}
MLC_INLINE AnyView::AnyView(::mlc::base::tag::ObjPtr, MLCObjPtr &&src) : MLCAny() {
  if (src.ptr != nullptr) {
    this->type_index = src.ptr->type_index;
    this->v_obj = src.ptr;
  }
}
template <typename T> MLC_INLINE AnyView::AnyView(::mlc::base::tag::POD, const T &src) : MLCAny() {
  ::mlc::base::PODTraits<::mlc::base::RemoveCR<T>>::TypeCopyToAny(src, this);
}
template <typename T> MLC_INLINE AnyView::AnyView(::mlc::base::tag::POD, T &&src) : MLCAny() {
  ::mlc::base::PODTraits<::mlc::base::RemoveCR<T>>::TypeCopyToAny(src, this);
}
template <typename T> MLC_INLINE AnyView::AnyView(::mlc::base::tag::RawObjPtr, T *src) : MLCAny() {
  ::mlc::base::ObjPtrTraits<T>::PtrToAnyView(src, this);
}
template <typename T> MLC_INLINE T AnyView::Cast(::mlc::base::tag::ObjPtr) const {
  if constexpr (::mlc::base::IsObjRef<T>) {
    if (this->type_index == static_cast<int32_t>(MLCTypeIndex::kMLCNone)) {
      using RefT = Ref<typename T::TObj>;
      MLC_THROW(TypeError) << "Cannot convert from type `None` to non-nullable `" << ::mlc::base::Type2Str<RefT>::Run()
                           << "`";
    }
  }
  return T(T::THelper::TryConvert(this));
}
template <typename T> MLC_INLINE T AnyView::Cast() const {
  if constexpr (std::is_same_v<T, Any> || std::is_same_v<T, AnyView>) {
    return *this;
  } else {
    return this->operator T();
  }
}
template <typename T> MLC_INLINE_NO_MSVC T AnyView::Cast(::mlc::base::tag::POD) const {
  MLC_TRY_CONVERT(::mlc::base::PODTraits<::mlc::base::RemoveCR<T>>::AnyCopyToType(this), this->type_index,
                  ::mlc::base::Type2Str<T>::Run());
}
template <typename _T> MLC_INLINE_NO_MSVC _T AnyView::Cast(::mlc::base::tag::RawObjPtr) const {
  using T = std::remove_pointer_t<_T>;
  MLC_TRY_CONVERT(::mlc::base::ObjPtrTraits<::mlc::base::RemoveCR<T>>::AnyToUnownedPtr(this), this->type_index,
                  ::mlc::base::Type2Str<T *>::Run());
}
template <typename T> MLC_INLINE_NO_MSVC T *AnyView::CastWithStorage(Any *storage) const {
  MLC_TRY_CONVERT(::mlc::base::ObjPtrTraits<T>::AnyToOwnedPtrWithStorage(this, storage), this->type_index,
                  ::mlc::base::Type2Str<T *>::Run());
}
} // namespace mlc

/*********** Any ***********/

namespace mlc {
MLC_INLINE Any::Any(::mlc::base::tag::ObjPtr, const MLCObjPtr &src) : MLCAny() {
  if (src.ptr != nullptr) {
    this->type_index = src.ptr->type_index;
    this->v_obj = src.ptr;
    this->IncRef();
  }
}
MLC_INLINE Any::Any(::mlc::base::tag::ObjPtr, MLCObjPtr &&src) : MLCAny() {
  if (src.ptr != nullptr) {
    this->type_index = src.ptr->type_index;
    this->v_obj = src.ptr;
    src.ptr = nullptr;
  }
}
template <typename T> MLC_INLINE Any::Any(::mlc::base::tag::RawObjPtr, T *src) : MLCAny() {
  ::mlc::base::ObjPtrTraits<T>::PtrToAnyView(src, this);
  this->IncRef();
}
template <typename T> MLC_INLINE T Any::Cast() const {
  if constexpr (std::is_same_v<T, Any> || std::is_same_v<T, AnyView>) {
    return *this;
  } else {
    return this->operator T();
  }
}
template <typename T> MLC_INLINE T Any::Cast(::mlc::base::tag::ObjPtr) const {
  if constexpr (::mlc::base::IsObjRef<T>) {
    if (this->type_index == static_cast<int32_t>(MLCTypeIndex::kMLCNone)) {
      using RefT = Ref<typename T::TObj>;
      MLC_THROW(TypeError) << "Cannot convert from type `None` to non-nullable `" << ::mlc::base::Type2Str<RefT>::Run()
                           << "`";
    }
  }
  return T(T::THelper::TryConvert(this));
}
template <typename T> MLC_INLINE_NO_MSVC T Any::Cast(::mlc::base::tag::POD) const {
  MLC_TRY_CONVERT(::mlc::base::PODTraits<::mlc::base::RemoveCR<T>>::AnyCopyToType(this), this->type_index,
                  ::mlc::base::Type2Str<T>::Run());
}
template <typename _T> MLC_INLINE_NO_MSVC _T Any::Cast(::mlc::base::tag::RawObjPtr) const {
  using T = std::remove_pointer_t<_T>;
  MLC_TRY_CONVERT(::mlc::base::ObjPtrTraits<::mlc::base::RemoveCR<T>>::AnyToUnownedPtr(this), this->type_index,
                  ::mlc::base::Type2Str<T *>::Run());
}
template <typename T> MLC_INLINE_NO_MSVC T *Any::CastWithStorage(Any *storage) const {
  MLC_TRY_CONVERT(::mlc::base::ObjPtrTraits<T>::AnyToOwnedPtrWithStorage(this, storage), this->type_index,
                  ::mlc::base::Type2Str<T *>::Run());
}
} // namespace mlc

namespace mlc {
namespace core {

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

namespace mlc {
namespace base {
struct ReflectionHelper {
  explicit ReflectionHelper(int32_t type_index);
  template <typename Super, typename FieldType>
  ReflectionHelper &FieldReadOnly(const char *name, FieldType Super::*field);
  template <typename Super, typename FieldType> ReflectionHelper &Field(const char *name, FieldType Super::*field);
  template <typename Callable> ReflectionHelper &Method(const char *name, Callable &&method);
  operator int32_t();
  static std::string DefaultStrMethod(AnyView any);

private:
  template <typename Cls, typename FieldType> constexpr std::ptrdiff_t ReflectOffset(FieldType Cls::*member) {
    return reinterpret_cast<std::ptrdiff_t>(&((Cls *)(nullptr)->*member));
  }
  int32_t type_index;
  std::vector<MLCTypeField> fields;
  std::vector<MLCTypeMethod> methods;
  std::vector<Any> method_pool;
};
} // namespace base
} // namespace mlc

namespace mlc {
template <std::size_t N> template <typename... Args> MLC_INLINE void AnyViewArray<N>::Fill(Args &&...args) {
  static_assert(sizeof...(args) == N, "Invalid number of arguments");
  if constexpr (N > 0) {
    using IdxSeq = std::make_index_sequence<N>;
    ::mlc::base::AnyViewArrayFill<Args...>::Run(v, std::forward<Args>(args)..., IdxSeq{});
  }
}
template <typename... Args> MLC_INLINE void AnyViewArray<0>::Fill(Args &&...args) {
  static_assert(sizeof...(args) == 0, "Invalid number of arguments");
}
} // namespace mlc

#endif // MLC_CORE_H_
