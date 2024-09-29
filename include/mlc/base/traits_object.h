#ifndef MLC_BASE_TRAITS_OBJECT_H_
#define MLC_BASE_TRAITS_OBJECT_H_

#include "./utils.h"

namespace mlc {
namespace base {

template <typename T> struct ObjPtrTraitsDefault {
  MLC_INLINE static T *AnyToUnownedPtr(const MLCAny *v) {
    if (::mlc::base::IsTypeIndexNone(v->type_index)) {
      return nullptr;
    }
    if (!::mlc::base::IsTypeIndexPOD(v->type_index) && ::mlc::base::IsInstanceOf<T>(v)) {
      return reinterpret_cast<T *>(v->v_obj);
    }
    throw TemporaryTypeError();
  }
  MLC_INLINE static T *AnyToOwnedPtr(const MLCAny *v) { return AnyToUnownedPtr(v); }
  MLC_INLINE static T *AnyToOwnedPtrWithStorage(const MLCAny *v, Any *storage) = delete;
};

template <typename T> struct ObjPtrTraits<T, void> {
  MLC_INLINE static T *AnyToUnownedPtr(const MLCAny *v) { return ObjPtrTraitsDefault<T>::AnyToUnownedPtr(v); }
  MLC_INLINE static T *AnyToOwnedPtr(const MLCAny *v) { return ObjPtrTraitsDefault<T>::AnyToOwnedPtr(v); }
};

struct ObjectDummyRoot {
  static constexpr int32_t _type_depth = -1;
  static constexpr int32_t _type_index = -1;
};

template <typename ObjectType> struct DefaultObjectAllocator<ObjectType, void> {
  using Storage = typename std::aligned_storage<sizeof(ObjectType), alignof(ObjectType)>::type;
  template <typename... Args> static constexpr bool CanConstruct = std::is_constructible_v<ObjectType, Args...>;

  template <typename... Args, typename = std::enable_if_t<CanConstruct<Args...>>>
  MLC_INLINE_NO_MSVC static ObjectType *New(Args &&...args) {
    Storage *data = new Storage[1];
    try {
      new (data) ObjectType(std::forward<Args>(args)...);
    } catch (...) {
      delete[] data;
      throw;
    }
    return InitIntrusive(data);
  }

  template <typename PadType, typename... Args, typename = std::enable_if_t<CanConstruct<Args...>>>
  MLC_INLINE_NO_MSVC static ObjectType *NewWithPad(size_t pad_size, Args &&...args) {
    size_t num_storages = (sizeof(ObjectType) + pad_size * sizeof(PadType) + sizeof(Storage) - 1) / sizeof(Storage);
    Storage *data = new Storage[num_storages];
    try {
      new (data) ObjectType(std::forward<Args>(args)...);
    } catch (...) {
      delete[] data;
      throw;
    }
    return InitIntrusive(data);
  }

  MLC_INLINE static ObjectType *InitIntrusive(Storage *data) {
    MLCAny header;
    header.type_index = ObjectType::_type_index;
    header.ref_cnt = 0;
    header.deleter = DefaultObjectAllocator::Deleter;
    ObjectType *ret = reinterpret_cast<ObjectType *>(data);
    ret->_mlc_header = header;
    return ret;
  }

  static void Deleter(void *objptr) {
    ObjectType *tptr = static_cast<ObjectType *>(objptr);
    tptr->ObjectType::~ObjectType();
    delete[] reinterpret_cast<Storage *>(tptr);
  }
};

template <typename T> struct IsDerivedFromImpl<T, T> {
  static constexpr bool value = true;
};
template <typename Base> struct IsDerivedFromImpl<ObjectDummyRoot, Base> {
  static constexpr bool value = false;
};
template <typename Derived, typename Base> struct IsDerivedFromImpl {
  static constexpr bool value = IsDerivedFromImpl<typename Derived::_type_parent, Base>::value;
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
  MLCTypeInfo *info;
  MLCTypeIndex2Info(nullptr, type_index, &info);
  if (info == nullptr) {
    MLC_THROW(InternalError) << "Undefined type index: " << type_index;
  }
  return info->type_depth > DerivedType::_type_depth &&
         info->type_ancestors[DerivedType::_type_depth] == DerivedType::_type_index;
}
} // namespace base
} // namespace mlc

#endif // MLC_BASE_TRAITS_OBJECT_H_
