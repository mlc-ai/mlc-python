#ifndef MLC_TRAITS_DTYPE_H_
#define MLC_TRAITS_DTYPE_H_

#include "./utils.h"
#include <unordered_map>

namespace mlc {
namespace ffi {

inline const char *DLDataTypeCode2Str(int32_t type_code);
inline DLDataType String2DLDataType(const std::string &source);
inline bool DataTypeEqual(DLDataType a, DLDataType b) {
  return a.code == b.code && a.bits == b.bits && a.lanes == b.lanes;
}

template <> struct PODTraits<DLDataType> {
  MLC_INLINE static void TypeCopyToAny(DLDataType src, MLCAny *ret) {
    ret->type_index = static_cast<int32_t>(MLCTypeIndex::kMLCDataType);
    ret->v_dtype = src;
  }

  MLC_INLINE static DLDataType AnyCopyToType(const MLCAny *v) {
    MLCTypeIndex type_index = static_cast<MLCTypeIndex>(v->type_index);
    if (type_index == MLCTypeIndex::kMLCDataType) {
      return v->v_dtype;
    }
    if (type_index == MLCTypeIndex::kMLCRawStr) {
      return String2DLDataType(v->v_str);
    }
    if (type_index == MLCTypeIndex::kMLCStr) {
      return String2DLDataType(reinterpret_cast<const MLCStr *>(v->v_obj)->data);
    }
    throw TemporaryTypeError();
  }

  MLC_INLINE static const char *Type2Str() { return "dtype"; }

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
  using Traits = PODTraits<DLDataType>;
  if (auto it = Traits::preset.find(source); it != Traits::preset.end()) {
    return it->second;
  }
#define MLC_DTYPE_PARSE(str, dtype_lanes, prefix, prefix_len, dtype_code)                                              \
  if (str.length() >= prefix_len && str.compare(0, prefix_len, prefix) == 0) {                                         \
    return {static_cast<uint8_t>(dtype_code), static_cast<uint8_t>(std::stoi(str.substr(prefix_len))),                 \
            static_cast<uint16_t>(dtype_lanes)};                                                                       \
  }
  try {
    uint16_t lanes = 1;
    std::string dtype_str;
    if (size_t x_pos = source.rfind('x'); x_pos != std::string::npos) {
      dtype_str = source.substr(0, x_pos);
      lanes = static_cast<uint16_t>(std::stoi(&source[x_pos + 1]));
    } else {
      dtype_str = source;
    }
    if (dtype_str == "float8_e4m3fn") {
      return {kDLDataTypeFloat8E4M3FN, 8, lanes};
    }
    if (dtype_str == "float8_e5m2") {
      return {kDLDataTypeFloat8E5M2, 8, lanes};
    }
    MLC_DTYPE_PARSE(dtype_str, lanes, "int", 3, kDLInt)
    MLC_DTYPE_PARSE(dtype_str, lanes, "uint", 4, kDLUInt)
    MLC_DTYPE_PARSE(dtype_str, lanes, "float", 5, kDLFloat)
    MLC_DTYPE_PARSE(dtype_str, lanes, "ptr", 3, kDLOpaqueHandle)
    MLC_DTYPE_PARSE(dtype_str, lanes, "bfloat", 6, kDLBfloat)
    MLC_DTYPE_PARSE(dtype_str, lanes, "complex", 7, kDLComplex)
  } catch (...) {
  }
  MLC_THROW(ValueError) << "Cannot convert to `dtype` from string: " << source;
  MLC_UNREACHABLE();
#undef MLC_DTYPE_PARSE
}

} // namespace ffi
} // namespace mlc

#endif // MLC_TRAITS_DTYPE_H_
