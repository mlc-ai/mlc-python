#ifndef MLC_BASE_TRAITS_DEVICE_H_
#define MLC_BASE_TRAITS_DEVICE_H_

#include "./lib.h"
#include "./utils.h"

namespace mlc {
namespace base {

DLDevice DeviceFromStr(const std::string &source);

inline bool DeviceEqual(DLDevice a, DLDevice b) { return a.device_type == b.device_type && a.device_id == b.device_id; }
inline const char *DeviceType2Str(int32_t device_type) { return ::mlc::Lib::DeviceTypeToStr(device_type); }

template <> struct TypeTraits<DLDevice> {
  static constexpr int32_t type_index = static_cast<int32_t>(MLCTypeIndex::kMLCDevice);
  static constexpr const char *type_str = "Device";

  MLC_INLINE static void TypeToAny(DLDevice src, MLCAny *ret) {
    ret->type_index = static_cast<int32_t>(MLCTypeIndex::kMLCDevice);
    ret->v.v_device = src;
  }

  MLC_INLINE static DLDevice AnyToTypeOwned(const MLCAny *v) {
    MLCTypeIndex ty = static_cast<MLCTypeIndex>(v->type_index);
    if (ty == MLCTypeIndex::kMLCDevice) {
      return v->v.v_device;
    }
    if (ty == MLCTypeIndex::kMLCRawStr) {
      return DeviceFromStr(v->v.v_str);
    }
    if (ty == MLCTypeIndex::kMLCStr) {
      return DeviceFromStr(reinterpret_cast<const MLCStr *>(v->v.v_obj)->data);
    }
    throw TemporaryTypeError();
  }

  MLC_INLINE static DLDevice AnyToTypeUnowned(const MLCAny *v) { return AnyToTypeOwned(v); }

  MLC_INLINE static std::string __str__(DLDevice device) {
    std::ostringstream os;
    os << DeviceType2Str(static_cast<DLDeviceType>(device.device_type)) << ":" << device.device_id;
    return os.str();
  }
};

inline DLDevice DeviceFromStr(const std::string &source) {
  constexpr int64_t i32_max = 2147483647;
  int32_t device_type;
  int64_t device_id = 0;
  try {
    if (size_t c_pos = source.rfind(':'); c_pos != std::string::npos) {
      device_type = ::mlc::Lib::DeviceTypeFromStr(source.substr(0, c_pos).c_str());
      device_id = StrToInt(source, c_pos + 1);
    } else {
      device_type = ::mlc::Lib::DeviceTypeFromStr(source.c_str());
      device_id = 0;
    }
    if (device_type < 0 || device_id < 0 || device_id > i32_max) {
      throw std::runtime_error(""); // Going to catch it below
    }
    return DLDevice{static_cast<DLDeviceType>(device_type), static_cast<int32_t>(device_id)};
  } catch (...) {
  }
  MLC_THROW(ValueError) << "Cannot convert to `Device` from string: " << source;
  MLC_UNREACHABLE();
}

inline std::string DeviceToStr(DLDevice device) { return TypeTraits<DLDevice>::__str__(device); }

} // namespace base
} // namespace mlc

#endif // MLC_BASE_TRAITS_DEVICE_H_
