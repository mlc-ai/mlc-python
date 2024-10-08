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
#include <functional>
#include <memory>
#include <sstream>
#include <type_traits>
#include <vector>
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
template <typename DerivedType, typename SelfType = Object> bool IsInstanceOf(const MLCAny *self);

MLC_INLINE MLCTypeInfo *TypeIndex2TypeInfo(int32_t type_index) {
  MLCTypeInfo *type_info;
  MLCTypeIndex2Info(nullptr, type_index, &type_info);
  return type_info;
}

MLC_INLINE const char *TypeIndex2TypeKey(int32_t type_index) {
  if (MLCTypeInfo *type_info = TypeIndex2TypeInfo(type_index)) {
    return type_info->type_key;
  }
  return "(undefined)";
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

MLC_INLINE MLCTypeInfo *TypeRegister(int32_t parent_type_index, int32_t type_index, const char *type_key,
                                     MLCAttrGetterSetter getter, MLCAttrGetterSetter setter) {
  MLCTypeInfo *info = nullptr;
  MLCTypeRegister(nullptr, parent_type_index, type_key, type_index, &info);
  info->setter = setter;
  info->getter = getter;
  return info;
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
    } else if constexpr (IsRawObjPtr<T>) {
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

  static void GetTypeAnnotation(std::vector<MLCTypeInfo *> *info) {
    if constexpr (std::is_same_v<T, Any> || std::is_same_v<T, AnyView>) {
      info->push_back(TypeIndex2TypeInfo(static_cast<int32_t>(MLCTypeIndex::kMLCNone)));
    } else if constexpr (std::is_same_v<T, void>) {
      MLC_THROW(TypeError) << "`void` is not allowed in type annotation";
    } else if constexpr (std::is_same_v<T, char *>) {
      info->push_back(TypeIndex2TypeInfo(TypeTraits<_T>::type_index));
    } else if constexpr (IsPOD<T>) {
      info->push_back(TypeIndex2TypeInfo(TypeTraits<T>::type_index));
    } else if constexpr (IsRawObjPtr<T>) {
      using U = std::remove_pointer_t<T>;
      Type2Str<Ref<U>>::GetTypeAnnotation(info);
    } else if constexpr (std::is_base_of_v<UList, T>) {
      info->push_back(TypeIndex2TypeInfo(static_cast<int32_t>(MLCTypeIndex::kMLCList)));
      Type2Str<typename T::TElem>::GetTypeAnnotation(info);
    } else if constexpr (std::is_base_of_v<UDict, T>) {
      info->push_back(TypeIndex2TypeInfo(static_cast<int32_t>(MLCTypeIndex::kMLCDict)));
      Type2Str<typename T::TKey>::GetTypeAnnotation(info);
      Type2Str<typename T::TValue>::GetTypeAnnotation(info);
    } else if constexpr (IsRef<T> || IsObjRef<T>) {
      using U = typename T::TObj;
      info->push_back(TypeIndex2TypeInfo(U::_type_index));
    } else {
      static_assert(std::is_void_v<T>, "Unsupported type");
    }
  }
};

inline std::string TypeAnnotation2Str(MLCTypeInfo **ann) {
  std::function<std::string(int *)> f;
  f = [ann, &f](int *i) -> std::string {
    MLCTypeInfo *info = ann[*i];
    ++(*i);
    if (info->type_index == static_cast<int32_t>(MLCTypeIndex::kMLCNone)) {
      return "Any";
    } else if (info->type_index == static_cast<int32_t>(MLCTypeIndex::kMLCList)) {
      std::string elem = f(i);
      return "list[" + elem + "]";
    } else if (info->type_index == static_cast<int32_t>(MLCTypeIndex::kMLCDict)) {
      std::string key = f(i);
      std::string value = f(i);
      return "dict[" + key + ", " + value + "]";
    } else {
      return info->type_key;
    }
  };
  int i = 0;
  return f(&i);
}

MLC_INLINE void IncRef(MLCObject *obj) {
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
      if (type_index < 0 || type_index >= 1000) {
        std::cout << "Something is seriously wrong here!!!!!!!!" << std::endl;
        std::abort();
      }
    }
#endif
  }
}

MLC_INLINE void DecRef(MLCObject *obj) {
  if (obj != nullptr) {
#if MLC_DEBUG_MODE == 1
    {
      int32_t type_index = obj->type_index;
      int32_t ref_cnt = obj->ref_cnt;
      void *addr = obj;
      ref_cnt -= 1;
      std::cout << "DecRef @ " << addr << ": type_index = " << type_index << ", ref_cnt = " << ref_cnt << std::endl;
      if (type_index < 0 || type_index >= 1000 || ref_cnt < 0) {
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
    if (ref_cnt == 1 && obj->deleter) {
      obj->deleter(obj);
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
} // namespace base
} // namespace mlc

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // MLC_BASE_UTILS_H_
