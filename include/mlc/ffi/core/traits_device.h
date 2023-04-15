#ifndef MLC_TRAITS_DEVICE_H_
#define MLC_TRAITS_DEVICE_H_

#include "./utils.h"
#include <unordered_map>

namespace mlc {
namespace ffi {

const char *DLDeviceType2Str(DLDeviceType type);
DLDevice String2DLDevice(const std::string &source);
inline bool DeviceEqual(DLDevice a, DLDevice b) { return a.device_type == b.device_type && a.device_id == b.device_id; }

template <> struct PODTraits<DLDevice> {
  MLC_INLINE static void TypeCopyToAny(DLDevice src, MLCAny *ret) {
    ret->type_index = static_cast<int32_t>(MLCTypeIndex::kMLCDevice);
    ret->v_device = src;
  }

  MLC_INLINE static DLDevice AnyCopyToType(const MLCAny *v) {
    MLCTypeIndex type_index = static_cast<MLCTypeIndex>(v->type_index);
    if (type_index == MLCTypeIndex::kMLCDevice) {
      return v->v_device;
    }
    if (type_index == MLCTypeIndex::kMLCRawStr) {
      return String2DLDevice(v->v_str);
    }
    if (type_index == MLCTypeIndex::kMLCStr) {
      return String2DLDevice(reinterpret_cast<const MLCStr *>(v->v_obj)->data);
    }
    throw TemporaryTypeError();
  }

  MLC_INLINE static const char *Type2Str() { return "Device"; }

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
  using Traits = PODTraits<DLDevice>;
  DLDeviceType device_type;
  int32_t device_id = 0;
  try {
    if (size_t c_pos = source.rfind(':'); c_pos != std::string::npos) {
      device_type = Traits::str2device_type.at(source.substr(0, c_pos));
      device_id = static_cast<int32_t>(std::stoi(&source[c_pos + 1]));
    } else {
      device_type = Traits::str2device_type.at(source);
    }
    return DLDevice{device_type, device_id};
  } catch (...) {
  }
  MLC_THROW(ValueError) << "Cannot convert to `Device` from string: " << source;
  MLC_UNREACHABLE();
}

} // namespace ffi
} // namespace mlc

#endif // MLC_TRAITS_DEVICE_H_
