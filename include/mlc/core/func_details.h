#ifndef MLC_CORE_FUNC_DETAILS_H_
#define MLC_CORE_FUNC_DETAILS_H_

#include "./func.h"
#include "./typing.h"
#include <cstring>
#include <memory>

namespace mlc {
namespace core {

/********** Section 1. Function signature and FuncTraits *********/

template <typename R, typename... Args> struct Func2Str {
  template <size_t i> static void Apply(std::ostream &os) {
    using Arg = std::tuple_element_t<i, std::tuple<Args...>>;
    if constexpr (i != 0) {
      os << ", ";
    }
    os << i << ": " << ::mlc::base::Type2Str<Arg>::Run();
  }
  template <size_t... I> static std::string Run(std::index_sequence<I...>) {
    std::ostringstream ss;
    ss << "(";
    {
      using TExpander = int[];
      (void)TExpander{0, (Apply<I>(ss), 0)...};
    }
    ss << ") -> " << ::mlc::base::Type2Str<R>::Run();
    return ss.str();
  }
};

/********** Section 2. UnpackCall and index sequence-related *********/

template <int32_t i, typename> struct PrependIntegerSeq;
template <int32_t i, int32_t... Is> struct PrependIntegerSeq<i, std::integer_sequence<int32_t, Is...>> {
  using type = std::integer_sequence<int32_t, i, Is...>;
};
template <int32_t I, typename> struct IndexIntSeq;
template <int32_t I, int32_t Head, int32_t... Tail>
struct IndexIntSeq<I, std::integer_sequence<int32_t, Head, Tail...>>
    : IndexIntSeq<I - 1, std::integer_sequence<int32_t, Tail...>> {};
template <int32_t Head, int32_t... Tail> struct IndexIntSeq<0, std::integer_sequence<int32_t, Head, Tail...>> {
  static constexpr int32_t value = Head;
};

template <typename T, typename = void> struct MayUseStorage {
  static constexpr int32_t value = 0;
};
template <typename T>
struct MayUseStorage<T *, std::void_t<decltype(::mlc::base::TypeTraits<T *>::AnyToTypeWithStorage)>> {
  static constexpr int32_t value = 1;
};

template <typename... Args> struct FFIStorageInfo;
template <> struct FFIStorageInfo<> {
  constexpr static int32_t total = 0;
  using indices = std::integer_sequence<int32_t>;
};
template <typename I, typename... Is> struct FFIStorageInfo<I, Is...> {
private:
  using Prev = FFIStorageInfo<Is...>;
  constexpr static int32_t may_use_storage = MayUseStorage<I>::value;

public:
  constexpr static int32_t total = Prev::total + may_use_storage;
  using indices = typename PrependIntegerSeq<(may_use_storage ? total : 0) - 1, typename Prev::indices>::type;
};

template <typename Function, typename StorageInfo> struct UnpackCallArgConverter {
  template <typename _Type, size_t i> struct AsType {
    MLC_INLINE_NO_MSVC static auto Run(const AnyView &v, Any *storage) {
      using Type = std::decay_t<_Type>;
      try {
        if constexpr (MayUseStorage<Type>::value == 0) {
          return v.template Cast<Type>();
        } else {
          constexpr int32_t storage_index = IndexIntSeq<i, typename StorageInfo::indices>::value;
          static_assert(storage_index >= 0, "Invalid storage index");
          return v.CastWithStorage<Type>(storage + storage_index);
        }
      } catch (const Exception &e) {
        if (strcmp(e.Obj()->kind(), "TypeError") == 0) {
          MLC_THROW(TypeError) << "Mismatched type on argument #" << i << " when calling: `"
                               << FuncTraits<Function>::Sig() << "`. Expected `" << ::mlc::base::Type2Str<Type>::Run()
                               << "` but got `" << ::mlc::base::TypeIndex2TypeKey(v.type_index) << "`";
        } else if (strcmp(e.Obj()->kind(), "NestedTypeError") == 0) {
          MLC_THROW(TypeError) << "Mismatched type on argument #" << i << " when calling: `"
                               << FuncTraits<Function>::Sig() << "`. " << e.what();
        } else {
          // This is used in the case where the error message is customized to something else.
          // For example, when parsing a string to DLDevice, it could be ValueError when the string is invalid
          throw;
        }
      }
      MLC_UNREACHABLE();
    }
  };

  template <size_t i> struct AsType<AnyView, i> {
    MLC_INLINE static AnyView Run(const AnyView &v, Any *) { return v; }
  };

  template <size_t i> struct AsType<Any, i> {
    MLC_INLINE static Any Run(const AnyView &v, Any *) { return v; }
  };
};

template <typename, typename> struct UnpackCall;

template <typename RetType, typename... Args> struct UnpackCall<RetType, std::tuple<Args...>> {
  template <typename Function, typename FuncType, size_t... I>
  MLC_INLINE static void Run(FuncType *func, const AnyView *args, Any *ret, std::index_sequence<I...>) {
    using Storage = FFIStorageInfo<Args...>;
    using CVT = UnpackCallArgConverter<Function, Storage>;
    if constexpr (Storage::total > 0 && std::is_void_v<RetType>) {
      Any storage[Storage::total];
      ret->Reset();
      (*func)(CVT::template AsType<Args, I>::Run(args[I], storage)...);
    } else if constexpr (Storage::total > 0 && !std::is_void_v<RetType>) {
      Any storage[Storage::total];
      *ret = (*func)(CVT::template AsType<Args, I>::Run(args[I], storage)...);
    } else if constexpr (Storage::total == 0 && std::is_void_v<RetType>) {
      ret->Reset();
      (*func)(CVT::template AsType<Args, I>::Run(args[I], nullptr)...);
    } else if constexpr (Storage::total == 0 && !std::is_void_v<RetType>) {
      *ret = (*func)(CVT::template AsType<Args, I>::Run(args[I], nullptr)...);
    }
  }
};

/********** Section 3. Func::Allocator *********/

template <typename FuncType>
inline void FuncCallPacked(const FuncObj *obj, int32_t num_args, const AnyView *args, Any *ret) {
  static_cast<const FuncImpl<FuncType> *>(obj)->func_(num_args, args, ret);
}

template <typename FuncType>
inline void FuncCallUnpacked(const FuncObj *obj, int32_t num_args, const AnyView *args, Any *ret) {
  using ArgType = typename FuncTraits<FuncType>::ArgType;
  constexpr int32_t N = std::tuple_size_v<ArgType>;
  if (num_args != N) {
    MLC_THROW(TypeError) << "Mismatched number of arguments when calling: `" << FuncTraits<FuncType>::Sig()
                         << "`. Expected " << N << " but got " << num_args << " arguments";
  }
  using IdxSeq = std::make_index_sequence<N>;
  using RetType = typename FuncTraits<FuncType>::RetType;
  UnpackCall<RetType, ArgType>::template Run<FuncType>(&static_cast<const FuncImpl<FuncType> *>(obj)->func_, args, ret,
                                                       IdxSeq{});
}

template <typename FuncType, typename = void> struct FuncAllocatorImpl {
  MLC_INLINE static FuncObj *Run(FuncType func) {
    FuncTraits<FuncType>::CheckIsUnpacked();
    using Allocator = typename FuncImpl<FuncType>::Allocator;
    return Allocator::New(std::forward<FuncType>(func), FuncCallUnpacked<FuncType>);
  }
};

template <typename FuncType> struct FuncAllocatorImpl<FuncType, std::enable_if_t<FuncTraits<FuncType>::packed>> {
  MLC_INLINE static FuncObj *Run(FuncType func) {
    using Allocator = typename FuncImpl<FuncType>::Allocator;
    return Allocator::New(std::forward<FuncType>(func), FuncCallPacked<FuncType>);
  }
};

template <typename Obj, typename R, typename... Args>
struct FuncAllocatorImpl<R (Obj::*)(Args...) const, std::enable_if_t<FuncTraits<R(const Obj *, Args...)>::unpacked>> {
  using FuncType = R (Obj::*)(Args...) const;
  MLC_INLINE static FuncObj *Run(FuncType func) {
    FuncTraits<R(const Obj *, Args...)>::CheckIsUnpacked();
    auto wrapped = [func](const Obj *self, Args... args) { return ((*self).*func)(std::forward<Args>(args)...); };
    return FuncAllocatorImpl<decltype(wrapped)>::Run(wrapped);
  }
};

template <typename Obj, typename R, typename... Args>
struct FuncAllocatorImpl<R (Obj::*)(Args...), std::enable_if_t<FuncTraits<R(Obj *, Args...)>::unpacked>> {
  using FuncType = R (Obj::*)(Args...);
  MLC_INLINE static FuncObj *Run(FuncType func) {
    FuncTraits<R(Obj *, Args...)>::CheckIsUnpacked();
    auto wrapped = [func](Obj *self, Args... args) { return ((*self).*func)(std::forward<Args>(args)...); };
    return FuncAllocatorImpl<decltype(wrapped)>::Run(wrapped);
  }
};

} // namespace core
} // namespace mlc

namespace mlc {
template <typename R, typename... Args> MLC_INLINE std::string FuncTraitsImpl<R, Args...>::Sig() {
  return ::mlc::core::Func2Str<R, Args...>::Run(std::index_sequence_for<Args...>{});
}
template <typename FuncType, typename> MLC_INLINE FuncObj *FuncObj::Allocator::New(FuncType func) {
  return ::mlc::core::FuncAllocatorImpl<::mlc::base::RemoveCR<FuncType>>::Run(std::forward<FuncType>(func));
}
inline Ref<FuncObj> FuncObj::FromForeign(void *self, MLCDeleterType deleter, MLCFuncSafeCallType safe_call) {
  if (deleter == nullptr) {
    return Ref<FuncObj>::New([self, safe_call](int32_t num_args, const MLCAny *args, MLCAny *ret) {
      if (int32_t err_code = safe_call(self, num_args, args, ret); err_code != 0) {
        ::mlc::core::HandleSafeCallError(err_code, ret);
      }
    });
  } else {
    return Ref<FuncObj>::New(
        [self = std::shared_ptr<void>(self, deleter), safe_call](int32_t num_args, const MLCAny *args, MLCAny *ret) {
          if (int32_t err_code = safe_call(self.get(), num_args, args, ret); err_code != 0) {
            ::mlc::core::HandleSafeCallError(err_code, ret);
          }
        });
  }
}
} // namespace mlc

#endif // MLC_CORE_FUNC_DETAILS_H_
