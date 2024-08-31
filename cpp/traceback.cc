#ifndef _MSC_VER
#include "./traceback.h"
#include <backtrace.h>
#include <cxxabi.h>
#include <iostream>
#include <mlc/c_api.h>

namespace mlc {
namespace ffi {
namespace {

static int32_t MLC_TRACEBACK_LIMIT = GetTracebackLimit();
static backtrace_state *_bt_state = backtrace_create_state(
    /*filename=*/nullptr,
    /*threaded=*/1,
    /*error_callback=*/
    +[](void * /*data*/, const char *msg, int /*errnum*/) -> void {
      std::cerr << "Failed to initialize libbacktrace: " << msg << std::endl;
    },
    /*data=*/nullptr);
thread_local size_t cxa_length = 1024;
thread_local char *cxa_buffer = static_cast<char *>(std::malloc(cxa_length * sizeof(char)));
thread_local char number_buffer[32];
thread_local TracebackStorage storage;

const char *CxaDemangle(const char *name) {
  int status = 0;
  size_t length = cxa_length;
  char *demangled_name = abi::__cxa_demangle(name, cxa_buffer, &length, &status);
  if (length > cxa_length) {
    cxa_length = length;
    cxa_buffer = demangled_name;
  }
  return (demangled_name && length && !status) ? demangled_name : name;
}

MLCByteArray TracebackImpl() {
  storage.buffer.clear();

  auto callback = +[](void *data, uintptr_t pc, const char *filename, int lineno, const char *symbol) -> int {
    if (!filename) {
      filename = "<unknown>";
    }
    if (!symbol) {
      backtrace_syminfo(
          /*state=*/_bt_state, /*addr=*/pc, /*callback=*/
          +[](void *data, uintptr_t /*pc*/, const char *symname, uintptr_t /*symval*/, uintptr_t /*symsize*/) {
            *reinterpret_cast<const char **>(data) = symname;
          },
          /*error_callback=*/+[](void * /*data*/, const char * /*msg*/, int /*errnum*/) {},
          /*data=*/&symbol);
    }
    symbol = symbol ? CxaDemangle(symbol) : StringifyPointer(pc, number_buffer, sizeof(number_buffer));
    if (IsForeignFrame(filename, lineno, symbol)) {
      return 1;
    }
    reinterpret_cast<TracebackStorage *>(data)->Append(filename)->Append(lineno)->Append(symbol);
    return 0;
  };
  backtrace_full(
      /*state=*/_bt_state, /*skip=*/1, /*callback=*/callback,
      /*error_callback=*/[](void * /*data*/, const char * /*msg*/, int /*errnum*/) {},
      /*data=*/&storage);
  return {static_cast<int64_t>(storage.buffer.size()), storage.buffer.data()};
}

} // namespace
} // namespace ffi
} // namespace mlc

MLC_API MLCByteArray MLCTraceback(const char *, const char *, const char *) {
  using namespace mlc::ffi;
  if (_bt_state) {
    return TracebackImpl();
  }
  return {0, nullptr};
}
#endif // _MSC_VER
