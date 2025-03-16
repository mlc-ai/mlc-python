#ifndef MLC_C_FFI_ABI_H_
#define MLC_C_FFI_ABI_H_

#include <dlpack/dlpack.h> // IWYU pragma: export
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

#ifndef MLC_EXPORTS
#define MLC_EXPORTS 0
#else
#undef MLC_EXPORTS
#define MLC_EXPORTS 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { // ranged [0, 2 ** 8)
  kMLCExtension_DLDataTypeCode_Begin = 7,
  // 8-bit floating point representations
  kDLDataTypeFloat8Begin = 7,
  kDLDataTypeFloat8E3M4 = 7,
  kDLDataTypeFloat8E4M3 = 8,
  kDLDataTypeFloat8E4M3B11FNUZ = 9,
  kDLDataTypeFloat8E4M3FN = 10,
  kDLDataTypeFloat8E4M3FNUZ = 11,
  kDLDataTypeFloat8E5M2 = 12,
  kDLDataTypeFloat8E5M2FNUZ = 13,
  kDLDataTypeFloat8E8M0FNU = 14,
  kDLDataTypeFloat8End = 15,
  // Microscaling (MX) sub-byte floating point representations
  kDLDataTypeFloat4E2M1FN = 15, // higher 4 bits are unused
  kDLDataTypeFloat6E2M3FN = 16, // higher 2 bits are unused
  kDLDataTypeFloat6E3M2FN = 17, // higher 2 bits are unused
  kMLCExtension_DLDataTypeCode_End = kDLDataTypeFloat6E3M2FN,
} MLCExtension_DLDataTypeCode;

typedef enum { // ranged [0, 2 ** 32)
  kMLCExtension_DLDeviceType_Begin = 18,
  kMLCExtension_DLDeviceType_End = kMLCExtension_DLDeviceType_Begin,
} MLCExtension_DLDeviceType;

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
  kMLCBool = 1,
  kMLCInt = 2,
  kMLCFloat = 3,
  kMLCPtr = 4,
  kMLCDataType = 5,
  kMLCDevice = 6,
  kMLCRawStr = 7,
  kMLCStaticObjectBegin = 1000,
  // kMLCCore [1000: 1100) {
  kMLCCoreBegin = 1000,
  kMLCObject = 1000,
  kMLCList = 1001,
  kMLCDict = 1002,
  kMLCError = 1003,
  kMLCFunc = 1004,
  kMLCStr = 1005,
  kMLCTensor = 1006,
  kMLCOpaque = 1007,
  kMLCCoreEnd = 1100,
  // }
  // kMLCTyping [1100: 1200) {
  kMLCTypingBegin = 1100,
  kMLCTyping = 1100,
  kMLCTypingAny = 1101,
  kMLCTypingAtomic = 1102,
  kMLCTypingPtr = 1103,
  kMLCTypingOptional = 1104,
  kMLCTypingList = 1105,
  kMLCTypingDict = 1106,
  kMLCTypingEnd = 1200,
  // }
  // [Section] Dynamic Boxed: [kMLCDynObjectBegin, +oo)
  kMLCDynObjectBegin = 100000,
#ifdef __cplusplus
};
#else
} MLCTypeIndex;
#endif

struct MLCAny;
typedef void (*MLCDeleterType)(void *);
typedef void (*MLCFuncCallType)(const void *self, int32_t num_args, const MLCAny *args, MLCAny *ret);
typedef int32_t (*MLCFuncSafeCallType)(const void *self, int32_t num_args, const MLCAny *args, MLCAny *ret);

typedef struct {
  MLCAny *ptr;
} MLCObjPtr;

typedef union {
  bool v_bool;            // booleans
  int64_t v_int64;        // integers
  double v_float64;       // floating-point numbers
  DLDataType v_dtype;     // data type
  DLDevice v_device;      // device
  void *v_ptr;            // typeless pointers
  const char *v_str;      // raw string
  MLCAny *v_obj;          // ref counted objects
  MLCDeleterType deleter; // Deleter of the object
  char v_bytes[8];        // small string
} MLCPODValueUnion;

typedef struct MLCAny {
  int32_t type_index;  // 4 bytes
  union {              // 4 bytes
    int32_t ref_cnt;   // reference counter for heap object
    int32_t small_len; // length for on-stack object
  };
  MLCPODValueUnion v; // 8 bytes
} MLCAny;

typedef struct {
  MLCAny _mlc_header;
  MLCPODValueUnion data;
} MLCBoxedPOD;

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

typedef struct {
  MLCAny _mlc_header;
  DLTensor tensor;
  void *manager_ctx;
} MLCTensor;

typedef struct {
  MLCAny _mlc_header;
  void *handle;
  MLCDeleterType handle_deleter;
  const char *opaque_type_name;
} MLCOpaque;

typedef struct {
  MLCAny _mlc_header;
} MLCTypingAny;

typedef struct {
  MLCAny _mlc_header;
} MLCTypingNone;

typedef struct {
  MLCAny _mlc_header;
  int32_t type_index;
} MLCTypingAtomic;

typedef struct {
  MLCAny _mlc_header;
  MLCObjPtr ty;
} MLCTypingPtr;

typedef struct {
  MLCAny _mlc_header;
  MLCObjPtr ty;
} MLCTypingOptional;

typedef struct {
  MLCAny _mlc_header;
  MLCObjPtr ty;
} MLCTypingList;

typedef struct {
  MLCAny _mlc_header;
  MLCObjPtr ty_k;
  MLCObjPtr ty_v;
} MLCTypingDict;

typedef struct MLCTypeField {
  const char *name;
  int32_t index;
  int64_t offset;
  int32_t num_bytes;
  int32_t frozen;
  MLCAny *ty;
} MLCTypeField;

typedef struct {
  const char *name;
  MLCFunc *func;
  int32_t kind; // 0: member method; 1: static method
} MLCTypeMethod;

typedef struct MLCTypeInfo {
  int32_t type_index;
  const char *type_key;
  uint64_t type_key_hash;
  int32_t type_depth;
  int32_t *type_ancestors; // Range: [0, type_depth)
  MLCTypeField *fields;    // Ends with a field with name == nullptr
  MLCTypeMethod *methods;  // Ends with a method with name == nullptr
  /*
   * structure_kind = 0: StructureKind::kNone;
   * structure_kind = 1: StructureKind::kNoBind;
   * structure_kind = 2: StructureKind::kBind;
   * structure_kind = 3: StructureKind::kVar.
   */
  int32_t structure_kind;
  int32_t *sub_structure_indices; // Ends with -1
  /*
   * sub_structure_kind = StructureFieldKind::kNoBind;
   * sub_structure_kind = StructureFieldKind::kBind;
   */
  int32_t *sub_structure_kinds; // Ends with -1
} MLCTypeInfo;

typedef void *MLCTypeTableHandle;
typedef void *MLCVTableHandle;
MLC_API MLCAny MLCGetLastError();
MLC_API int32_t MLCAnyIncRef(MLCAny *any);
MLC_API int32_t MLCAnyDecRef(MLCAny *any);
MLC_API int32_t MLCAnyInplaceViewToOwned(MLCAny *any);
MLC_API int32_t MLCHandleGetGlobal(MLCTypeTableHandle *self);
MLC_API int32_t MLCFuncCreate(void *self, MLCDeleterType deleter, MLCFuncSafeCallType safe_call, MLCAny *ret);
MLC_API int32_t MLCFuncSetGlobal(MLCTypeTableHandle self, const char *name, MLCAny func, int allow_override);
MLC_API int32_t MLCFuncGetGlobal(MLCTypeTableHandle self, const char *name, MLCAny *ret);
MLC_API int32_t MLCFuncSafeCall(MLCFunc *func, int32_t num_args, MLCAny *args, MLCAny *ret);
MLC_API int32_t MLCTypeIndex2Info(MLCTypeTableHandle self, int32_t type_index, MLCTypeInfo **out_type_info);
MLC_API int32_t MLCTypeKey2Info(MLCTypeTableHandle self, const char *type_key, MLCTypeInfo **out_type_info);
MLC_API int32_t MLCTypeRegister(MLCTypeTableHandle self, int32_t parent_type_index, const char *type_key,
                                int32_t type_index, MLCTypeInfo **out_type_info);
MLC_API int32_t MLCTypeRegisterFields(MLCTypeTableHandle self, int32_t type_index, int64_t num_fields,
                                      MLCTypeField *fields);
MLC_API int32_t MLCTypeRegisterStructure(MLCTypeTableHandle self, int32_t type_index, int32_t structure_kind,
                                         int64_t num_sub_structures, int32_t *sub_structure_indices,
                                         int32_t *sub_structure_kinds);
MLC_API int32_t MLCTypeAddMethod(MLCTypeTableHandle self, int32_t type_index, MLCTypeMethod method);
MLC_API int32_t MLCVTableCreate(MLCTypeTableHandle self, const char *key, MLCVTableHandle *ret);
MLC_API int32_t MLCVTableDelete(MLCVTableHandle self);
MLC_API int32_t MLCVTableCall(MLCVTableHandle vtable, int32_t num_args, MLCAny *args, MLCAny *ret);
MLC_API int32_t MLCVTableGetGlobal(MLCTypeTableHandle self, const char *key, MLCVTableHandle *ret);
MLC_API int32_t MLCVTableGetFunc(MLCVTableHandle vtable, int32_t type_index, int32_t allow_ancestor, MLCAny *ret);
MLC_API int32_t MLCVTableSetFunc(MLCVTableHandle vtable, int32_t type_index, MLCFunc *func, int32_t override_mode);
MLC_API int32_t MLCErrorCreate(const char *kind, int64_t num_bytes, const char *bytes, MLCAny *ret);
MLC_API int32_t MLCErrorGetInfo(MLCAny error, int32_t *num_strs, const char ***strs);
MLC_API int32_t MLCExtObjCreate(int32_t num_bytes, int32_t type_index, MLCAny *ret);
MLC_API void MLCExtObjDelete(void *objptr);
#ifdef __cplusplus
} // MLC_EXTERN_C
#endif

#endif // MLC_C_FFI_ABI_H_
