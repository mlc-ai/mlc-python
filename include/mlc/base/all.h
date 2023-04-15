#ifndef MLC_BASE_ALL_H_
#define MLC_BASE_ALL_H_

#include "./alloc.h"         // IWYU pragma: export
#include "./any.h"           // IWYU pragma: export
#include "./base_traits.h"   // IWYU pragma: export
#include "./optional.h"      // IWYU pragma: export
#include "./ref.h"           // IWYU pragma: export
#include "./traits_device.h" // IWYU pragma: export
#include "./traits_dtype.h"  // IWYU pragma: export
#include "./traits_object.h" // IWYU pragma: export
#include "./traits_scalar.h" // IWYU pragma: export
#include "./traits_str.h"    // IWYU pragma: export
#include <mlc/c_api.h>       // IWYU pragma: export

namespace mlc {

/*********** Section 1. Any <=> Any View ***********/
MLC_INLINE AnyView::AnyView(const Any &src) : MLCAny(static_cast<const MLCAny &>(src)) {}
MLC_INLINE AnyView::AnyView(Any &&src) : MLCAny(static_cast<const MLCAny &>(src)) {}
MLC_INLINE Any::Any(const Any &src) : MLCAny(static_cast<const MLCAny &>(src)) { this->IncRef(); }
MLC_INLINE Any::Any(Any &&src) : MLCAny(static_cast<const MLCAny &>(src)) { *static_cast<MLCAny *>(&src) = MLCAny(); }
MLC_INLINE Any::Any(const AnyView &src) : MLCAny(static_cast<const MLCAny &>(src)) {
  this->SwitchFromRawStr();
  this->IncRef();
}
MLC_INLINE Any::Any(AnyView &&src) : MLCAny(static_cast<const MLCAny &>(src)) {
  *static_cast<MLCAny *>(&src) = MLCAny();
  this->SwitchFromRawStr();
  this->IncRef();
}
MLC_INLINE Any &Any::operator=(const Any &src) {
  Any(src).Swap(*this);
  return *this;
}
MLC_INLINE Any &Any::operator=(Any &&src) {
  Any(std::move(src)).Swap(*this);
  return *this;
}

/*********** Section 2. Conversion between Any/AnyView <=> POD ***********/

template <typename _T> MLC_INLINE AnyView::AnyView(const _T &src) : MLCAny() {
  using namespace ::mlc::base;
  using T = RemoveCR<_T>;
  static_assert(Anyable<T>);
  if constexpr (IsPOD<T> || IsRawObjPtr<T>) {
    TypeTraits<T>::TypeToAny(src, this);
  } else {
    (src.operator AnyView()).Swap(*this);
  }
}

template <typename _T> MLC_INLINE Any::Any(const _T &src) : MLCAny() {
  using namespace ::mlc::base;
  using T = RemoveCR<_T>;
  static_assert(Anyable<T>);
  if constexpr (IsPOD<T> || IsRawObjPtr<T>) {
    TypeTraits<RemoveCR<T>>::TypeToAny(src, this);
    this->SwitchFromRawStr();
    this->IncRef();
  } else {
    (src.operator Any()).Swap(*this);
  }
}

template <typename _T> MLC_INLINE_NO_MSVC _T AnyView::Cast() const {
  using namespace ::mlc::base;
  using T = RemoveCR<_T>;
  static_assert(Anyable<T>);
  if constexpr (IsPOD<T> || IsRawObjPtr<T>) {
    MLC_TRY_CONVERT(TypeTraits<T>::AnyToTypeUnowned(this), this->type_index, Type2Str<T>::Run());
  } else {
    return T(*this);
  }
}

template <typename _T> MLC_INLINE_NO_MSVC _T Any::Cast() const {
  using namespace ::mlc::base;
  using T = RemoveCR<_T>;
  static_assert(Anyable<T>);
  if constexpr (IsPOD<T> || IsRawObjPtr<T>) {
    MLC_TRY_CONVERT(TypeTraits<T>::AnyToTypeUnowned(this), this->type_index, Type2Str<T>::Run());
  } else {
    return T(*this);
  }
}

template <typename _T> MLC_INLINE_NO_MSVC _T AnyView::CastWithStorage(Any *storage) const {
  using namespace ::mlc::base;
  using T = RemoveCR<_T>;
  static_assert(Anyable<T>);
  MLC_TRY_CONVERT(TypeTraits<T>::AnyToTypeWithStorage(this, storage), this->type_index, Type2Str<T>::Run());
}

template <typename _T> MLC_INLINE_NO_MSVC _T Any::CastWithStorage(Any *storage) const {
  using namespace ::mlc::base;
  using T = RemoveCR<_T>;
  static_assert(Anyable<T>);
  MLC_TRY_CONVERT(TypeTraits<T>::AnyToTypeWithStorage(this, storage), this->type_index, Type2Str<T>::Run());
}

/*********** Section 3. AnyViewArray ***********/

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
