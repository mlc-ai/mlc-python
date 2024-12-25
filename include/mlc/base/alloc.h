#ifndef MLC_BASE_ALLOC_H_
#define MLC_BASE_ALLOC_H_

#include "./utils.h"
#include <cstring>
#include <type_traits>

namespace mlc {

template <typename T> struct DefaultObjectAllocator {
  using Storage = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

  template <typename... Args, typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
  MLC_INLINE_NO_MSVC static T *New(Args &&...args) {
    Storage *data = new Storage;
    try {
      new (data) T(std::forward<Args>(args)...);
    } catch (...) {
      delete data;
      throw;
    }
    T *ret = reinterpret_cast<T *>(data);
    ret->_mlc_header.type_index = T::_type_index;
    ret->_mlc_header.ref_cnt = 0;
    ret->_mlc_header.v.deleter = DefaultObjectAllocator<T>::Deleter;
    return ret;
  }

  template <typename PadType, typename... Args, typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
  MLC_INLINE_NO_MSVC static T *NewWithPad(size_t pad_size, Args &&...args) {
    size_t num_storages = (sizeof(T) + pad_size * sizeof(PadType) + sizeof(Storage) - 1) / sizeof(Storage);
    Storage *data = new Storage[num_storages];
    try {
      new (data) T(std::forward<Args>(args)...);
    } catch (...) {
      delete[] data;
      throw;
    }
    T *ret = reinterpret_cast<T *>(data);
    ret->_mlc_header.type_index = T::_type_index;
    ret->_mlc_header.ref_cnt = 0;
    ret->_mlc_header.v.deleter = DefaultObjectAllocator<T>::DeleterArray;
    return ret;
  }

  static void Deleter(void *objptr) {
    T *tptr = static_cast<T *>(objptr);
    tptr->T::~T();
    delete reinterpret_cast<Storage *>(tptr);
  }

  static void DeleterArray(void *objptr) {
    T *tptr = static_cast<T *>(objptr);
    tptr->T::~T();
    delete[] reinterpret_cast<Storage *>(tptr);
  }
};

template <typename T> struct PODAllocator;

#define MLC_DEF_POD_ALLOCATOR(Type, TypeIndex, Field)                                                                  \
  template <> struct PODAllocator<Type> {                                                                              \
    MLC_INLINE_NO_MSVC static MLCAny *New(Type data) {                                                                 \
      MLCBoxedPOD *ret = new MLCBoxedPOD;                                                                              \
      ret->_mlc_header.type_index = static_cast<int32_t>(TypeIndex);                                                   \
      ret->_mlc_header.ref_cnt = 0;                                                                                    \
      ret->_mlc_header.v.deleter = PODAllocator::Deleter;                                                              \
      ret->data.Field = data;                                                                                          \
      return reinterpret_cast<MLCAny *>(ret);                                                                          \
    }                                                                                                                  \
    static void Deleter(void *objptr) { delete static_cast<MLCBoxedPOD *>(objptr); }                                   \
  }

MLC_DEF_POD_ALLOCATOR(int64_t, MLCTypeIndex::kMLCInt, v_int64);
MLC_DEF_POD_ALLOCATOR(double, MLCTypeIndex::kMLCFloat, v_float64);
MLC_DEF_POD_ALLOCATOR(DLDataType, MLCTypeIndex::kMLCDataType, v_dtype);
MLC_DEF_POD_ALLOCATOR(DLDevice, MLCTypeIndex::kMLCDevice, v_device);
MLC_DEF_POD_ALLOCATOR(::mlc::base::VoidPtr, MLCTypeIndex::kMLCPtr, v_ptr);

#undef MLC_DEF_POD_ALLOCATOR

MLC_INLINE mlc::Object *AllocExternObject(int32_t type_index, int32_t num_bytes) {
  MLCAny *ptr = reinterpret_cast<MLCAny *>(std::malloc(num_bytes));
  std::memset(ptr, 0, num_bytes);
  ptr->type_index = type_index;
  ptr->ref_cnt = 0;
  ptr->v.deleter = MLCExtObjDelete;
  return reinterpret_cast<mlc::Object *>(ptr);
}

} // namespace mlc

#endif // MLC_BASE_ALLOC_H_
