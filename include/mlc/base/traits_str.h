#ifndef MLC_BASE_TRAITS_STR_H_
#define MLC_BASE_TRAITS_STR_H_

#include "./utils.h"

namespace mlc {
namespace base {

template <> struct TypeTraits<const char *> {
  static constexpr int32_t type_index = static_cast<int32_t>(MLCTypeIndex::kMLCRawStr);
  static constexpr const char *type_str = "char *";

  static void TypeToAny(const char *src, MLCAny *ret) {
    ret->type_index = static_cast<int32_t>(MLCTypeIndex::kMLCRawStr);
    ret->v.v_str = src;
  }
  static const char *AnyToTypeOwned(const MLCAny *v) {
    MLCTypeIndex ty = static_cast<MLCTypeIndex>(v->type_index);
    if (ty == MLCTypeIndex::kMLCRawStr) {
      return v->v.v_str;
    }
    if (ty == MLCTypeIndex::kMLCStr) {
      return reinterpret_cast<MLCStr *>(v->v.v_obj)->data;
    }
    throw TemporaryTypeError();
  }
  static const char *AnyToTypeUnowned(const MLCAny *v) { return AnyToTypeOwned(v); }
  static std::string __str__(const char *src) { return '"' + std::string(src) + '"'; }
};

template <> struct TypeTraits<char *> {
  static constexpr int32_t type_index = static_cast<int32_t>(MLCTypeIndex::kMLCRawStr);
  static constexpr const char *type_str = "char *";

  static void TypeToAny(char *src, MLCAny *ret) { return TypeTraits<const char *>::TypeToAny(src, ret); }
  static char *AnyToTypeOwned(const MLCAny *v) {
    return const_cast<char *>(TypeTraits<const char *>::AnyToTypeOwned(v));
  }
  static const char *AnyToTypeUnowned(const MLCAny *v) { return AnyToTypeOwned(v); }
  static std::string __str__(const char *src) { return '"' + std::string(src) + '"'; }
};

template <> struct TypeTraits<std::string> {
  static constexpr int32_t type_index = static_cast<int32_t>(MLCTypeIndex::kMLCRawStr);
  static constexpr const char *type_str = "char *";

  static void TypeToAny(const std::string &src, MLCAny *ret) {
    return TypeTraits<const char *>::TypeToAny(src.data(), ret);
  }
  static std::string AnyToTypeOwned(const MLCAny *v) { return TypeTraits<const char *>::AnyToTypeOwned(v); }
  static std::string AnyToTypeUnowned(const MLCAny *v) { return TypeTraits<const char *>::AnyToTypeUnowned(v); }
  static std::string __str__(const char *src) { return '"' + std::string(src) + '"'; }
};

template <size_t N> struct TypeTraits<char[N]> : public TypeTraits<const char *> {};

} // namespace base
} // namespace mlc

#endif // MLC_BASE_TRAITS_STR_H_
