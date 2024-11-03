#ifndef MLC_CORE_FUNC_TRAITS_H_
#define MLC_CORE_FUNC_TRAITS_H_
#include <functional>
#include <mlc/base/all.h>
#include <type_traits>

namespace mlc {

template <typename, typename = void> struct FuncTraits;
template <typename T, typename = void> struct HasFuncTraitsImpl : std::false_type {};
template <typename T> struct HasFuncTraitsImpl<T, std::void_t<decltype(FuncTraits<T>::packed)>> : std::true_type {};
template <typename T> constexpr bool HasFuncTraits = HasFuncTraitsImpl<T>::value;

template <typename R, typename... Args> struct FuncTraitsImpl {
  using FType = R(Args...);
  using ArgType = std::tuple<Args...>;
  using RetType = R;
  static constexpr bool packed = std::is_convertible_v<FType, std::function<void(int32_t, const AnyView *, Any *)>>;
  static constexpr bool unpacked = (::mlc::base::Anyable<::mlc::base::RemoveCR<Args>> && ...) &&
                                   (std::is_void_v<R> || ::mlc::base::Anyable<::mlc::base::RemoveCR<R>>);
  static MLC_INLINE std::string Sig();
  static MLC_INLINE void CheckIsUnpacked() {
    CheckIsUnpackedRun(std::index_sequence_for<Args...>{});
    static_assert(std::is_void_v<R> || ::mlc::base::Anyable<R>, "Invalid return type");
  }

protected:
  template <size_t i> static MLC_INLINE void CheckIsUnpackedApply() {
    using Arg = std::tuple_element_t<i, ArgType>;
    static_assert(::mlc::base::Anyable<::mlc::base::RemoveCR<Arg>>, "Invalid argument type");
  }
  template <size_t... I> static MLC_INLINE void CheckIsUnpackedRun(std::index_sequence<I...>) {
    static_assert((std::is_void_v<RetType> || std::is_convertible_v<RetType, Any>), "Invalid return type");
    using TExpander = int[];
    (void)TExpander{0, (CheckIsUnpackedApply<I>(), 0)...};
  }
};

template <typename Functor>
struct FuncTraits<Functor, std::void_t<decltype(&Functor::operator())>>
    : public FuncTraits<decltype(&Functor::operator())> {};
template <typename R, typename... Args> struct FuncTraits<R(Args...), void> : public FuncTraitsImpl<R, Args...> {};
template <typename R, typename... Args> struct FuncTraits<R (*)(Args...), void> : public FuncTraitsImpl<R, Args...> {};
template <typename Cls, typename R, typename... Args>
struct FuncTraits<R (Cls::*)(Args...) const> : public FuncTraitsImpl<R, Args...> {};
template <typename Cls, typename R, typename... Args>
struct FuncTraits<R (Cls::*)(Args...)> : public FuncTraitsImpl<R, Args...> {};
} // namespace mlc
#endif // MLC_CORE_FUNC_TRAITS_H_
