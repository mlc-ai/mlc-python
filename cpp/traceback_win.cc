#ifdef _MSC_VER
// clang-format off
#include <windows.h>
#include <dbghelp.h>
// clang-format on
#include "./traceback.h"
#include <mlc/c_api.h>

namespace mlc {
namespace ffi {
namespace {

static int32_t MLC_TRACEBACK_LIMIT = GetTracebackLimit();
thread_local TracebackStorage storage;
thread_local char number_buffer[32];
thread_local char undecorated_name[1024];
thread_local char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
#pragma comment(lib, "dbghelp.lib")

thread_local std::pair<HANDLE, HANDLE> process_thread = []() {
  HANDLE process = GetCurrentProcess();
  HANDLE thread = GetCurrentThread();
  SymInitialize(process, NULL, TRUE);
  SymSetOptions(SYMOPT_LOAD_LINES);
  return std::make_pair(process, thread);
}();

MLCByteArray TracebackImpl() {
  HANDLE process = process_thread.first;
  HANDLE thread = process_thread.second;
  CONTEXT context;
  STACKFRAME64 frame;
  DWORD machine;

  memset(&context, 0, sizeof(CONTEXT));
  context.ContextFlags = CONTEXT_FULL;
  RtlCaptureContext(&context);

  memset(&frame, 0, sizeof(STACKFRAME64));
#ifdef _M_IX86
  machine = IMAGE_FILE_MACHINE_I386;
  frame.AddrPC.Offset = context.Eip;
  frame.AddrStack.Offset = context.Esp;
  frame.AddrFrame.Offset = context.Ebp;
#elif _M_X64
  machine = IMAGE_FILE_MACHINE_AMD64;
  frame.AddrPC.Offset = context.Rip;
  frame.AddrStack.Offset = context.Rsp;
  frame.AddrFrame.Offset = context.Rbp;
#elif _M_ARM64
  machine = IMAGE_FILE_MACHINE_ARM64;
  frame.AddrPC.Offset = context.Pc;
  frame.AddrStack.Offset = context.Sp;
  frame.AddrFrame.Offset = context.Fp;
#else
#error "Unsupported architecture"
#endif
  frame.AddrPC.Mode = AddrModeFlat;
  frame.AddrStack.Mode = AddrModeFlat;
  frame.AddrFrame.Mode = AddrModeFlat;
  storage.buffer.clear();
  while (StackWalk64(machine, process, thread, &frame, &context, NULL, SymFunctionTableAccess64, SymGetModuleBase64,
                     NULL)) {
    PSYMBOL_INFO symbol = reinterpret_cast<PSYMBOL_INFO>(symbol_buffer);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    const char *filename = "<unknown>";
    int lineno = 0;
    const char *symbol_name;

    IMAGEHLP_LINE64 line;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD displacement_line = 0;
    if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &displacement_line, &line)) {
      filename = line.FileName;
      lineno = line.LineNumber;
    }
    DWORD64 displacement = 0;
    if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol)) {
      symbol_name = UnDecorateSymbolName(symbol->Name, undecorated_name, sizeof(undecorated_name), UNDNAME_COMPLETE)
                        ? undecorated_name
                        : symbol->Name;
    } else {
      symbol_name = StringifyPointer(frame.AddrPC.Offset, number_buffer, sizeof(number_buffer));
    }
    if (IsForeignFrame(filename, lineno, symbol_name)) {
      break;
    }
    if (!EndsWith(filename, "traceback_win.cc")) {
      storage.Append(filename);
      storage.Append(lineno);
      storage.Append(symbol_name);
    }
  }
  return {static_cast<int64_t>(storage.buffer.size()), storage.buffer.data()};
}

} // namespace
} // namespace ffi
} // namespace mlc

MLC_API MLCByteArray MLCTraceback(const char *, const char *, const char *) { return ::mlc::ffi::TracebackImpl(); }
#endif // _MSC_VER
