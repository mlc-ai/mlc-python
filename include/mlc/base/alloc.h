#ifndef MLC_BASE_ALLOC_H_
#define MLC_BASE_ALLOC_H_

#include "./utils.h"
#include <type_traits>

namespace mlc {

template <typename T> struct DefaultObjectAllocator {
  using Storage = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
  template <typename... Args> static constexpr bool CanConstruct = std::is_constructible_v<T, Args...>;

  template <typename... Args, typename = std::enable_if_t<CanConstruct<Args...>>>
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
    ret->_mlc_header.deleter = DefaultObjectAllocator<T>::Deleter;
    return ret;
  }

  template <typename PadType, typename... Args, typename = std::enable_if_t<CanConstruct<Args...>>>
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
    ret->_mlc_header.deleter = DefaultObjectAllocator<T>::DeleterArray;
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

template <typename T> struct PODAllocator {
  static_assert(std::is_trivial_v<T> && std::is_standard_layout_v<T>, "PODAllocator can only be used with POD types");
  struct Storage {
    MLCAny _mlc_header;
    T data;
  };

  MLC_INLINE_NO_MSVC static MLCAny *New(T data) {
    Storage *ret = new Storage;
    ret->_mlc_header.type_index = ::mlc::base::TypeTraits<T>::type_index;
    ret->_mlc_header.ref_cnt = 0;
    ret->_mlc_header.deleter = PODAllocator::Deleter;
    ret->data = data;
    return reinterpret_cast<MLCAny *>(ret);
  }

  static void Deleter(void *objptr) { delete reinterpret_cast<Storage *>(objptr); }
};

} // namespace mlc

#endif // MLC_BASE_ALLOC_H_
