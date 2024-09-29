#ifndef MLC_BASE_ALL_H_
#define MLC_BASE_ALL_H_

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
  ::mlc::base::PtrToAnyView<T>(src, this);
}
template <typename T> MLC_INLINE T AnyView::Cast(::mlc::base::tag::ObjPtr) const {
  if constexpr (::mlc::base::IsObjRef<T>) {
    if (this->type_index == static_cast<int32_t>(MLCTypeIndex::kMLCNone)) {
      using RefT = Ref<typename T::TObj>;
      MLC_THROW(TypeError) << "Cannot convert from type `None` to non-nullable `" << ::mlc::base::Type2Str<RefT>::Run()
                           << "`";
    }
  }
  using TObj = typename T::TObj;
  return T([this]() -> TObj * {
    MLC_TRY_CONVERT(::mlc::base::ObjPtrTraits<TObj>::AnyToOwnedPtr(this), this->type_index,
                    ::mlc::base::Type2Str<TObj *>::Run());
  }());
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
  ::mlc::base::PtrToAnyView<T>(src, this);
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
  using TObj = typename T::TObj;
  return T([this]() -> TObj * {
    MLC_TRY_CONVERT(::mlc::base::ObjPtrTraits<TObj>::AnyToOwnedPtr(this), this->type_index,
                    ::mlc::base::Type2Str<TObj *>::Run());
  }());
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
namespace base {
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

#endif // MLC_BASE_ALL_H_
