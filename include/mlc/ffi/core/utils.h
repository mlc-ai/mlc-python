#ifndef MLC_UTILS_H_
#define MLC_UTILS_H_
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
#include <cstdlib>
#include <memory>
#include <mlc/ffi/c_api.h>
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

namespace mlc {
namespace ffi {

/********** Section 1. Macros *********/

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
#define MLC_THROW(ErrorKind) ::mlc::ffi::details::ErrorBuilder(#ErrorKind, MLC_TRACEBACK_HERE()).Get()
#define MLC_SAFE_CALL_BEGIN()                                                                                          \
  try {                                                                                                                \
  (void)0
#define MLC_SAFE_CALL_END(err_ret)                                                                                     \
  return 0;                                                                                                            \
  }                                                                                                                    \
  catch (::mlc::ffi::Exception & err) {                                                                                \
    err.MoveToAny(err_ret);                                                                                            \
    return -2;                                                                                                         \
  }                                                                                                                    \
  catch (std::exception & err) {                                                                                       \
    *(err_ret) = Ref<::mlc::ffi::StrObj>::New(err.what());                                                             \
    return -1;                                                                                                         \
  }                                                                                                                    \
  MLC_UNREACHABLE()

#define MLC_DEF_ASSIGN(SelfType, SourceType)                                                                           \
  MLC_INLINE SelfType &operator=(SourceType other) {                                                                   \
    if constexpr (std::is_rvalue_reference_v<SourceType>) {                                                            \
      SelfType(std::move(other)).Swap(*this);                                                                          \
    } else {                                                                                                           \
      SelfType(other).Swap(*this);                                                                                     \
    }                                                                                                                  \
    return *this;                                                                                                      \
  }

#define MLC_TRY_CONVERT(Expr, TypeIndex, TypeStr)                                                                      \
  try {                                                                                                                \
    return Expr;                                                                                                       \
  } catch (const TemporaryTypeError &) {                                                                               \
    MLC_THROW(TypeError) << "Cannot convert from type `" << details::TypeIndex2TypeKey(TypeIndex) << "` to `"          \
                         << (TypeStr) << "`";                                                                          \
  }                                                                                                                    \
  MLC_UNREACHABLE()

#define MLC_DEF_REFLECTION(ObjType)                                                                                    \
  static inline const int32_t _type_reflect =                                                                          \
      ::mlc::ffi::details::ReflectionHelper(static_cast<int32_t>(ObjType::_type_index))

/********** Section 2. Forward declaration of any/ref/objects *********/

struct AnyView;
struct Any;
struct Object;
struct ErrorObj;
struct Error;
struct StrObj;
struct Str;
struct UListObj;
struct UList;
struct UDictObj;
struct UDict;
struct FuncObj;
struct Func;
struct ObjectRef;
template <typename> struct ListObj;
template <typename> struct List;
template <typename, typename> struct DictObj;
template <typename, typename> struct Dict;
template <typename> struct Ref;
template <std::size_t N> struct AnyViewArray;
template <size_t N> using CharArray = const char[N];

namespace details {
template <typename T> using RemoveCR = std::remove_const_t<std::remove_reference_t<T>>;

// clang-format off
// IsPOD<>
template <typename T, typename = void> struct IsPODImpl : std::false_type {};
template <> struct IsPODImpl<DLDevice> : std::true_type {};
template <> struct IsPODImpl<DLDataType> : std::true_type {};
template <typename Int> struct IsPODImpl<Int, std::enable_if_t<std::is_integral_v<Int>>> : std::true_type {};
template <typename Float> struct IsPODImpl<Float, std::enable_if_t<std::is_floating_point_v<Float>>> : std::true_type {};
template <> struct IsPODImpl<void *> : std::true_type {};
template <> struct IsPODImpl<std::nullptr_t> : std::true_type {};
template <> struct IsPODImpl<const char *> : std::true_type {};
template <> struct IsPODImpl<std::string> : std::true_type {};
template <size_t N> struct IsPODImpl<char[N]> : std::true_type {};
template <typename T> constexpr bool IsPOD = IsPODImpl<T>::value;
// clang-format on

// clang-format off
// Object and Object::Allocator
template <typename, typename = void> struct IsObjImpl : std::false_type {};
template <typename T> struct IsObjImpl<T, std::void_t<decltype(T::_type_info)>> : std::true_type {};
template <typename T> constexpr static bool IsObj = IsObjImpl<T>::value;
template <typename ObjectType, typename = std::enable_if_t<details::IsObj<ObjectType>>> struct DefaultObjectAllocator;
template <typename T, typename = void> struct AllocatorOfImplImpl { using Type = DefaultObjectAllocator<T>; };
template <typename T> struct AllocatorOfImplImpl<T, std::void_t<typename T::Allocator>> { using Type = typename T::Allocator; };
template <typename T, typename = std::enable_if_t<IsObj<T>>> struct AllocatorOfImpl { using Type = typename AllocatorOfImplImpl<T>::Type; };
template <typename T> using AllocatorOf = typename AllocatorOfImpl<T>::Type;
// clang-format on

// clang-format off
// Ref<T> and TObjectRef
template <typename> struct IsRefImpl;
template <typename T> constexpr bool IsRef = IsRefImpl<T>::value;
template <typename T> struct IsObjRefImpl { static constexpr bool value = std::is_base_of_v<ObjectRef, T>; };
template <typename T> constexpr bool IsObjRef = IsObjRefImpl<T>::value;
// clang-format on

// clang-format off
template <typename Derived, typename Base> struct IsDerivedFromImpl;
template <typename Derived, typename Base> constexpr bool IsDerivedFrom = std::conjunction_v<IsObjImpl<Derived>, IsObjImpl<Base>, IsDerivedFromImpl<Derived, Base>>;

template <typename T, typename = void> struct HasFuncTraitsImpl : std::false_type {};
template <typename T> constexpr bool HasFuncTraits = HasFuncTraitsImpl<T>::value;

struct NewableImpl {
  template <typename, typename, typename...> static std::false_type test(...);
  template <typename Allocator, typename Ret, typename... Args>
  static auto test(int) -> decltype(std::is_same<decltype(Allocator::New(std::declval<Args>()...)), Ret>{});
};
template <typename T, typename... Args>
constexpr bool Newable = decltype(NewableImpl::test<details::AllocatorOf<T>, T*, Args...>(0))::value;
} // namespace details
// clang-format on

namespace tag {
// Tags for static dispatching
struct POD {};
struct ObjPtr {};
struct RawObjPtr {};
template <typename T, typename = void> struct TagImpl {
  using type = void;
};
template <typename T> struct TagImpl<T, std::enable_if_t<details::IsPOD<T>>> {
  using type = tag::POD;
};
template <typename T> struct TagImpl<T, std::enable_if_t<std::is_base_of_v<MLCObjPtr, T>>> {
  using type = tag::ObjPtr;
};
template <typename T> struct TagImpl<T *, std::enable_if_t<details::IsObj<T>>> {
  using type = tag::RawObjPtr;
};
template <typename T> using Tag = typename TagImpl<details::RemoveCR<T>>::type;
template <typename T> constexpr bool Exists = std::is_same_v<T, details::RemoveCR<T>> && !std::is_same_v<Tag<T>, void>;
} // namespace tag

template <typename T>
constexpr bool IsContainerElement = std::is_same_v<T, details::RemoveCR<T>> //
                                    && (std::is_same_v<T, Any> ||           //
                                        std::is_integral_v<T> ||            //
                                        std::is_floating_point_v<T> ||      //
                                        std::is_same_v<T, DLDevice> ||      //
                                        std::is_same_v<T, DLDataType> ||    //
                                        std::is_same_v<tag::Tag<T>, tag::ObjPtr>);

/********** Section 3. Traits *********/
/*!
 * \brief PODTraits<T> is a template class that provides the following set of
 * static methods that are associated with a specific POD type `T`.
 *
 * 0) Type2Str<T>: () -> std::string
 * 1) AnyCopyToType<T>: (const MLCAny *) -> T
 * 2) TypeCopyToAny<T>: (T, MLCAny *) -> void
 */
template <typename, typename = void> struct PODTraits {};

/*!
 * \brief ObjPtrTraits<T> is a template class that provides the following set of
 * static methods that are associated with a specific object type `T`.
 *
 * 1) PtrToAnyView<T>: (const T *, MLCAny *) -> void
 * 2) AnyToUnownedPtr<T>: (const MLCAny *) -> T *
 * 3) AnyToOwnedPtr<T>: (const MLCAny *) -> T *
 * 4) AnyToOwnedPtrWithStorage<T>: (const MLCAny *, Any *) -> T * (optional)
 */
template <typename T, typename = void> struct ObjPtrTraits;
template <typename T> struct ObjPtrTraits<ListObj<T>, void>;

template <typename _T> struct Type2Str {
  static std::string Run() {
    using T = details::RemoveCR<_T>;
    if constexpr (std::is_same_v<T, Any>) {
      return "Any";
    } else if constexpr (std::is_same_v<T, AnyView>) {
      return "AnyView";
    } else if constexpr (std::is_same_v<T, void>) {
      return "void";
    } else if constexpr (details::IsPOD<T>) {
      return PODTraits<T>::Type2Str();
    } else if constexpr (std::is_pointer_v<T> && details::IsObj<std::remove_pointer_t<T>>) {
      return std::string(std::remove_pointer_t<T>::_type_key) + " *";
    } else if constexpr (std::is_same_v<T, UList>) {
      return "list[Any]";
    } else if constexpr (std::is_same_v<T, UDict>) {
      return "dict[Any, Any]";
    } else if constexpr (std::is_base_of_v<UList, T>) {
      return "list[" + Type2Str<typename T::TElem>::Run() + "]";
    } else if constexpr (std::is_base_of_v<UDict, T>) {
      return "dict[" + Type2Str<typename T::TKey>::Run() + ", " + Type2Str<typename T::TValue>::Run() + "]";
    } else if constexpr (details::IsRef<T>) {
      return "Ref<" + std::string(T::TObj::_type_key) + ">";
    } else if constexpr (details::IsObjRef<T>) {
      return std::string(T::TObj::_type_key);
    } else if constexpr (details::IsObj<T>) {
      return std::string(T::_type_key);
    } else {
      static_assert(std::is_void_v<T>, "Unsupported type");
    }
  }
};

/********** Section 4. Errors *********/

struct TemporaryTypeError : public std::exception {};

namespace details {

[[noreturn]] void MLCErrorFromBuilder(const char *kind, MLCByteArray message, MLCByteArray traceback) noexcept(false);

struct ErrorBuilder {
  const char *kind;
  MLCByteArray traceback;
  std::ostringstream oss;

  explicit ErrorBuilder(const char *kind, MLCByteArray traceback) : kind(kind), traceback(traceback), oss() {}

  [[noreturn]] ~ErrorBuilder() noexcept(false) {
    const std::string &message_str = oss.str();
    MLCByteArray message{static_cast<int64_t>(message_str.size()), message_str.data()};
    MLCErrorFromBuilder(this->kind, message, this->traceback);
    MLC_UNREACHABLE();
  }

  std::ostringstream &Get() { return this->oss; }
};

} // namespace details

/********** Section 5. Reflection *********/

namespace details {

struct ReflectionHelper {
  explicit ReflectionHelper(int32_t type_index);
  template <typename Super, typename FieldType>
  ReflectionHelper &FieldReadOnly(const char *name, FieldType Super::*field);
  template <typename Super, typename FieldType> ReflectionHelper &Field(const char *name, FieldType Super::*field);
  template <typename Callable> ReflectionHelper &Method(const char *name, Callable &&method);
  operator int32_t();
  static std::string DefaultStrMethod(AnyView any);

private:
  template <typename Cls, typename FieldType> constexpr std::ptrdiff_t ReflectOffset(FieldType Cls::*member) {
    return reinterpret_cast<std::ptrdiff_t>(&((Cls *)(nullptr)->*member));
  }
  int32_t type_index;
  std::vector<MLCTypeField> fields;
  std::vector<MLCTypeMethod> methods;
  std::vector<Any> method_pool;
};
} // namespace details

/********** Section 6. Util methods *********/

namespace details {
void FuncCall(const void *func, int32_t num_args, const MLCAny *args, MLCAny *ret);

template <typename DerivedType, typename SelfType = Object> bool IsInstanceOf(const MLCAny *self);

MLC_INLINE const char *TypeIndex2TypeKey(int32_t type_index) {
  MLCTypeInfo *type_info;
  MLCTypeIndex2Info(nullptr, type_index, &type_info);
  return type_info ? type_info->type_key : "(undefined)";
}

MLC_INLINE const char *TypeIndex2TypeKey(const MLCAny *self) {
  return self == nullptr ? "None" : TypeIndex2TypeKey(self->type_index);
}

MLC_INLINE MLCTypeInfo *TypeRegister(int32_t parent_type_index, int32_t type_index, const char *type_key) {
  MLCTypeInfo *info = nullptr;
  MLCTypeRegister(nullptr, parent_type_index, type_key, type_index, &info);
  return info;
}

MLC_INLINE bool IsTypeIndexNone(int32_t type_index) {
  return type_index == static_cast<int32_t>(MLCTypeIndex::kMLCNone);
}

MLC_INLINE bool IsTypeIndexPOD(int32_t type_index) {
  return type_index < static_cast<int32_t>(MLCTypeIndex::kMLCStaticObjectBegin);
}

void AnyView2Str(std::ostream &os, const MLCAny *v);
int32_t StrCompare(const MLCStr *a, const MLCStr *b);
uint64_t StrHash(const MLCStr *str);
MLCObject *StrMoveFromStdString(std::string &&source);
MLCObject *StrCopyFromCharArray(const char *source, size_t length);

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
      std::cout << "IncRef: type_index = " << type_index << ", ref_cnt = " << ref_cnt << ", addr = " << addr
                << std::endl;
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
      std::cout << "DecRef: type_index = " << type_index << ", ref_cnt = " << ref_cnt << ", addr = " << addr
                << std::endl;
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
} // namespace details
} // namespace ffi
} // namespace mlc

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // MLC_UTILS_H_
