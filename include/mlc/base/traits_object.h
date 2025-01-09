#ifndef MLC_BASE_TRAITS_OBJECT_H_
#define MLC_BASE_TRAITS_OBJECT_H_

#include "./lib.h"
#include "./utils.h"

namespace mlc {
namespace base {

template <typename T> struct ObjPtrTraitsDefault {
  MLC_INLINE static void TypeToAny(T *src, MLCAny *ret) {
    if (src == nullptr) {
      ret->type_index = static_cast<int32_t>(MLCTypeIndex::kMLCNone);
      ret->v.v_obj = nullptr;
    } else {
      ret->type_index = src->_mlc_header.type_index;
      ret->v.v_obj = const_cast<MLCAny *>(reinterpret_cast<const MLCAny *>(src));
    }
  }
  MLC_INLINE static T *AnyToTypeUnowned(const MLCAny *v) {
    if (::mlc::base::IsTypeIndexNone(v->type_index)) {
      return nullptr;
    }
    if (!::mlc::base::IsTypeIndexPOD(v->type_index) && ::mlc::base::IsInstanceOf<T>(v)) {
      return reinterpret_cast<T *>(v->v.v_obj);
    }
    throw TemporaryTypeError();
  }
  MLC_INLINE static T *AnyToTypeOwned(const MLCAny *v) { return AnyToTypeUnowned(v); }
  MLC_INLINE static T *AnyToTypeWithStorage(const MLCAny *v, Any *storage) = delete;
};

template <typename T> struct TypeTraits<T *, std::enable_if_t<IsObj<T> && !IsTemplate<T>>> {
  MLC_INLINE static void TypeToAny(T *src, MLCAny *ret) { ObjPtrTraitsDefault<T>::TypeToAny(src, ret); }
  MLC_INLINE static T *AnyToTypeUnowned(const MLCAny *v) { return ObjPtrTraitsDefault<T>::AnyToTypeUnowned(v); }
  MLC_INLINE static T *AnyToTypeOwned(const MLCAny *v) { return ObjPtrTraitsDefault<T>::AnyToTypeOwned(v); }
};

template <typename E> struct TypeTraits<ListObj<E> *> {
  using T = ListObj<E>;
  MLC_INLINE static void TypeToAny(T *src, MLCAny *ret) { ObjPtrTraitsDefault<UListObj>::TypeToAny(src, ret); }
  MLC_INLINE static T *AnyToTypeOwned(const MLCAny *v) { return AnyToTypeUnowned(v); }
  MLC_INLINE static T *AnyToTypeUnowned(const MLCAny *v);
};
template <typename K, typename V> struct TypeTraits<DictObj<K, V> *> {
  using T = DictObj<K, V>;
  MLC_INLINE static void TypeToAny(T *src, MLCAny *ret) { ObjPtrTraitsDefault<UDictObj>::TypeToAny(src, ret); }
  MLC_INLINE static T *AnyToTypeOwned(const MLCAny *v) { return AnyToTypeUnowned(v); }
  MLC_INLINE static T *AnyToTypeUnowned(const MLCAny *v);
};

template <typename DerivedType, typename SelfType> MLC_INLINE bool IsInstanceOf(const MLCAny *self) {
  if constexpr (std::is_same_v<DerivedType, Object> || std::is_base_of_v</*base=*/DerivedType, /*derived=*/SelfType>) {
    return true;
  }
  // Special case: `DerivedType` is exactly the underlying type of `type_index`
  if (self == nullptr) {
    return false;
  }
  int32_t type_index = self->type_index;
  if (type_index == DerivedType::_type_index) {
    return true;
  }
  // Given an index `i = DerivedType::_type_index`,
  // and the underlying type of `T = *this`, we wanted to check if
  // `T::_type_ancestors[i] == DerivedType::_type_index`.
  //
  // There are 3 ways to reflect `T` out of `this->type_index`:
  // (Case 1) Use `SelfType` as a surrogate type if `SelfType::_type_ancestors`
  // is long enough, whose length is reflected by `SelfType::_type_depth`.
  if constexpr (SelfType::_type_depth > DerivedType::_type_depth) {
    return SelfType::_type_ancestors[DerivedType::_type_depth] == DerivedType::_type_index;
  }
  if constexpr (SelfType::_type_depth == DerivedType::_type_depth) {
    return SelfType::_type_index == DerivedType::_type_index;
  }
  // (Case 2) If `type_index` falls in static object section
  if (::mlc::base::IsTypeIndexPOD(type_index)) {
    return false;
  }
  // (Case 3) Look up the type table for type hierarchy via `type_index`.
  if (MLCTypeInfo *info = Lib::GetTypeInfo(type_index)) {
    return info->type_depth > DerivedType::_type_depth &&
           info->type_ancestors[DerivedType::_type_depth] == DerivedType::_type_index;
  }
  MLC_THROW(InternalError) << "Undefined type index: " << type_index;
  MLC_UNREACHABLE();
}
} // namespace base
} // namespace mlc

#endif // MLC_BASE_TRAITS_OBJECT_H_
