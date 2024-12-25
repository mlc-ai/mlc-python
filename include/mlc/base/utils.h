#ifndef MLC_BASE_UTILS_H_
#define MLC_BASE_UTILS_H_
#define MLC_DEBUG_MODE 0
#ifndef MLC_DEBUG_MODE
#define MLC_DEBUG_MODE 0
#endif

#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_BitScanReverse64)
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#endif
#if __cplusplus >= 202002L
#include <bit>
#endif
#include "./base_traits.h"
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sstream>
#include <type_traits>
#if MLC_DEBUG_MODE == 1
#include <iostream>
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4722) // throw in the destructor, which is expected behavior
#pragma warning(disable : 4702) // unreachable code
#endif

#if defined(_MSC_VER)
#define MLC_INLINE __forceinline
#define MLC_INLINE_NO_MSVC __inline
#else
#define MLC_INLINE inline __attribute__((always_inline))
#define MLC_INLINE_NO_MSVC MLC_INLINE
#endif

#if defined(_MSC_VER)
#define MLC_UNREACHABLE() __assume(false)
#else
#define MLC_UNREACHABLE() __builtin_unreachable()
#endif

#if defined(__GNUC__) || defined(__clang__)
#define MLC_SYMBOL_HIDE __attribute__((visibility("hidden")))
#else
#define MLC_SYMBOL_HIDE
#endif

#if defined(__GNUC__) || defined(__clang__)
#define MLC_FUNC_SIG __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define MLC_FUNC_SIG __FUNCSIG__
#else
#define MLC_FUNC_SIG __func__
#endif

#define MLC_STR(__x) #__x
#define MLC_STR_CONCAT_(__x, __y) __x##__y
#define MLC_STR_CONCAT(__x, __y) MLC_STR_CONCAT_(__x, __y)
#define MLC_UNIQUE_ID() MLC_STR_CONCAT(__mlc_unique_id_, __COUNTER__)
#define MLC_TRACEBACK_HERE() MLCTraceback(__FILE__, MLC_STR(__LINE__), MLC_FUNC_SIG)
#define MLC_THROW(ErrorKind) ::mlc::base::ErrorBuilder(#ErrorKind, MLC_TRACEBACK_HERE()).Get()
#define MLC_MAKE_ERROR_HERE(ErrorKind, Msg) ::mlc::base::MLCCreateError(#ErrorKind, Msg, MLC_TRACEBACK_HERE())

#define MLC_TRY_CONVERT(Expr, TypeIndex, TypeStr)                                                                      \
  try {                                                                                                                \
    return Expr;                                                                                                       \
  } catch (const ::mlc::base::TemporaryTypeError &) {                                                                  \
    MLC_THROW(TypeError) << "Cannot convert from type `" << ::mlc::base::TypeIndex2TypeKey(TypeIndex) << "` to `"      \
                         << (TypeStr) << "`";                                                                          \
  }                                                                                                                    \
  MLC_UNREACHABLE()

namespace mlc {
namespace base {

/********** Section 1. Errors *********/

struct TemporaryTypeError : public std::exception {};

[[noreturn]] void MLCThrowError(const char *kind, MLCByteArray message, MLCByteArray traceback) noexcept(false);
Any MLCCreateError(const char *kind, const std::string &message, MLCByteArray traceback);

struct ErrorBuilder {
  const char *kind;
  MLCByteArray traceback;
  std::ostringstream oss;

  explicit ErrorBuilder(const char *kind, MLCByteArray traceback) : kind(kind), traceback(traceback), oss() {}

  [[noreturn]] ~ErrorBuilder() noexcept(false) {
    const std::string &message_str = oss.str();
    MLCByteArray message{static_cast<int64_t>(message_str.size()), message_str.data()};
    MLCThrowError(this->kind, message, this->traceback);
    MLC_UNREACHABLE();
  }

  std::ostringstream &Get() { return this->oss; }
};

/********** Section 2. Util methods *********/

StrObj *StrCopyFromCharArray(const char *source, size_t length);
void FuncCall(const void *func, int32_t num_args, const MLCAny *args, MLCAny *ret);
template <typename Callable> Any CallableToAny(Callable &&callable);
template <typename DerivedType, typename SelfType = Object> bool IsInstanceOf(const MLCAny *self);

MLC_INLINE MLCTypeInfo *TypeIndex2TypeInfo(int32_t type_index) {
  MLCTypeInfo *type_info;
  MLCTypeIndex2Info(nullptr, type_index, &type_info);
  return type_info;
}

MLC_INLINE MLCTypeInfo *TypeKey2TypeInfo(const char *type_key) {
  MLCTypeInfo *type_info;
  MLCTypeKey2Info(nullptr, type_key, &type_info);
  return type_info;
}

MLC_INLINE const char *TypeIndex2TypeKey(int32_t type_index) {
  if (MLCTypeInfo *type_info = TypeIndex2TypeInfo(type_index)) {
    return type_info->type_key;
  }
  return "(undefined)";
}

MLC_INLINE int32_t TypeIndexOf(const MLCAny *self) { return self ? self->type_index : static_cast<int32_t>(kMLCNone); }

MLC_INLINE int32_t TypeKey2TypeIndex(const char *type_key) {
  if (MLCTypeInfo *type_info = TypeKey2TypeInfo(type_key)) {
    return type_info->type_index;
  }
  MLC_THROW(TypeError) << "Cannot find type with key: " << type_key;
  MLC_UNREACHABLE();
}

MLC_INLINE const char *TypeIndex2TypeKey(const MLCAny *self) {
  if (self == nullptr) {
    return "None";
  }
  if (MLCTypeInfo *type_info = TypeIndex2TypeInfo(self->type_index)) {
    return type_info->type_key;
  }
  return "(undefined)";
}

MLC_INLINE bool IsTypeIndexNone(int32_t type_index) {
  return type_index == static_cast<int32_t>(MLCTypeIndex::kMLCNone);
}

MLC_INLINE bool IsTypeIndexPOD(int32_t type_index) {
  return type_index < static_cast<int32_t>(MLCTypeIndex::kMLCStaticObjectBegin);
}

template <typename _T> struct Type2Str {
  using T = RemoveCR<_T>;
  static std::string Run() {
    if constexpr (std::is_same_v<T, Any>) {
      return "Any";
    } else if constexpr (std::is_same_v<T, AnyView>) {
      return "AnyView";
    } else if constexpr (std::is_same_v<T, void>) {
      return "void";
    } else if constexpr (std::is_same_v<T, Object>) {
      return "object.Object";
    } else if constexpr (std::is_same_v<T, ObjectRef>) {
      return "object.ObjectRef";
    } else if constexpr (std::is_same_v<T, Str>) {
      return "str";
    } else if constexpr (IsPOD<T>) {
      return TypeTraits<T>::type_str;
    } else if constexpr (IsObjPtr<T>) {
      using U = std::remove_pointer_t<T>;
      return Type2Str<U>::Run() + " *";
    } else if constexpr (std::is_base_of_v<UListObj, T>) {
      using E = typename T::TElem;
      return "object.ListObj[" + Type2Str<E>::Run() + "]";
    } else if constexpr (std::is_base_of_v<UDictObj, T>) {
      using K = typename T::TKey;
      using V = typename T::TValue;
      return "object.DictObj[" + Type2Str<K>::Run() + ", " + Type2Str<V>::Run() + "]";
    } else if constexpr (std::is_base_of_v<UList, T>) {
      using E = typename T::TElem;
      return "list[" + Type2Str<E>::Run() + "]";
    } else if constexpr (std::is_base_of_v<UDict, T>) {
      using K = typename T::TKey;
      using V = typename T::TValue;
      return "dict[" + Type2Str<K>::Run() + ", " + Type2Str<V>::Run() + "]";
    } else if constexpr (IsRef<T>) {
      return "Ref<" + Type2Str<typename T::TObj>::Run() + ">";
    } else if constexpr (IsOptional<T>) {
      return "Optional<" + Type2Str<typename T::TObj>::Run() + ">";
    } else if constexpr (IsObjRef<T>) {
      return std::string(T::TObj::_type_key);
    } else if constexpr (IsObj<T>) {
      return std::string(T::_type_key) + "Obj";
    } else {
      static_assert(std::is_void_v<T>, "Unsupported type");
    }
  }
};

MLC_INLINE void IncRef(MLCAny *obj) {
  if (obj != nullptr) {
#ifdef _MSC_VER
    _InterlockedIncrement(reinterpret_cast<volatile long *>(&obj->ref_cnt));
#else
    __atomic_fetch_add(&obj->ref_cnt, 1, __ATOMIC_RELAXED);
#endif
#if MLC_DEBUG_MODE == 1
    {
      int32_t type_index = obj->type_index;
      int32_t ref_cnt = obj->ref_cnt;
      void *addr = obj;
      std::cout << "IncRef @ " << addr << ": type_index = " << type_index << ", ref_cnt = " << ref_cnt << std::endl;
      if (type_index < 0 || type_index >= 200000) {
        std::cout << "Something is seriously wrong here!!!!!!!!" << std::endl;
        std::abort();
      }
    }
#endif
  }
}

MLC_INLINE void DecRef(MLCAny *obj) {
  if (obj != nullptr) {
#if MLC_DEBUG_MODE == 1
    {
      int32_t type_index = obj->type_index;
      int32_t ref_cnt = obj->ref_cnt;
      void *addr = obj;
      ref_cnt -= 1;
      std::cout << "DecRef @ " << addr << ": type_index = " << type_index << ", ref_cnt = " << ref_cnt << std::endl;
      if (type_index < 0 || type_index >= 200000 || ref_cnt < 0) {
        std::cout << "Something is seriously wrong here!!!!!!!!" << std::endl;
        std::abort();
      }
    }
#endif
#ifdef _MSC_VER
    int32_t ref_cnt = _InterlockedDecrement(reinterpret_cast<volatile long *>(&obj->ref_cnt)) + 1;
#else
    int32_t ref_cnt = __atomic_fetch_sub(&obj->ref_cnt, 1, __ATOMIC_ACQ_REL);
#endif
    if (ref_cnt == 1 && obj->v.deleter) {
      obj->v.deleter(obj);
    }
  }
}

MLC_INLINE int32_t CountLeadingZeros(uint64_t x) {
#if __cplusplus >= 202002L
  return std::countl_zero(x);
#elif defined(_MSC_VER)
  unsigned long leading_zero = 0;
  if (_BitScanReverse64(&leading_zero, x)) {
    return static_cast<int32_t>(63 - leading_zero);
  } else {
    return 64;
  }
#else
  return x == 0 ? 64 : __builtin_clzll(x);
#endif
}

MLC_INLINE uint64_t BitCeil(uint64_t x) {
#if __cplusplus >= 202002L
  return std::bit_ceil(x);
#else
  return x <= 1 ? 1 : static_cast<uint64_t>(1) << (64 - CountLeadingZeros(x - 1));
#endif
}

using PODArray = std::unique_ptr<void, decltype(&std::free)>;
template <typename PODType> inline PODArray PODArrayCreate(int64_t size) {
  return PODArray(static_cast<void *>(std::malloc(size * sizeof(PODType))), std::free);
}
inline void PODArraySwapOut(PODArray *self, void **data) {
  void *out = self->release();
  self->reset(*data);
  *data = out;
}
struct PODArrayFinally {
  void *data;
  ~PODArrayFinally() { std::free(data); }
};

inline int64_t StrToInt(const std::string &str, size_t start_pos = 0) {
  if (start_pos >= str.size()) {
    throw std::runtime_error("Invalid integer string");
  }
  const char *c_str = str.c_str() + start_pos;
  char *endptr = nullptr;
  int64_t result = std::strtoll(c_str, &endptr, 10);
  if (*endptr != '\0') {
    throw std::runtime_error("Invalid integer string");
  }
  return result;
}

MLC_INLINE uint64_t HashCombine(uint64_t seed, uint64_t value) {
  return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

MLC_INLINE int32_t StrCompare(const char *a, const char *b, int64_t a_len, int64_t b_len) {
  if (a_len != b_len) {
    return static_cast<int32_t>(a_len - b_len);
  }
  return std::strncmp(a, b, a_len);
}

inline uint64_t StrHash(const char *str, int64_t length) {
  const char *it = str;
  const char *end = str + length;
  uint64_t result = 0;
  for (; it + 8 <= end; it += 8) {
    uint64_t b = (static_cast<uint64_t>(it[0]) << 56) | (static_cast<uint64_t>(it[1]) << 48) |
                 (static_cast<uint64_t>(it[2]) << 40) | (static_cast<uint64_t>(it[3]) << 32) |
                 (static_cast<uint64_t>(it[4]) << 24) | (static_cast<uint64_t>(it[5]) << 16) |
                 (static_cast<uint64_t>(it[6]) << 8) | static_cast<uint64_t>(it[7]);
    result = HashCombine(result, b);
  }
  if (it < end) {
    uint64_t b = 0;
    if (it + 4 <= end) {
      b = (static_cast<uint64_t>(it[0]) << 24) | (static_cast<uint64_t>(it[1]) << 16) |
          (static_cast<uint64_t>(it[2]) << 8) | static_cast<uint64_t>(it[3]);
      it += 4;
    }
    if (it + 2 <= end) {
      b = (b << 16) | (static_cast<uint64_t>(it[0]) << 8) | static_cast<uint64_t>(it[1]);
      it += 2;
    }
    if (it + 1 <= end) {
      b = (b << 8) | static_cast<uint64_t>(it[0]);
      it += 1;
    }
    result = HashCombine(result, b);
  }
  return result;
}

inline uint64_t StrHash(const char *str) {
  int64_t length = static_cast<int64_t>(std::strlen(str));
  return StrHash(str, length);
}

inline uint64_t AnyHash(const MLCAny &a) {
  if (a.type_index == static_cast<int32_t>(MLCTypeIndex::kMLCStr)) {
    const MLCStr *str = reinterpret_cast<MLCStr *>(a.v.v_obj);
    return ::mlc::base::StrHash(str->data, str->length);
  }
  union {
    int64_t i64;
    uint64_t u64;
  } cvt;
  cvt.i64 = a.v.v_int64;
  return cvt.u64;
}

inline bool AnyEqual(const MLCAny &a, const MLCAny &b) {
  if (a.type_index != b.type_index) {
    return false;
  }
  if (a.type_index == static_cast<int32_t>(MLCTypeIndex::kMLCStr)) {
    const MLCStr *str_a = reinterpret_cast<MLCStr *>(a.v.v_obj);
    const MLCStr *str_b = reinterpret_cast<MLCStr *>(b.v.v_obj);
    return ::mlc::base::StrCompare(str_a->data, str_b->data, str_a->length, str_b->length) == 0;
  }
  return a.v.v_int64 == b.v.v_int64;
}

struct LibState {
  static inline MLCVTableHandle VTableGetGlobal(const char *name) {
    MLCVTableHandle ret;
    MLCVTableGetGlobal(nullptr, name, &ret);
    return ret;
  }
  static inline FuncObj *VTableGetFunc(MLCVTableHandle vtable, int32_t type_index, const char *vtable_name) {
    MLCAny func{};
    MLCVTableGetFunc(vtable, type_index, true, &func);
    if (!IsTypeIndexPOD(func.type_index)) {
      DecRef(func.v.v_obj);
    }
    FuncObj *ret = reinterpret_cast<FuncObj *>(func.v.v_obj);
    if (func.type_index != kMLCFunc) {
      MLC_THROW(TypeError) << "Function `" << vtable_name
                           << "` for type: " << ::mlc::base::TypeIndex2TypeKey(type_index)
                           << " is not callable. Its type is " << ::mlc::base::TypeIndex2TypeKey(func.type_index);
    }
    return ret;
  }
  static inline ::mlc::Str CxxStr(AnyView obj);
  static inline ::mlc::Str Str(AnyView obj);
  static inline Any IRPrint(AnyView obj, AnyView printer, AnyView path);

  static MLC_SYMBOL_HIDE inline MLCVTableHandle cxx_str = VTableGetGlobal("__cxx_str__");
  static MLC_SYMBOL_HIDE inline MLCVTableHandle str = VTableGetGlobal("__str__");
  static MLC_SYMBOL_HIDE inline MLCVTableHandle ir_print = VTableGetGlobal("__ir_print__");
};

} // namespace base
} // namespace mlc

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // MLC_BASE_UTILS_H_
