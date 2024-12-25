#ifndef MLC_BASE_TRAITS_H_
#define MLC_BASE_TRAITS_H_
#include <mlc/c_api.h>
#include <ostream>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>

namespace mlc {
struct Exception;
struct NullType {};
constexpr NullType Null{};

struct AnyView;
struct Any;
template <std::size_t N> struct AnyViewArray;
template <typename> struct Ref;
template <typename> struct Optional;
struct Object;
struct ObjectRef;
struct ErrorObj;
struct Error;
struct StrObj;
struct Str;
struct FuncObj;
struct Func;
struct UListObj;
struct UList;
struct UDictObj;
struct UDict;
template <typename> struct ListObj;
template <typename> struct List;
template <typename, typename> struct DictObj;
template <typename, typename> struct Dict;

template <typename T, typename... Args> Ref<Object> InitOf(Args &&...args);
template <typename T> struct DefaultObjectAllocator;
template <typename T> struct PODAllocator;

enum class StructureKind : int32_t {
  kNone = 0,
  kNoBind = 1,
  kBind = 2,
  kVar = 3,
};
enum class StructureFieldKind : int32_t {
  kNoBind = 0,
  kBind = 1,
};

} // namespace mlc

namespace mlc {
namespace base {

// Basic types, tuples and other utils
using VoidPtr = void *;
template <size_t N> using CharArray = char[N];
template <typename T> using RemoveCR = std::remove_cv_t<std::remove_reference_t<T>>;
// clang-format off
template <typename Tuple, typename Append> struct _TupleAppend;
template <typename Append, typename... Args> struct _TupleAppend<std::tuple<Args...>, Append> { using type = std::tuple<Args..., Append>; };
template <typename Tuple, int32_t tuple_len, int32_t i, typename expected> struct _TupleElementEq {   static constexpr bool Run() { if constexpr (i < tuple_len) return std::is_same_v<typename std::tuple_element_t<i, Tuple>, expected>; else return false; } };
// clang-format on
template <typename Tuple, typename Append> using TupleAppend = typename _TupleAppend<Tuple, Append>::type;
template <typename Tuple, int32_t tuple_len, int32_t i, typename expected>
constexpr bool TupleElementEq = _TupleElementEq<Tuple, tuple_len, i, expected>::Run();

// IsTemplate<T>
template <typename T, typename = void> struct IsTemplateImpl : std::false_type {};
template <template <typename...> class C, typename... Args> struct IsTemplateImpl<C<Args...>> : std::true_type {};
template <typename T> inline constexpr bool IsTemplate = IsTemplateImpl<T>::value;

// IsPOD<T>
template <typename T, typename = void> struct IsPODImpl : std::false_type {};
template <typename Int> struct IsPODImpl<Int, std::enable_if_t<std::is_integral_v<Int>>> : std::true_type {};
template <typename Float>
struct IsPODImpl<Float, std::enable_if_t<std::is_floating_point_v<Float>>> : std::true_type {};
template <> struct IsPODImpl<DLDevice> : std::true_type {};
template <> struct IsPODImpl<DLDataType> : std::true_type {};
template <> struct IsPODImpl<VoidPtr> : std::true_type {};
template <> struct IsPODImpl<std::nullptr_t> : std::true_type {};
template <size_t N> struct IsPODImpl<CharArray<N>> : std::true_type {};
template <size_t N> struct IsPODImpl<const CharArray<N>> : std::true_type {};
template <> struct IsPODImpl<const char *> : std::true_type {};
template <> struct IsPODImpl<char *> : std::true_type {};
template <> struct IsPODImpl<std::string> : std::true_type {};
template <typename T> constexpr bool IsPOD = IsPODImpl<T>::value;

// TypeKind-related tags
enum class TypeKind : int32_t {
  kInvalid = 0,
  kPOD = 1,
  kAny = 2,
  kObj = 3,
  kRef = 4,
  kObjPtr = 5,
  kObjRef = 6,
  kOptional = 7,
};

template <typename T, typename = void> struct _TypeKindExtractor {
  static constexpr TypeKind value = TypeKind::kInvalid;
};
template <typename T> struct _TypeKindExtractor<T, std::void_t<decltype(T::_type_kind)>> {
  static constexpr TypeKind value = T::_type_kind;
};

template <typename T> struct _TypeKindOf {
  static constexpr TypeKind Value() {
    if constexpr (IsPOD<T>) {
      return TypeKind::kPOD;
    } else if constexpr (std::is_pointer_v<T>) {
      constexpr TypeKind kind = _TypeKindExtractor<std::remove_const_t<std::remove_pointer_t<T>>>::value;
      return kind == TypeKind::kObj ? TypeKind::kObjPtr : TypeKind::kInvalid;
    } else {
      return _TypeKindExtractor<T>::value;
    }
  }
};

template <typename T> constexpr TypeKind TypeKindOf = _TypeKindOf<RemoveCR<T>>::Value();
template <typename T> constexpr bool IsAny = TypeKindOf<T> == TypeKind::kAny;
template <typename T> constexpr bool IsObj = TypeKindOf<T> == TypeKind::kObj;
template <typename T> constexpr bool IsRef = TypeKindOf<T> == TypeKind::kRef;
template <typename T> constexpr bool IsObjPtr = TypeKindOf<T> == TypeKind::kObjPtr;
template <typename T> constexpr bool IsObjRef = TypeKindOf<T> == TypeKind::kObjRef;
template <typename T> constexpr bool IsOptional = TypeKindOf<T> == TypeKind::kOptional;
template <typename T>
constexpr bool IsPODOrObjPtrOrObjRef =
    TypeKindOf<T> == TypeKind::kPOD || TypeKindOf<T> == TypeKind::kObjPtr || TypeKindOf<T> == TypeKind::kObjRef;
template <typename T> constexpr bool Anyable = TypeKindOf<T> != TypeKind::kInvalid;
template <typename T>
constexpr bool IsContainerElement =
    std::is_same_v<T, RemoveCR<T>> && (std::is_same_v<T, Any> || IsPOD<T> || IsObjRef<T>);

// IsDerivedFrom      <DerivedObj   , BaseObj>
// IsDerivedFromOpt   <DerivedOpt   , BaseObj>
// IsDerivedFromObjRef<DerivedObjRef, BaseObj>
template <typename DerivedObj, typename BaseObj> struct _IsDerivedFrom {
  static constexpr bool Value() {
    if constexpr (std::is_same_v<DerivedObj, BaseObj>) {
      return true;
    } else if constexpr (IsObj<DerivedObj> && IsObj<BaseObj>) {
      return TupleElementEq<typename DerivedObj::_type_ancestor_types, DerivedObj::_type_depth + 2,
                            BaseObj::_type_depth + 1, BaseObj>;
    } else {
      return false;
    }
  }
};
template <typename DerivedObj, typename BaseObj>
constexpr bool IsDerivedFrom = _IsDerivedFrom<DerivedObj, BaseObj>::Value();

template <typename TDerivedObjRef, typename TBaseObj> struct _IsDerivedFromOpt {
  static constexpr bool Value() {
    if constexpr (IsOptional<TDerivedObjRef> && IsObj<TBaseObj>) {
      return IsDerivedFrom<typename TDerivedObjRef::TObj, TBaseObj>;
    } else {
      return false;
    }
  }
};
template <typename TDerivedOpt, typename TBaseObj>
constexpr bool IsDerivedFromOpt = _IsDerivedFromOpt<TDerivedOpt, TBaseObj>::Value();

template <typename TDerivedObjRef, typename TBaseObj> struct _IsDerivedFromObjRef {
  static constexpr bool Value() {
    constexpr TypeKind kind = _TypeKindExtractor<TDerivedObjRef>::value;
    if constexpr (kind == TypeKind::kObjRef) {
      return IsDerivedFrom<typename TDerivedObjRef::TObj, TBaseObj>;
    } else {
      return false;
    }
  }
};
template <typename TDerivedObjRef, typename TBaseObj>
constexpr bool IsDerivedFromObjRef = _IsDerivedFromObjRef<TDerivedObjRef, TBaseObj>::Value();

// AllocatorOf<T>, Allocatable<T, Args...>
template <typename T, typename = void> struct AllocatorOfImpl {
  using Type = DefaultObjectAllocator<T>;
};
template <typename T> struct AllocatorOfImpl<T, std::void_t<typename T::Allocator>> {
  using Type = typename T::Allocator;
};
template <typename T> using AllocatorOf = typename AllocatorOfImpl<T>::Type;
struct _Allocatable {
  template <typename, typename, typename...> static std::false_type test(...);
  template <typename Allocator, typename Ret, typename... Args>
  static auto test(int) -> decltype(std::is_same<decltype(Allocator::New(std::declval<Args>()...)), Ret>{});
};
template <typename T, typename... Args>
constexpr bool Allocatable = decltype(_Allocatable::test<AllocatorOf<T>, T *, Args...>(0))::value;

// Func traits
enum class FuncKind : int32_t {
  kInvalid = 0,
  kPacked = 1,
  kUnpacked = 2,
};

template <typename T> struct Type2Str;

template <size_t i, typename... _Args> struct _Args2Str;
template <size_t i> struct _Args2Str<i> {
  static void Run(std::ostream &) {}
};
template <size_t i, typename Arg, typename... _Args> struct _Args2Str<i, Arg, _Args...> {
  static void Run(std::ostream &os) {
    if constexpr (i > 0) {
      os << ", ";
    }
    os << i << ": " << Type2Str<Arg>::Run();
    _Args2Str<i + 1, _Args...>::Run(os);
  }
};

template <typename _R, typename... _Args> struct _FuncKind {
  static constexpr FuncKind Value() {
    if constexpr ((Anyable<_Args> && ...)) {
      if constexpr (std::is_void_v<_R>) {
        return FuncKind::kUnpacked;
      } else if constexpr (Anyable<_R>) {
        return FuncKind::kUnpacked;
      }
    }
    return FuncKind::kInvalid;
  }
  static std::string Sig() {
    std::ostringstream os;
    os << "(";
    _Args2Str<0, _Args...>::Run(os);
    os << ") -> " << Type2Str<_R>::Run();
    return os.str();
  }
};

template <> struct _FuncKind<void, int32_t, const AnyView *, Any *> {
  static constexpr FuncKind Value() { return FuncKind::kPacked; }
  static std::string Sig() { return "(...AnyView) -> Any"; }
};
template <> struct _FuncKind<void, int32_t, const MLCAny *, MLCAny *> {
  static constexpr FuncKind Value() { return FuncKind::kPacked; }
  static std::string Sig() { return "(...AnyView) -> Any"; }
};

template <typename R, typename... Args> struct _FuncCanonicalize {
  using FType = R(Args...);
  using ArgType = std::tuple<Args...>;
  using RetType = R;
  using Impl = _FuncKind<R, Args...>;
  static constexpr int32_t N = sizeof...(Args);
  static constexpr FuncKind kind = Impl::Value();
  static constexpr bool packed = kind == FuncKind::kPacked;
  static constexpr bool unpacked = kind == FuncKind::kUnpacked;
  static std::string Sig() { return Impl::Sig(); }
};

template <typename, typename = void> struct FuncCanonicalize {
  static constexpr FuncKind kind = FuncKind::kInvalid;
};
template <typename Functor>
struct FuncCanonicalize<Functor, std::void_t<decltype(&Functor::operator())>>
    : public FuncCanonicalize<decltype(&Functor::operator())> {};
template <typename R, typename... Args>
struct FuncCanonicalize<R(Args...), void> : public _FuncCanonicalize<R, Args...> {};
template <typename R, typename... Args>
struct FuncCanonicalize<R (*)(Args...), void> : public _FuncCanonicalize<R, Args...> {};
template <typename Cls, typename R, typename... Args>
struct FuncCanonicalize<R (Cls::*)(Args...) const> : public _FuncCanonicalize<R, Args...> {};
template <typename Cls, typename R, typename... Args>
struct FuncCanonicalize<R (Cls::*)(Args...)> : public _FuncCanonicalize<R, Args...> {};

template <typename Functor> constexpr bool HasFuncTraits = FuncCanonicalize<Functor>::kind != FuncKind::kInvalid;

template <typename Functor> struct UnpackedFuncDiagnostics {
  // TODO: this diagnosis is not usually triggered
  using Func = FuncCanonicalize<Functor>;
  template <size_t i, typename... Args> struct _RunArgs;
  template <size_t i> struct _RunArgs<i> {
    static constexpr bool Run() { return true; }
  };
  template <size_t i, typename Arg, typename... Args> struct _RunArgs<i, Arg, Args...> {
    static constexpr bool Run() {
      if constexpr (!Anyable<Arg>) {
        static_assert(Anyable<Arg>, "Invalid argument type at index `i`");
      } else {
        return _RunArgs<i + 1, Args...>::Run();
      }
    }
  };
  template <typename> struct RunArgs;
  template <typename... Args> struct RunArgs<std::tuple<Args...>> {
    static constexpr bool Run() { return _RunArgs<0, Args...>::Run(); }
  };
  static constexpr FuncKind Run() {
    using R = typename Func::RetType;
    using Args = typename Func::ArgType;
    if constexpr (!(std::is_void_v<R> || Anyable<R>)) {
      static_assert(std::is_void_v<R> || Anyable<R>, "Invalid return type");
    } else {
      constexpr bool valid_args = RunArgs<Args>::Run();
      static_assert(valid_args, "InternalError: Can't diagnose function");
    }
    return Func::kind;
  }
};

/*!
 * \brief TypeTraits<T> is a template class that provides the following set of
 * static methods that are associated with a specific type `T`:
 *
 * (T1) TypeToAny<T>: (T, MLCAny *) -> void
 * (T2) AnyToTypeUnowned<T>: (const MLCAny *) -> T
 * (T3) AnyToTypeOwned<T>: (const MLCAny *) -> T
 * (T4) AnyToTypeWithStorage<T>: (const MLCAny *, Any *) -> T
 * - This method is optional if `T` doesn't support such conversion
 *
 * The trait is enabled if and only if
 * - `T` is a POD type
 * - `T` is `TObject *` where `TObject` is an object type
 */
template <typename T, typename = void> struct TypeTraits;

} // namespace base
} // namespace mlc
#endif // MLC_BASE_TRAITS_H_
