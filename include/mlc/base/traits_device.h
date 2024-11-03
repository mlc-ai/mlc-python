#ifndef MLC_BASE_TRAITS_DEVICE_H_
#define MLC_BASE_TRAITS_DEVICE_H_

#include "./utils.h"
#include <unordered_map>

namespace mlc {
namespace base {

const char *DLDeviceType2Str(DLDeviceType type);
DLDevice String2DLDevice(const std::string &source);
inline bool DeviceEqual(DLDevice a, DLDevice b) { return a.device_type == b.device_type && a.device_id == b.device_id; }

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
      return String2DLDevice(v->v.v_str);
    }
    if (ty == MLCTypeIndex::kMLCStr) {
      return String2DLDevice(reinterpret_cast<const MLCStr *>(v->v.v_obj)->data);
    }
    throw TemporaryTypeError();
  }

  MLC_INLINE static DLDevice AnyToTypeUnowned(const MLCAny *v) { return AnyToTypeOwned(v); }

  MLC_INLINE static std::string __str__(DLDevice device) {
    std::ostringstream os;
    os << DLDeviceType2Str(static_cast<DLDeviceType>(device.device_type)) << ":" << device.device_id;
    return os.str();
  }

  static inline MLC_SYMBOL_HIDE std::unordered_map<std::string, DLDeviceType> str2device_type = {
      {"cpu", kDLCPU},
      {"cuda", kDLCUDA},
      {"cuda_host", kDLCUDAHost},
      {"opencl", kDLOpenCL},
      {"vulkan", kDLVulkan},
      {"mps", kDLMetal},
      {"vpi", kDLVPI},
      {"rocm", kDLROCM},
      {"rocm_host", kDLROCMHost},
      {"ext_dev", kDLExtDev},
      {"cuda_managed", kDLCUDAManaged},
      {"oneapi", kDLOneAPI},
      {"webgpu", kDLWebGPU},
      {"hexagon", kDLHexagon},
      {"maia", kDLMAIA},
      // aliases
      {"llvm", kDLCPU},
      {"nvptx", kDLCUDA},
      {"cl", kDLOpenCL},
      {"sdaccel", kDLOpenCL},
      {"metal", kDLMetal},
  };
};

MLC_INLINE const char *DLDeviceType2Str(DLDeviceType type) {
  switch (type) {
  case kDLCPU:
    return "cpu";
  case kDLCUDA:
    return "cuda";
  case kDLCUDAHost:
    return "cuda_host";
  case kDLOpenCL:
    return "opencl";
  case kDLVulkan:
    return "vulkan";
  case kDLMetal:
    return "mps";
  case kDLVPI:
    return "vpi";
  case kDLROCM:
    return "rocm";
  case kDLROCMHost:
    return "rocm_host";
  case kDLExtDev:
    return "ext_dev";
  case kDLCUDAManaged:
    return "cuda_managed";
  case kDLOneAPI:
    return "oneapi";
  case kDLWebGPU:
    return "webgpu";
  case kDLHexagon:
    return "hexagon";
  case kDLMAIA:
    return "maia";
  }
  return "unknown";
}

inline DLDevice String2DLDevice(const std::string &source) {
  constexpr int64_t i32_max = 2147483647;
  using Traits = TypeTraits<DLDevice>;
  DLDeviceType device_type;
  int64_t device_id = 0;
  try {
    if (size_t c_pos = source.rfind(':'); c_pos != std::string::npos) {
      device_type = Traits::str2device_type.at(source.substr(0, c_pos));
      device_id = StrToInt(source, c_pos + 1);
    } else {
      device_type = Traits::str2device_type.at(source);
    }
    if (device_id < 0 || device_id > i32_max) {
      throw std::runtime_error("Invalid device id");
    }
    return DLDevice{device_type, static_cast<int32_t>(device_id)};
  } catch (...) {
  }
  MLC_THROW(ValueError) << "Cannot convert to `Device` from string: " << source;
  MLC_UNREACHABLE();
}

} // namespace base
} // namespace mlc

#endif // MLC_BASE_TRAITS_DEVICE_H_
