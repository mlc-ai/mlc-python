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
MLC_INLINE AnyView &AnyView::operator=(const Any &src) {
  *static_cast<MLCAny *>(this) = static_cast<const MLCAny &>(src);
  return *this;
}
MLC_INLINE AnyView &AnyView::operator=(Any &&src) {
  *static_cast<MLCAny *>(this) = static_cast<const MLCAny &>(src);
  return *this;
}
MLC_INLINE Any &Any::operator=(const Any &src) {
  Any(src).Swap(*this);
  return *this;
}
MLC_INLINE Any &Any::operator=(Any &&src) {
  Any(std::move(src)).Swap(*this);
  return *this;
}
MLC_INLINE Any &Any::operator=(const AnyView &src) {
  Any(src).Swap(*this);
  return *this;
}
MLC_INLINE Any &Any::operator=(AnyView &&src) {
  Any(std::move(src)).Swap(*this);
  return *this;
}

/*********** Section 2. Conversion between Any/AnyView <=> POD ***********/

template <typename T> MLC_INLINE AnyView::AnyView(const T &src) : MLCAny() {
  using namespace ::mlc::base;
  if constexpr (HasTypeTraits<T>) {
    TypeTraits<T>::TypeToAny(src, this);
  } else {
    (src.operator AnyView()).Swap(*this);
  }
}

template <typename T> MLC_INLINE Any::Any(const T &src) : MLCAny() {
  using namespace ::mlc::base;
  if constexpr (HasTypeTraits<RemoveCR<T>>) {
    TypeTraits<RemoveCR<T>>::TypeToAny(src, this);
    this->SwitchFromRawStr();
    this->IncRef();
  } else {
    (src.operator Any()).Swap(*this);
  }
}

template <typename T> MLC_INLINE_NO_MSVC T AnyView::Cast() const {
  using namespace ::mlc::base;
  if constexpr (HasTypeTraits<T>) {
    MLC_TRY_CONVERT(TypeTraits<T>::AnyToTypeUnowned(this), this->type_index, Type2Str<T>::Run());
  } else {
    return T(*this);
  }
}

template <typename T> MLC_INLINE_NO_MSVC T Any::Cast() const {
  using namespace ::mlc::base;
  if constexpr (HasTypeTraits<T>) {
    MLC_TRY_CONVERT(TypeTraits<T>::AnyToTypeUnowned(this), this->type_index, Type2Str<T>::Run());
  } else {
    return T(*this);
  }
}

template <typename T> MLC_INLINE_NO_MSVC T AnyView::CastWithStorage(Any *storage) const {
  using namespace ::mlc::base;
  MLC_TRY_CONVERT(TypeTraits<T>::AnyToTypeWithStorage(this, storage), this->type_index, Type2Str<T>::Run());
}

template <typename T> MLC_INLINE_NO_MSVC T Any::CastWithStorage(Any *storage) const {
  using namespace ::mlc::base;
  MLC_TRY_CONVERT(TypeTraits<T>::AnyToTypeWithStorage(this, storage), this->type_index, Type2Str<T>::Run());
}

/*********** Section 3. Conversion between Any/AnyView <=> Ref ***********/

template <typename T> MLC_INLINE Ref<T>::Ref(const AnyView &src) : TBase() { TBase::_Init<T>(src); }
template <typename T> MLC_INLINE Ref<T>::Ref(const Any &src) : TBase() { TBase::_Init<T>(src); }
template <typename T> MLC_INLINE Ref<T>::operator AnyView() const {
  if (this->ptr != nullptr) {
    AnyView ret;
    ret.type_index = this->ptr->type_index;
    ret.v_obj = this->ptr;
    return ret;
  }
  return AnyView();
}
template <typename T> MLC_INLINE Ref<T>::operator Any() const & {
  if (this->ptr != nullptr) {
    Any ret;
    ret.type_index = this->ptr->type_index;
    ret.v_obj = this->ptr;
    ::mlc::base::IncRef(this->ptr);
    return ret;
  }
  return Any();
}
template <typename T> MLC_INLINE Ref<T>::operator Any() && {
  if (this->ptr != nullptr) {
    Any ret;
    ret.type_index = this->ptr->type_index;
    ret.v_obj = this->ptr;
    this->ptr = nullptr;
    return ret;
  }
  return Any();
}

/*********** Section 4. AnyViewArray ***********/

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
