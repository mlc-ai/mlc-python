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
#include <type_traits>

namespace mlc {

/*********** New stuff ***********/

MLC_INLINE AnyView::AnyView(const Any &src) : MLCAny(static_cast<const MLCAny &>(src)) {}
MLC_INLINE AnyView::AnyView(Any &&src) : MLCAny(static_cast<const MLCAny &>(src)) {}

template <typename _T, typename> AnyView::AnyView(_T &&src) : MLCAny() {
  using namespace ::mlc::base;
  constexpr TypeKind kind = TypeKindOf<_T>;
  if constexpr (kind == TypeKind::kObjRef) {
    using TObjRef = RemoveCR<_T>;
    using TObj = typename TObjRef::TObj;
    TypeTraits<TObj *>::TypeToAny(const_cast<TObj *>(src.get()), this);
  } else if constexpr (kind == TypeKind::kObjPtr) {
    using TObjPtr = RemoveCR<_T>;
    using TObj = std::remove_pointer_t<TObjPtr>;
    TypeTraits<TObj *>::TypeToAny(src, this);
  } else { // POD
    using TPOD = RemoveCR<_T>;
    TypeTraits<TPOD>::TypeToAny(src, this);
  }
}

template <typename T, typename> MLC_INLINE AnyView::AnyView(const T *src) : MLCAny() {
  ::mlc::base::TypeTraits<T *>::TypeToAny(const_cast<T *>(src), this);
}

template <typename T> MLC_INLINE AnyView::AnyView(const Ref<T> &src) : MLCAny() {
  if (const auto *value = src.get()) {
    if constexpr (::mlc::base::IsPOD<T>) {
      using TPOD = T;
      ::mlc::base::TypeTraits<TPOD>::TypeToAny(*value, this);
    } else {
      using TObj = T;
      ::mlc::base::TypeTraits<TObj *>::TypeToAny(const_cast<TObj *>(value), this);
    }
  }
}

template <typename T> MLC_INLINE AnyView::AnyView(Ref<T> &&src) : AnyView(static_cast<const Ref<T> &>(src)) {
  // `src` is not reset here because `AnyView` does not take ownership of the object
}

template <typename T> MLC_INLINE AnyView::AnyView(const Optional<T> &src) {
  if (const auto *value = src.get()) {
    if constexpr (::mlc::base::IsPOD<T>) {
      using TPOD = T;
      ::mlc::base::TypeTraits<TPOD>::TypeToAny(*value, this);
    } else {
      using TObj = typename T::TObj;
      ::mlc::base::TypeTraits<TObj *>::TypeToAny(const_cast<TObj *>(value), this);
    }
  }
}

template <typename T> MLC_INLINE AnyView::AnyView(Optional<T> &&src) : AnyView(static_cast<const Optional<T> &>(src)) {
  // `src` is not reset here because `AnyView` does not take ownership of the object
}

template <typename T, typename> MLC_INLINE AnyView &AnyView::operator=(T &&src) {
  AnyView(std::forward<T>(src)).Swap(*this);
  return *this;
}

MLC_INLINE AnyView &AnyView::operator=(const Any &src) {
  AnyView(src).Swap(*this);
  return *this;
}

MLC_INLINE AnyView &AnyView::operator=(Any &&src) {
  AnyView(std::move(src)).Swap(*this);
  return *this;
}

template <typename T, typename> MLC_INLINE AnyView &AnyView::operator=(const T *src) {
  AnyView(src).Swap(*this);
  return *this;
}

template <typename T> MLC_INLINE AnyView &AnyView::operator=(const Ref<T> &src) {
  AnyView(src).Swap(*this);
  return *this;
}

template <typename T> MLC_INLINE AnyView &AnyView::operator=(Ref<T> &&src) {
  AnyView(std::move(src)).Swap(*this);
  return *this;
}

template <typename T> MLC_INLINE AnyView &AnyView::operator=(const Optional<T> &src) {
  AnyView(src).Swap(*this);
  return *this;
}

template <typename T> MLC_INLINE AnyView &AnyView::operator=(Optional<T> &&src) {
  AnyView(std::move(src)).Swap(*this);
  return *this;
}

template <typename T, typename> inline AnyView::operator T() const {
  using namespace ::mlc::base;
  if constexpr (IsObjRef<T>) {
    using TObjRef = T;
    using TObj = typename TObjRef::TObj;
    TObj *value = [this]() {
      MLC_TRY_CONVERT(TypeTraits<TObj *>::AnyToTypeOwned(this), this->type_index, Type2Str<TObj *>::Run());
    }();
    return TObjRef(value);
  } else {
    // POD or ObjPtr
    MLC_TRY_CONVERT(TypeTraits<T>::AnyToTypeUnowned(this), this->type_index, Type2Str<T>::Run());
  }
}

template <typename T, typename> inline Any::operator T() const {
  using namespace ::mlc::base;
  if constexpr (IsObjRef<T>) {
    using TObjRef = T;
    using TObj = typename TObjRef::TObj;
    TObj *value = [this]() {
      MLC_TRY_CONVERT(TypeTraits<TObj *>::AnyToTypeOwned(this), this->type_index, Type2Str<TObj *>::Run());
    }();
    return TObjRef(value);
  } else {
    // POD or ObjPtr
    MLC_TRY_CONVERT(TypeTraits<T>::AnyToTypeUnowned(this), this->type_index, Type2Str<T>::Run());
  }
}

template <typename T> inline AnyView::operator Ref<T>() const {
  using namespace ::mlc::base;
  if (IsTypeIndexNone(this->type_index)) {
    return Ref<T>(nullptr);
  }
  if constexpr (IsPOD<T>) {
    using TPOD = T;
    TPOD value = [this]() {
      MLC_TRY_CONVERT(TypeTraits<TPOD>::AnyToTypeUnowned(this), this->type_index, Type2Str<TPOD>::Run());
    }();
    return Ref<T>(value);
  } else {
    static_assert(IsObj<T>, "Unsupported type. Expect `Ref<POD>` or `Ref<Obj>`");
    using TObj = T;
    TObj *value = [this]() {
      MLC_TRY_CONVERT(TypeTraits<TObj *>::AnyToTypeOwned(this), this->type_index, Type2Str<TObj *>::Run());
    }();
    return Ref<T>(value);
  }
}

template <typename T> inline Any::operator Ref<T>() const {
  using namespace ::mlc::base;
  if (IsTypeIndexNone(this->type_index)) {
    return Ref<T>(nullptr);
  }
  if constexpr (IsPOD<T>) {
    using TPOD = T;
    TPOD value = [this]() {
      MLC_TRY_CONVERT(TypeTraits<TPOD>::AnyToTypeUnowned(this), this->type_index, Type2Str<TPOD>::Run());
    }();
    return Ref<T>(value);
  } else {
    static_assert(IsObj<T>, "Unsupported type. Expect `Ref<POD>` or `Ref<Obj>`");
    using TObj = T;
    TObj *value = [this]() {
      MLC_TRY_CONVERT(TypeTraits<TObj *>::AnyToTypeOwned(this), this->type_index, Type2Str<TObj *>::Run());
    }();
    return Ref<T>(value);
  }
}

template <typename T> inline AnyView::operator Optional<T>() const {
  using namespace ::mlc::base;
  if (IsTypeIndexNone(this->type_index)) {
    return Optional<T>(Null);
  }
  if constexpr (IsPOD<T>) {
    using TPOD = T;
    TPOD value = [this]() {
      MLC_TRY_CONVERT(TypeTraits<TPOD>::AnyToTypeUnowned(this), this->type_index, Type2Str<TPOD>::Run());
    }();
    return Optional<T>(value);
  } else {
    static_assert(IsObjRef<T>, "Unsupported type. Expect `Optional<POD>` or `Optional<ObjRef>`");
    using TObj = typename T::TObj;
    TObj *value = [this]() {
      MLC_TRY_CONVERT(TypeTraits<TObj *>::AnyToTypeOwned(this), this->type_index, Type2Str<TObj *>::Run());
    }();
    return Optional<T>(value);
  }
}

template <typename T> inline Any::operator Optional<T>() const {
  using namespace ::mlc::base;
  if (IsTypeIndexNone(this->type_index)) {
    return Optional<T>(Null);
  }
  if constexpr (IsPOD<T>) {
    using TPOD = T;
    TPOD value = [this]() {
      MLC_TRY_CONVERT(TypeTraits<TPOD>::AnyToTypeUnowned(this), this->type_index, Type2Str<TPOD>::Run());
    }();
    return Optional<T>(value);
  } else {
    static_assert(IsObjRef<T>, "Unsupported type. Expect `Optional<POD>` or `Optional<ObjRef>`");
    using TObj = typename T::TObj;
    TObj *value = [this]() {
      MLC_TRY_CONVERT(TypeTraits<TObj *>::AnyToTypeOwned(this), this->type_index, Type2Str<TObj *>::Run());
    }();
    return Optional<T>(value);
  }
}

/*********** Section 2. Conversion between Any/AnyView <=> POD ***********/

template <typename _T, typename> MLC_INLINE_NO_MSVC _T AnyView::_CastWithStorage(Any *storage) const {
  using namespace ::mlc::base;
  using T = RemoveCR<_T>;
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
