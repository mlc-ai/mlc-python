#ifndef MLC_BASE_TRAITS_H_
#define MLC_BASE_TRAITS_H_
#include <mlc/c_api.h>
#include <string>
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
} // namespace mlc

namespace mlc {
namespace base {
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

struct ObjectDummyRoot {
  static constexpr int32_t _type_depth = -1;
  static constexpr int32_t _type_index = -1;
};

using VoidPtr = void *;
template <typename T> using RemoveCR = std::remove_const_t<std::remove_reference_t<T>>;
template <typename T> constexpr bool AlwaysFalse = false;

// CharArray<N> and IsCharArray<T>
template <size_t N> using CharArray = char[N];
template <typename T, typename = void> struct IsCharArray : std::false_type {};
template <size_t N> struct IsCharArray<CharArray<N>> : std::true_type {};
template <size_t N> struct IsCharArray<const CharArray<N>> : std::true_type {};

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

// IsObj<T>, IsRawObjPtr<T>, AllocatorOf<T>
template <typename, typename = void> struct IsObjImpl : std::false_type {};
template <> struct IsObjImpl<::mlc::Object, void> : std::true_type {};
template <typename T> struct IsObjImpl<T, std::void_t<decltype(T::_type_info)>> : std::true_type {};
template <typename T> constexpr static bool IsObj = IsObjImpl<T>::value;
template <typename T> constexpr static bool IsRawObjPtr = std::is_pointer_v<T> && IsObj<std::remove_pointer_t<T>>;
template <typename T, typename = void> struct AllocatorOfImpl {
  using Type = DefaultObjectAllocator<T>;
};
template <typename T> struct AllocatorOfImpl<T, std::void_t<typename T::Allocator>> {
  using Type = typename T::Allocator;
};
template <typename T> using AllocatorOf = typename AllocatorOfImpl<T>::Type;

// IsRef<T>, IsObjRef<T>
template <typename> struct IsRefImpl;
template <typename T> constexpr bool IsRef = IsRefImpl<T>::value;
template <typename> struct IsRefImpl {
  static constexpr bool value = false;
};
template <typename T> struct IsRefImpl<Ref<T>> {
  static constexpr bool value = true;
};
template <typename T> struct IsObjRefImpl {
  static constexpr bool value = std::is_base_of_v<ObjectRef, T>;
};
template <typename T> constexpr bool IsObjRef = IsObjRefImpl<T>::value;

// IsOptional<T>
template <typename T> struct IsOptionalImpl : std::false_type {};
template <typename T> struct IsOptionalImpl<Optional<T>> : std::true_type {};
template <typename T> constexpr bool IsOptional = IsOptionalImpl<T>::value;

// Newable<T, Args...>
struct NewableImpl {
  template <typename, typename, typename...> static std::false_type test(...);
  template <typename Allocator, typename Ret, typename... Args>
  static auto test(int) -> decltype(std::is_same<decltype(Allocator::New(std::declval<Args>()...)), Ret>{});
};
template <typename T, typename... Args>
constexpr bool Newable = decltype(NewableImpl::test<AllocatorOf<T>, T *, Args...>(0))::value;

// Anyable<T>
template <typename T>
constexpr bool Anyable = IsPOD<T> || IsRawObjPtr<T> || IsRef<T> || IsOptional<T> || IsObjRef<T> ||
                         std::is_same_v<T, Any> || std::is_same_v<T, AnyView>;

// IsContainerElement<T, Args...>
template <typename T>
constexpr bool IsContainerElement =
    std::is_same_v<T, RemoveCR<T>> && (std::is_same_v<T, Any> || IsPOD<T> || IsObjRef<T>);

// IsTemplate<T>
template <typename T, typename = void> struct IsTemplateImpl : std::false_type {};
template <template <typename...> class C, typename... Args> struct IsTemplateImpl<C<Args...>> : std::true_type {};
template <typename T> inline constexpr bool IsTemplate = IsTemplateImpl<T>::value;

// ObjFromRef<T>
template <typename, typename = void> struct ObjFromRefImpl;
template <typename T> struct ObjFromRefImpl<T, std::enable_if_t<IsObj<T>>> {
  using type = T;
};
template <typename T> struct ObjFromRefImpl<T, Ref<T>> {
  using type = T;
};
template <typename T> struct ObjFromRefImpl<T, Optional<T>> {
  using type = typename Optional<T>::TObj;
};
template <typename T> struct ObjFromRefImpl<T, std::enable_if_t<IsObjRef<T>>> {
  using type = typename T::TObj;
};
template <typename T> struct ObjFromRefImpl<T, std::enable_if_t<IsPOD<T>>> {
  using type = T;
};
template <typename T> using ObjFromRef = typename ObjFromRefImpl<T>::type;

// IsDerivedFrom<Derived, Base>
template <typename Derived, typename Base> struct IsDerivedFromImpl;
template <typename T> struct IsDerivedFromImpl<T, T> {
  static constexpr bool value = true;
};
template <typename Base> struct IsDerivedFromImpl<ObjectDummyRoot, Base> {
  static constexpr bool value = false;
};
template <typename Derived, typename Base> struct IsDerivedFromImpl {
  static constexpr bool value = IsDerivedFromImpl<typename Derived::_type_parent, Base>::value;
};
template <typename Derived, typename Base>
constexpr bool IsDerivedFrom = std::conjunction_v<IsObjImpl<Derived>, //
                                                  IsObjImpl<Base>,    //
                                                  IsDerivedFromImpl<Derived, Base>>;

// IsObjRefDerivedFrom<TObjRef, TObj>
template <typename TObjRef, typename TObj, typename = void> //
struct IsObjRefDerivedFromImpl {
  static constexpr bool value = false;
};
template <typename TObjRef, typename TObj>
struct IsObjRefDerivedFromImpl<TObjRef, TObj, std::enable_if_t<IsObjRef<TObjRef> && IsObj<TObj>>> {
  static constexpr bool value = IsDerivedFrom</*derived=*/ObjFromRef<TObjRef>, /*base=*/TObj>;
};
template <typename TObjRef, typename TObj> //
constexpr bool IsObjRefDerivedFrom = IsObjRefDerivedFromImpl<TObjRef, TObj>::value;

} // namespace base
} // namespace mlc
#endif // MLC_BASE_TRAITS_H_
