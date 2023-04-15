#ifndef MLC_TRAITS_NUMBER_H_
#define MLC_TRAITS_NUMBER_H_

#include "./utils.h"
#include <type_traits>

namespace mlc {
namespace ffi {

/********** PODTraits: Integer *********/

template <typename Int> struct PODTraits<Int, std::enable_if_t<std::is_integral_v<Int>>> {
  MLC_INLINE static void TypeCopyToAny(Int src, MLCAny *ret) {
    ret->type_index = static_cast<int32_t>(MLCTypeIndex::kMLCInt);
    ret->v_int64 = static_cast<int64_t>(src);
  }

  MLC_INLINE static Int AnyCopyToType(const MLCAny *v) {
    MLCTypeIndex type_index = static_cast<MLCTypeIndex>(v->type_index);
    if (type_index == MLCTypeIndex::kMLCInt) {
      return static_cast<Int>(v->v_int64);
    }
    throw TemporaryTypeError();
  }

  MLC_INLINE static const char *Type2Str() { return "int"; }

  MLC_INLINE static std::string __str__(int64_t src) { return std::to_string(src); }
};

/********** PODTraits: Float *********/

template <typename Float> struct PODTraits<Float, std::enable_if_t<std::is_floating_point_v<Float>>> {
  MLC_INLINE static void TypeCopyToAny(Float src, MLCAny *ret) {
    ret->type_index = static_cast<int32_t>(MLCTypeIndex::kMLCFloat);
    ret->v_float64 = src;
  }

  MLC_INLINE static Float AnyCopyToType(const MLCAny *v) {
    MLCTypeIndex type_index = static_cast<MLCTypeIndex>(v->type_index);
    if (type_index == MLCTypeIndex::kMLCFloat) {
      return v->v_float64;
    } else if (type_index == MLCTypeIndex::kMLCInt) {
      return static_cast<Float>(v->v_int64);
    }
    throw TemporaryTypeError();
  }

  MLC_INLINE static const char *Type2Str() { return "float"; }

  MLC_INLINE static std::string __str__(double src) { return std::to_string(src); }
};

/********** PODTraits: Opaque Pointer *********/

template <> struct PODTraits<void *> {
  using Ptr = void *;

  MLC_INLINE static void TypeCopyToAny(Ptr src, MLCAny *ret) {
    ret->type_index =
        (src == nullptr) ? static_cast<int32_t>(MLCTypeIndex::kMLCNone) : static_cast<int32_t>(MLCTypeIndex::kMLCPtr);
    ret->v_ptr = src;
  }

  MLC_INLINE static Ptr AnyCopyToType(const MLCAny *v) {
    MLCTypeIndex type_index = static_cast<MLCTypeIndex>(v->type_index);
    if (type_index == MLCTypeIndex::kMLCPtr || type_index == MLCTypeIndex::kMLCRawStr ||
        type_index == MLCTypeIndex::kMLCNone) {
      return v->v_ptr;
    }
    throw TemporaryTypeError();
  }

  MLC_INLINE static const char *Type2Str() { return "Ptr"; }

  MLC_INLINE static std::string __str__(Ptr src) {
    if (src == nullptr) {
      return "None";
    } else {
      std::ostringstream oss;
      oss << src;
      return oss.str();
    }
  }
};

template <> struct PODTraits<std::nullptr_t> : public PODTraits<void *> {
  MLC_INLINE static const char *Type2Str() { return "None"; }
};
} // namespace ffi
} // namespace mlc

#endif // MLC_TRAITS_NUMBER_H_
