#ifndef MLC_BASE_TRAITS_DTYPE_H_
#define MLC_BASE_TRAITS_DTYPE_H_

#include "./lib.h"
#include "./utils.h"

namespace mlc {
namespace base {

inline const char *DataTypeCode2Str(int32_t type_code) { return ::mlc::Lib::DataTypeCodeToStr(type_code); }

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
      return ::mlc::Lib::DataTypeFromStr(v->v.v_str);
    }
    if (ty == MLCTypeIndex::kMLCStr) {
      return ::mlc::Lib::DataTypeFromStr(reinterpret_cast<const MLCStr *>(v->v.v_obj)->data);
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
    os << DataTypeCode2Str(code);
    if (code < kMLCExtension_DLDataTypeCode_Begin) {
      // for `code >= kMLCExtension_DLDataTypeCode_Begin`, the `bits` is already encoded in `code`
      os << bits;
    }
    if (lanes != 1) {
      os << "x" << lanes;
    }
    return os.str();
  }
};

struct DType {
  static DLDataType Int(int bits, int lanes = 1) {
    DLDataType dtype;
    dtype.code = kDLInt;
    dtype.bits = static_cast<uint8_t>(bits);
    dtype.lanes = static_cast<uint16_t>(lanes);
    return dtype;
  }
  static DLDataType UInt(int bits, int lanes = 1) {
    DLDataType dtype;
    dtype.code = kDLUInt;
    dtype.bits = static_cast<uint8_t>(bits);
    dtype.lanes = static_cast<uint16_t>(lanes);
    return dtype;
  }
  static DLDataType Float(int bits, int lanes = 1) {
    DLDataType dtype;
    dtype.code = kDLFloat;
    dtype.bits = static_cast<uint8_t>(bits);
    dtype.lanes = static_cast<uint16_t>(lanes);
    return dtype;
  }
  static DLDataType Bool(int lanes = 1) {
    DLDataType dtype;
    dtype.code = kDLUInt;
    dtype.bits = 1;
    dtype.lanes = static_cast<uint16_t>(lanes);
    return dtype;
  }
  static DLDataType Void() {
    DLDataType dtype;
    dtype.code = kDLOpaqueHandle;
    dtype.bits = 0;
    dtype.lanes = 0;
    return dtype;
  }
  static bool Equal(DLDataType a, DLDataType b) { return a.code == b.code && a.bits == b.bits && a.lanes == b.lanes; }
  static bool IsBool(DLDataType dtype) { return dtype.code == kDLUInt && dtype.bits == 1; }
  static bool IsFloat(DLDataType dtype) {
    // TODO: handle fp8
    return dtype.code == kDLFloat || dtype.code == kDLBfloat;
  }
  static std::string Str(DLDataType dtype) { return TypeTraits<DLDataType>::__str__(dtype); }
  static int32_t Size(DLDataType dtype) {
    int32_t bits = static_cast<int32_t>(dtype.bits);
    int32_t lanes = static_cast<int32_t>(dtype.lanes);
    return ((bits + 7) / 8) * lanes;
  }
};

} // namespace base
} // namespace mlc

#endif // MLC_BASE_TRAITS_DTYPE_H_
