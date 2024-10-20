#ifndef MLC_BASE_TRAITS_SCALAR_H_
#define MLC_BASE_TRAITS_SCALAR_H_

#include "./utils.h"
#include <type_traits>

namespace mlc {
namespace base {

template <typename Int> struct TypeTraits<Int, std::enable_if_t<std::is_integral_v<Int>>> {
  static constexpr int32_t type_index = static_cast<int32_t>(MLCTypeIndex::kMLCInt);
  static constexpr const char *type_str = "int";

  MLC_INLINE static void TypeToAny(Int src, MLCAny *ret) {
    ret->type_index = static_cast<int32_t>(MLCTypeIndex::kMLCInt);
    ret->v.v_int64 = static_cast<int64_t>(src);
  }
  MLC_INLINE static Int AnyToTypeOwned(const MLCAny *v) {
    MLCTypeIndex ty = static_cast<MLCTypeIndex>(v->type_index);
    if (ty == MLCTypeIndex::kMLCInt) {
      return static_cast<Int>(v->v.v_int64);
    }
    throw TemporaryTypeError();
  }
  MLC_INLINE static Int AnyToTypeUnowned(const MLCAny *v) { return AnyToTypeOwned(v); }
  MLC_INLINE static std::string __str__(Int src) { return std::to_string(src); }
};

template <typename Float> struct TypeTraits<Float, std::enable_if_t<std::is_floating_point_v<Float>>> {
  static constexpr int32_t type_index = static_cast<int32_t>(MLCTypeIndex::kMLCFloat);
  static constexpr const char *type_str = "float";

  MLC_INLINE static void TypeToAny(Float src, MLCAny *ret) {
    ret->type_index = static_cast<int32_t>(MLCTypeIndex::kMLCFloat);
    ret->v.v_float64 = src;
  }
  MLC_INLINE static Float AnyToTypeOwned(const MLCAny *v) {
    MLCTypeIndex ty = static_cast<MLCTypeIndex>(v->type_index);
    if (ty == MLCTypeIndex::kMLCFloat) {
      return static_cast<Float>(v->v.v_float64);
    } else if (ty == MLCTypeIndex::kMLCInt) {
      return static_cast<Float>(v->v.v_int64);
    }
    throw TemporaryTypeError();
  }
  MLC_INLINE static Float AnyToTypeUnowned(const MLCAny *v) { return AnyToTypeOwned(v); }
  MLC_INLINE static std::string __str__(Float src) { return std::to_string(src); }
};

template <> struct TypeTraits<void *> {
  static constexpr int32_t type_index = static_cast<int32_t>(MLCTypeIndex::kMLCPtr);
  static constexpr const char *type_str = "Ptr";

  MLC_INLINE static void TypeToAny(void *src, MLCAny *ret) {
    ret->type_index =
        (src == nullptr) ? static_cast<int32_t>(MLCTypeIndex::kMLCNone) : static_cast<int32_t>(MLCTypeIndex::kMLCPtr);
    ret->v.v_ptr = src;
  }
  MLC_INLINE static void *AnyToTypeOwned(const MLCAny *v) {
    MLCTypeIndex ty = static_cast<MLCTypeIndex>(v->type_index);
    if (ty == MLCTypeIndex::kMLCPtr || ty == MLCTypeIndex::kMLCRawStr || ty == MLCTypeIndex::kMLCNone) {
      return v->v.v_ptr;
    }
    throw TemporaryTypeError();
  }
  MLC_INLINE static void *AnyToTypeUnowned(const MLCAny *v) { return AnyToTypeOwned(v); }
  MLC_INLINE static std::string __str__(void *src) {
    if (src == nullptr) {
      return "None";
    } else {
      std::ostringstream oss;
      oss << src;
      return oss.str();
    }
  }
};

template <> struct TypeTraits<std::nullptr_t> : public TypeTraits<void *> {
  static constexpr int32_t type_index = static_cast<int32_t>(MLCTypeIndex::kMLCNone);
  static constexpr const char *type_str = "None";
};

} // namespace base
} // namespace mlc

#endif // MLC_BASE_TRAITS_SCALAR_H_
