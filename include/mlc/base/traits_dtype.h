#ifndef MLC_BASE_TRAITS_DTYPE_H_
#define MLC_BASE_TRAITS_DTYPE_H_

#include "./utils.h"
#include <cstdlib>
#include <unordered_map>

namespace mlc {
namespace base {

inline const char *DLDataTypeCode2Str(int32_t type_code);
inline DLDataType String2DLDataType(const std::string &source);
inline bool DataTypeEqual(DLDataType a, DLDataType b) {
  return a.code == b.code && a.bits == b.bits && a.lanes == b.lanes;
}

template <> struct TypeTraits<DLDataType> {
  static constexpr int32_t type_index = static_cast<int32_t>(MLCTypeIndex::kMLCDataType);
  static constexpr const char *type_str = "dtype";

  MLC_INLINE static void TypeToAny(DLDataType src, MLCAny *ret) {
    ret->type_index = static_cast<int32_t>(MLCTypeIndex::kMLCDataType);
    ret->v.v_dtype = src;
  }

  MLC_INLINE static DLDataType AnyToTypeOwned(const MLCAny *v) {
    MLCTypeIndex ty = static_cast<MLCTypeIndex>(v->type_index);
    if (ty == MLCTypeIndex::kMLCDataType) {
      return v->v.v_dtype;
    }
    if (ty == MLCTypeIndex::kMLCRawStr) {
      return String2DLDataType(v->v.v_str);
    }
    if (ty == MLCTypeIndex::kMLCStr) {
      return String2DLDataType(reinterpret_cast<const MLCStr *>(v->v.v_obj)->data);
    }
    throw TemporaryTypeError();
  }

  MLC_INLINE static DLDataType AnyToTypeUnowned(const MLCAny *v) { return AnyToTypeOwned(v); }

  MLC_INLINE static std::string __str__(DLDataType dtype) {
    int32_t code = static_cast<int32_t>(dtype.code);
    int32_t bits = static_cast<int32_t>(dtype.bits);
    int32_t lanes = static_cast<int32_t>(dtype.lanes);
    if (code == kDLUInt && bits == 1 && lanes == 1) {
      return "bool";
    }
    if (code == kDLOpaqueHandle && bits == 0 && lanes == 0) {
      return "void";
    }
    std::ostringstream os;
    os << DLDataTypeCode2Str(code);
    if (code != kDLDataTypeFloat8E5M2 && code != kDLDataTypeFloat8E4M3FN) {
      os << bits;
    }
    if (lanes != 1) {
      os << "x" << lanes;
    }
    return os.str();
  }

  static inline MLC_SYMBOL_HIDE std::unordered_map<std::string, DLDataType> preset = {
      {"void", {kDLOpaqueHandle, 0, 0}},
      {"bool", {kDLUInt, 1, 1}},
      {"int4", {kDLInt, 4, 1}},
      {"int8", {kDLInt, 8, 1}},
      {"int16", {kDLInt, 16, 1}},
      {"int32", {kDLInt, 32, 1}},
      {"int64", {kDLInt, 64, 1}},
      {"uint4", {kDLUInt, 4, 1}},
      {"uint8", {kDLUInt, 8, 1}},
      {"uint16", {kDLUInt, 16, 1}},
      {"uint32", {kDLUInt, 32, 1}},
      {"uint64", {kDLUInt, 64, 1}},
      {"float8_e4m3fn", {kDLDataTypeFloat8E4M3FN, 8, 1}},
      {"float8_e5m2", {kDLDataTypeFloat8E5M2, 8, 1}},
      {"float16", {kDLFloat, 16, 1}},
      {"float32", {kDLFloat, 32, 1}},
      {"float64", {kDLFloat, 64, 1}},
      {"bfloat16", {kDLBfloat, 16, 1}},
  };
};

MLC_INLINE const char *DLDataTypeCode2Str(int32_t type_code) {
  switch (type_code) {
  case kDLInt:
    return "int";
  case kDLUInt:
    return "uint";
  case kDLFloat:
    return "float";
  case kDLOpaqueHandle:
    return "ptr";
  case kDLBfloat:
    return "bfloat";
  case kDLComplex:
    return "complex";
  case kDLBool:
    return "bool";
  case kDLDataTypeFloat8E4M3FN:
    return "float8_e4m3fn";
  case kDLDataTypeFloat8E5M2:
    return "float8_e5m2";
  }
  return "unknown";
}

inline DLDataType String2DLDataType(const std::string &source) {
  constexpr int64_t u16_max = 65535;
  constexpr int64_t u8_max = 255;
  using Traits = TypeTraits<DLDataType>;
  if (auto it = Traits::preset.find(source); it != Traits::preset.end()) {
    return it->second;
  }
  try {
    int64_t dtype_lanes = 1;
    std::string dtype_str;
    if (size_t x_pos = source.rfind('x'); x_pos != std::string::npos) {
      dtype_str = source.substr(0, x_pos);
      dtype_lanes = StrToInt(source, x_pos + 1);
      if (dtype_lanes < 0 || dtype_lanes > u16_max) {
        throw std::runtime_error("Invalid DLDataType");
      }
    } else {
      dtype_str = source;
    }
    if (dtype_str == "float8_e4m3fn") {
      return {static_cast<uint8_t>(kDLDataTypeFloat8E4M3FN), 8, static_cast<uint16_t>(dtype_lanes)};
    }
    if (dtype_str == "float8_e5m2") {
      return {static_cast<uint8_t>(kDLDataTypeFloat8E5M2), 8, static_cast<uint16_t>(dtype_lanes)};
    }
#define MLC_DTYPE_PARSE_(str, prefix, prefix_len, dtype_code)                                                          \
  if (str.length() >= prefix_len && str.compare(0, prefix_len, prefix) == 0) {                                         \
    int64_t dtype_bits = StrToInt(str, prefix_len);                                                                    \
    if (dtype_bits < 0 || dtype_bits > u8_max) {                                                                       \
      throw std::runtime_error("Invalid DLDataType");                                                                  \
    }                                                                                                                  \
    return {static_cast<uint8_t>(dtype_code), static_cast<uint8_t>(dtype_bits), static_cast<uint16_t>(dtype_lanes)};   \
  }
    MLC_DTYPE_PARSE_(dtype_str, "int", 3, kDLInt)
    MLC_DTYPE_PARSE_(dtype_str, "uint", 4, kDLUInt)
    MLC_DTYPE_PARSE_(dtype_str, "float", 5, kDLFloat)
    MLC_DTYPE_PARSE_(dtype_str, "ptr", 3, kDLOpaqueHandle)
    MLC_DTYPE_PARSE_(dtype_str, "bfloat", 6, kDLBfloat)
    MLC_DTYPE_PARSE_(dtype_str, "complex", 7, kDLComplex)
#undef MLC_DTYPE_PARSE_
  } catch (...) {
  }
  MLC_THROW(ValueError) << "Cannot convert to `dtype` from string: " << source;
  MLC_UNREACHABLE();
}

} // namespace base
} // namespace mlc

#endif // MLC_BASE_TRAITS_DTYPE_H_
