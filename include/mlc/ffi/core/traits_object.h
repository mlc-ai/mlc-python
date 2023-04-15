#ifndef MLC_TRAITS_OBJECT_H_
#define MLC_TRAITS_OBJECT_H_

#include "./utils.h"

namespace mlc {
namespace ffi {

/********** PODTraits: Object *********/

template <typename T> struct ObjPtrTraitsDefault {
  MLC_INLINE static void PtrToAnyView(const T *v, MLCAny *ret) {
    if (v == nullptr) {
      ret->type_index = static_cast<int32_t>(MLCTypeIndex::kMLCNone);
      ret->v_obj = nullptr;
    } else {
      ret->type_index = v->_mlc_header.type_index;
      ret->v_obj = const_cast<MLCAny *>(reinterpret_cast<const MLCAny *>(v));
    }
  }
  MLC_INLINE static T *AnyToUnownedPtr(const MLCAny *v) {
    if (details::IsTypeIndexNone(v->type_index)) {
      return nullptr;
    }
    if (!details::IsTypeIndexPOD(v->type_index) && details::IsInstanceOf<T>(v)) {
      return reinterpret_cast<T *>(v->v_obj);
    }
    throw TemporaryTypeError();
  }
  MLC_INLINE static T *AnyToOwnedPtr(const MLCAny *v) { return AnyToUnownedPtr(v); }
  MLC_INLINE static T *AnyToOwnedPtrWithStorage(const MLCAny *v, Any *storage) = delete;
};

template <typename T> struct ObjPtrTraits<T, void> {
  MLC_INLINE static void PtrToAnyView(const T *v, MLCAny *ret) { ObjPtrTraitsDefault<T>::PtrToAnyView(v, ret); }
  MLC_INLINE static T *AnyToUnownedPtr(const MLCAny *v) { return ObjPtrTraitsDefault<T>::AnyToUnownedPtr(v); }
  MLC_INLINE static T *AnyToOwnedPtr(const MLCAny *v) { return ObjPtrTraitsDefault<T>::AnyToOwnedPtr(v); }
};

} // namespace ffi
} // namespace mlc

#endif // MLC_TRAITS_OBJECT_H_
