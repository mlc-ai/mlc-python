#ifndef MLC_BASE_TRAITS_DTYPE_H_
#define MLC_BASE_TRAITS_DTYPE_H_

#include "./lib.h"
#include "./utils.h"

namespace mlc {
namespace base {

inline DLDataType DataTypeFromStr(const char *source);

inline bool DataTypeEqual(DLDataType a, DLDataType b) {
  return a.code == b.code && a.bits == b.bits && a.lanes == b.lanes;
}
inline const char *DataTypeCode2Str(int32_t type_code) { return ::mlc::Lib::DataTypeCodeToStr(type_code); }

inline int32_t DataTypeSize(DLDataType dtype) {
  int32_t bits = static_cast<int32_t>(dtype.bits);
  int32_t lanes = static_cast<int32_t>(dtype.lanes);
  return ((bits + 7) / 8) * lanes;
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
      return DataTypeFromStr(v->v.v_str);
    }
    if (ty == MLCTypeIndex::kMLCStr) {
      return DataTypeFromStr(reinterpret_cast<const MLCStr *>(v->v.v_obj)->data);
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

inline DLDataType DataTypeFromStr(const char *source) { return ::mlc::Lib::DataTypeFromStr(source); }
inline std::string DataTypeToStr(DLDataType dtype) { return TypeTraits<DLDataType>::__str__(dtype); }

} // namespace base
} // namespace mlc

#endif // MLC_BASE_TRAITS_DTYPE_H_
