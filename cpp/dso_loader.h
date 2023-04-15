#ifndef MLC_DSO_LIBRARY_H_
#define MLC_DSO_LIBRARY_H_

#ifdef _MSC_VER
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <mlc/ffi/core/core.h>

namespace mlc {
namespace ffi {

struct DSOLibrary {
  ~DSOLibrary() { Unload(); }

#ifdef _MSC_VER
  DSOLibrary(std::string name) {
    std::wstring wname(name.begin(), name.end());
    lib_handle_ = LoadLibraryW(wname.c_str());
    if (lib_handle_ == nullptr) {
      MLC_THROW(ValueError) << "Failed to load dynamic shared library " << name;
    }
  }

  void Unload() {
    if (lib_handle_ != nullptr) {
      FreeLibrary(lib_handle_);
      lib_handle_ = nullptr;
    }
  }

  void *GetSymbol_(const char *name) {
    return reinterpret_cast<void *>(GetProcAddress(lib_handle_, (LPCSTR)name)); // NOLINT(*)
  }

  HMODULE lib_handle_{nullptr};
#else
  DSOLibrary(std::string name) {
    lib_handle_ = dlopen(name.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (lib_handle_ == nullptr) {
      MLC_THROW(ValueError) << "Failed to load dynamic shared library " << name << " " << dlerror();
    }
  }

  void Unload() {
    if (lib_handle_ != nullptr) {
      dlclose(lib_handle_);
      lib_handle_ = nullptr;
    }
  }

  void *GetSymbol(const char *name) { return dlsym(lib_handle_, name); }

  void *lib_handle_{nullptr};
#endif
};

} // namespace ffi
} // namespace mlc

#endif // MLC_DSO_LIBRARY_H_
