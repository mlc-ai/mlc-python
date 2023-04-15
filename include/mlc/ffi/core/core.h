#ifndef MLC_CORE_H_
#define MLC_CORE_H_

#include "./any.h"
#include "./object.h"
#include "./ref.h"
#include "./traits_device.h"
#include "./traits_dtype.h"
#include "./traits_object.h"
#include "./traits_scalar.h"
#include "./traits_str.h"
#include "./utils.h"
#include <cstring>

namespace mlc {
namespace ffi {

MLC_INLINE Any VTableGet(int32_t type_index, const char *attr_key) {
  Any ret;
  MLCVTableGet(nullptr, type_index, attr_key, &ret);
  return ret;
}

/*********** AnyView ***********/

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

MLC_INLINE AnyView::AnyView(tag::ObjPtr, const MLCObjPtr &src) : MLCAny() {
  if (src.ptr != nullptr) {
    this->type_index = src.ptr->type_index;
    this->v_obj = src.ptr;
  }
}
MLC_INLINE AnyView::AnyView(tag::ObjPtr, MLCObjPtr &&src) : MLCAny() {
  if (src.ptr != nullptr) {
    this->type_index = src.ptr->type_index;
    this->v_obj = src.ptr;
  }
}
template <typename T> MLC_INLINE AnyView::AnyView(tag::POD, const T &src) : MLCAny() {
  PODTraits<details::RemoveCR<T>>::TypeCopyToAny(src, this);
}
template <typename T> MLC_INLINE AnyView::AnyView(tag::POD, T &&src) : MLCAny() {
  PODTraits<details::RemoveCR<T>>::TypeCopyToAny(src, this);
}
template <typename T> MLC_INLINE AnyView::AnyView(tag::RawObjPtr, T *src) : MLCAny() {
  ObjPtrTraits<T>::PtrToAnyView(src, this);
}
template <typename T> MLC_INLINE T AnyView::Cast(tag::ObjPtr) const {
  if constexpr (details::IsObjRef<T>) {
    if (this->type_index == static_cast<int32_t>(MLCTypeIndex::kMLCNone)) {
      using RefT = Ref<typename T::TObj>;
      MLC_THROW(TypeError) << "Cannot convert from type `None` to non-nullable `" << Type2Str<RefT>::Run() << "`";
    }
  }
  return T(T::THelper::TryConvert(this));
}
template <typename T> MLC_INLINE_NO_MSVC T AnyView::Cast(tag::POD) const {
  MLC_TRY_CONVERT(PODTraits<details::RemoveCR<T>>::AnyCopyToType(this), this->type_index, Type2Str<T>::Run());
}
template <typename _T> MLC_INLINE_NO_MSVC _T AnyView::Cast(tag::RawObjPtr) const {
  using T = std::remove_pointer_t<_T>;
  MLC_TRY_CONVERT(ObjPtrTraits<details::RemoveCR<T>>::AnyToUnownedPtr(this), this->type_index, Type2Str<T *>::Run());
}
template <typename T> MLC_INLINE_NO_MSVC T *AnyView::CastWithStorage(Any *storage) const {
  MLC_TRY_CONVERT(ObjPtrTraits<T>::AnyToOwnedPtrWithStorage(this, storage), this->type_index, Type2Str<T *>::Run());
}

/*********** Any ***********/

MLC_INLINE Any::Any(tag::ObjPtr, const MLCObjPtr &src) : MLCAny() {
  if (src.ptr != nullptr) {
    this->type_index = src.ptr->type_index;
    this->v_obj = src.ptr;
    this->IncRef();
  }
}
MLC_INLINE Any::Any(tag::ObjPtr, MLCObjPtr &&src) : MLCAny() {
  if (src.ptr != nullptr) {
    this->type_index = src.ptr->type_index;
    this->v_obj = src.ptr;
    src.ptr = nullptr;
  }
}
template <typename T> MLC_INLINE Any::Any(tag::RawObjPtr, T *src) : MLCAny() {
  ObjPtrTraits<T>::PtrToAnyView(src, this);
  this->IncRef();
}
template <typename T> MLC_INLINE T Any::Cast(tag::ObjPtr) const {
  if constexpr (details::IsObjRef<T>) {
    if (this->type_index == static_cast<int32_t>(MLCTypeIndex::kMLCNone)) {
      using RefT = Ref<typename T::TObj>;
      MLC_THROW(TypeError) << "Cannot convert from type `None` to non-nullable `" << Type2Str<RefT>::Run() << "`";
    }
  }
  return T(T::THelper::TryConvert(this));
}
template <typename T> MLC_INLINE_NO_MSVC T Any::Cast(tag::POD) const {
  MLC_TRY_CONVERT(PODTraits<details::RemoveCR<T>>::AnyCopyToType(this), this->type_index, Type2Str<T>::Run());
}
template <typename _T> MLC_INLINE_NO_MSVC _T Any::Cast(tag::RawObjPtr) const {
  using T = std::remove_pointer_t<_T>;
  MLC_TRY_CONVERT(ObjPtrTraits<details::RemoveCR<T>>::AnyToUnownedPtr(this), this->type_index, Type2Str<T *>::Run());
}
template <typename T> MLC_INLINE_NO_MSVC T *Any::CastWithStorage(Any *storage) const {
  MLC_TRY_CONVERT(ObjPtrTraits<T>::AnyToOwnedPtrWithStorage(this, storage), this->type_index, Type2Str<T *>::Run());
}

/*********** Type checking ***********/

namespace details {
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
} // namespace details

/*********** Reflection ***********/

struct ObjectRef : protected ObjectRefDummyRoot {
  MLC_DEF_OBJ_REF(ObjectRef, Object, ObjectRefDummyRoot).Method("__init__", InitOf<Object>);
};

} // namespace ffi
} // namespace mlc

#endif // MLC_CORE_H_
