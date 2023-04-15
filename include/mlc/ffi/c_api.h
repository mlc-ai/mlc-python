#ifndef MLC_C_FFI_ABI_H_
#define MLC_C_FFI_ABI_H_

#include <dlpack/dlpack.h>
#include <stdint.h>

#if !defined(MLC_API) && defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#define MLC_API EMSCRIPTEN_KEEPALIVE
#endif
#if !defined(MLC_API) && defined(_MSC_VER)
#ifdef MLC_EXPORTS
#define MLC_API __declspec(dllexport)
#else
#define MLC_API __declspec(dllimport)
#endif
#endif
#ifndef MLC_API
#define MLC_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  // TODO: 1) add complete set of fp8 support; 2) allow more flexible dtype definition
  kDLDataTypeFloat8E4M3FN = 7,
  kDLDataTypeFloat8E5M2 = 8,
} DLDataTypeCodeExtension;

#ifdef __cplusplus
enum MLCTypeIndex : int32_t {
#else
typedef enum {
#endif
  // [Section] On-stack POD Types: [0, kMLCStaticObjectBegin)
  // N.B. `kMLCRawStr` is a string backed by a `\0`-terminated char array,
  // which is not owned by MLCAny. It is required that the following
  // invariant holds:
  // - `Any::type_index` is never `kMLCRawStr`
  // - `AnyView::type_index` can be `kMLCRawStr`
  kMLCNone = 0,
  kMLCInt = 1,
  kMLCFloat = 2,
  kMLCPtr = 3,
  kMLCDataType = 4,
  kMLCDevice = 5,
  kMLCRawStr = 6,
  // [Section] Static Boxed: [kMLCStaticObjectBegin, kMLCDynObjectBegin)
  kMLCStaticObjectBegin = 64,
  kMLCObject = 64,
  kMLCList = 65,
  kMLCDict = 66,
  kMLCError = 67,
  kMLCFunc = 68,
  kMLCStr = 69,
  // [Section] Dynamic Boxed: [kMLCDynObjectBegin, +oo)
  kMLCDynObjectBegin = 128,
#ifdef __cplusplus
};
#else
} MLCTypeIndex;
#endif

struct MLCAny;
typedef struct MLCAny MLCObject;
typedef void (*MLCDeleterType)(void *);
typedef void (*MLCFuncCallType)(const void *self, int32_t num_args, const MLCAny *args, MLCAny *ret);
typedef int32_t (*MLCFuncSafeCallType)(const void *self, int32_t num_args, const MLCAny *args, MLCAny *ret);

typedef struct MLCAny {
  int32_t type_index;
  union {              // 4 bytes
    int32_t ref_cnt;   // reference counter for heap object
    int32_t small_len; // length for on-stack object
  };
  union {                   // 8 bytes
    int64_t v_int64;        // integers
    double v_float64;       // floating-point numbers
    DLDataType v_dtype;     // data type
    DLDevice v_device;      // device
    void *v_ptr;            // typeless pointers
    const char *v_str;      // raw string
    MLCObject *v_obj;       // ref counted objects
    MLCDeleterType deleter; // Deleter of the object
    char v_bytes[8];        // small string
  };
} MLCAny;

typedef struct {
  MLCObject *ptr;
} MLCObjPtr;

typedef struct {
  int64_t num_bytes;
  const char *bytes;
} MLCByteArray;

typedef struct {
  MLCAny _mlc_header;
  const char *kind;
} MLCError;

typedef struct {
  MLCAny _mlc_header;
  int64_t length;
  char *data;
} MLCStr;

typedef struct {
  MLCAny _mlc_header;
  MLCFuncCallType call;
  MLCFuncSafeCallType safe_call;
} MLCFunc;

typedef struct {
  MLCAny _mlc_header;
  int64_t capacity;
  int64_t size;
  void *data;
} MLCList;

typedef struct {
  MLCAny _mlc_header;
  int64_t capacity;
  int64_t size;
  void *data;
} MLCDict;

typedef int32_t (*MLCAttrGetter)(void *addr, MLCAny *);
typedef int32_t (*MLCAttrSetter)(void *addr, MLCAny *);

typedef struct {
  const char *name;
  int64_t offset;
  MLCAttrGetter getter;
  MLCAttrSetter setter;
} MLCTypeField;

typedef struct {
  const char *name;
  MLCFunc *func;
} MLCTypeMethod;

typedef struct {
  int32_t type_index;
  const char *type_key;
  int32_t type_depth;
  int32_t *type_ancestors; // Range: [0, type_depth)
  MLCTypeField *fields;    // Ends with a field with name == nullptr
  MLCTypeMethod *methods;  // Ends with a method with name == nullptr
} MLCTypeInfo;

typedef void *MLCTypeTableHandle;
MLC_API MLCAny MLCGetLastError();
MLC_API int32_t MLCAnyIncRef(MLCAny *any);
MLC_API int32_t MLCAnyDecRef(MLCAny *any);
MLC_API int32_t MLCAnyInplaceViewToOwned(MLCAny *any);
MLC_API int32_t MLCFuncCreate(void *self, MLCDeleterType deleter, MLCFuncSafeCallType safe_call, MLCAny *ret);
MLC_API int32_t MLCFuncSetGlobal(MLCTypeTableHandle self, const char *name, MLCAny func, int allow_override);
MLC_API int32_t MLCFuncGetGlobal(MLCTypeTableHandle self, const char *name, MLCAny *ret);
MLC_API int32_t MLCFuncSafeCall(MLCFunc *func, int32_t num_args, MLCAny *args, MLCAny *ret);
MLC_API int32_t MLCTypeIndex2Info(MLCTypeTableHandle self, int32_t type_index, MLCTypeInfo **out_type_info);
MLC_API int32_t MLCTypeKey2Info(MLCTypeTableHandle self, const char *type_key, MLCTypeInfo **out_type_info);
MLC_API int32_t MLCTypeRegister(MLCTypeTableHandle self, int32_t parent_type_index, const char *type_key,
                                int32_t type_index, MLCTypeInfo **out_type_info);
MLC_API int32_t MLCTypeDefReflection(MLCTypeTableHandle self, int32_t type_index, int64_t num_fields,
                                     MLCTypeField *fields, int64_t num_methods, MLCTypeMethod *methods);
MLC_API int32_t MLCVTableSet(MLCTypeTableHandle self, int32_t type_index, const char *key, MLCAny *value);
MLC_API int32_t MLCVTableGet(MLCTypeTableHandle self, int32_t type_index, const char *key, MLCAny *value);
MLC_API int32_t MLCErrorCreate(const char *kind, int64_t num_bytes, const char *bytes, MLCAny *ret);
MLC_API int32_t MLCErrorGetInfo(MLCAny error, int32_t *num_strs, const char ***strs);
MLC_API MLCByteArray MLCTraceback(const char *filename, const char *lineno, const char *func_name);
#ifdef __cplusplus
} // MLC_EXTERN_C
#endif

#endif // MLC_C_FFI_ABI_H_
