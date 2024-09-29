#ifndef MLC_CPP_TRACEBACK_H_
#define MLC_CPP_TRACEBACK_H_

#include <cstring>
#include <inttypes.h>
#include <string>
#include <vector>

namespace mlc {
namespace ffi {

inline const char *StringifyPointer(uintptr_t ptr, char *buffer, size_t buffer_size) {
  snprintf(buffer, buffer_size, "0x%016" PRIxPTR, ptr);
  return buffer;
}

inline bool EndsWith(const char *str, const char *suffix) {
  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);
  return str_len >= suffix_len && strcmp(str + str_len - suffix_len, suffix) == 0;
}

inline bool StartsWith(const char *str, const char *prefix) {
  size_t str_len = strlen(str);
  size_t prefix_len = strlen(prefix);
  return str_len >= prefix_len && strncmp(str, prefix, prefix_len) == 0;
}

inline bool IsForeignFrame(const char *filename, int32_t lineno, const char *func_name) {
  (void)lineno;
  (void)func_name;
  if (EndsWith(filename, "core_cython.cc")) {
    return true;
  }
#if defined(__APPLE__)
  if (strcmp(func_name, "MLCFuncSafeCall") == 0) {
    return true;
  }
#endif
  return false;
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // std::getenv is unsafe
#endif

inline int32_t GetTracebackLimit() {
  if (const char *env = std::getenv("MLC_TRACEBACK_LIMIT")) {
    return std::stoi(env);
  }
  return 512;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

struct TracebackStorage {
  std::vector<char> buffer = std::vector<char>(1024 * 1024, '\0'); // 1 MB
  char lineno_buffer[32];

  TracebackStorage *Append(const char *str) {
    buffer.insert(buffer.end(), str, str + strlen(str));
    buffer.push_back('\0');
    return this;
  }

  TracebackStorage *Append(int lineno) {
    snprintf(lineno_buffer, sizeof(lineno_buffer), "%d", lineno);
    return this->Append(lineno_buffer);
  }
};

} // namespace ffi
} // namespace mlc

#endif // MLC_CPP_TRACEBACK_H_
