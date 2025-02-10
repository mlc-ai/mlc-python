#ifndef MLC_CORE_FUNC_DETAILS_H_
#define MLC_CORE_FUNC_DETAILS_H_

#include "./func.h"
#include "./typing.h"
#include <cstring>
#include <memory>

namespace mlc {
namespace core {

using mlc::base::FuncCanonicalize;

/********** Section 1. UnpackCall and index sequence-related *********/

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
          return v.operator Type();
        } else {
          constexpr int32_t storage_index = IndexIntSeq<i, typename StorageInfo::indices>::value;
          static_assert(storage_index >= 0, "Invalid storage index");
          return v._CastWithStorage<Type>(storage + storage_index);
        }
      } catch (const Exception &e) {
        if (strcmp(e.Obj()->kind(), "TypeError") == 0) {
          MLC_THROW(TypeError) << "Mismatched type on argument #" << i << " when calling: `"
                               << FuncCanonicalize<Function>::Sig() << "`. Expected `"
                               << ::mlc::base::Type2Str<Type>::Run() << "` but got `"
                               << ::mlc::Lib::GetTypeKey(v.type_index) << "`";
        } else if (strcmp(e.Obj()->kind(), "NestedTypeError") == 0) {
          MLC_THROW(TypeError) << "Mismatched type on argument #" << i << " when calling: `"
                               << FuncCanonicalize<Function>::Sig() << "`. " << e.what();
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

  template <size_t i> struct AsType<const AnyView &, i> {
    MLC_INLINE static AnyView Run(const AnyView &v, Any *) { return v; }
  };

  template <size_t i> struct AsType<AnyView &&, i> {
    MLC_INLINE static AnyView Run(const AnyView &v, Any *) { return v; }
  };

  template <size_t i> struct AsType<Any, i> {
    MLC_INLINE static Any Run(const AnyView &v, Any *) { return v; }
  };
  template <size_t i> struct AsType<const Any &, i> {
    MLC_INLINE static Any Run(const AnyView &v, Any *) { return v; }
  };
  template <size_t i> struct AsType<Any &&, i> {
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

/********** Section 2. Func::Allocator *********/

template <typename FuncType>
inline void FuncCallPacked(const FuncObj *obj, int32_t num_args, const AnyView *args, Any *ret) {
  static_cast<const FuncImpl<FuncType> *>(obj)->func_(num_args, args, ret);
}

template <typename FuncType>
inline void FuncCallUnpacked(const FuncObj *obj, int32_t num_args, const AnyView *args, Any *ret) {
  using Traits = FuncCanonicalize<FuncType>;
  constexpr int32_t N = Traits::N;
  if (num_args != N) {
    MLC_THROW(TypeError) << "Mismatched number of arguments when calling: `" << FuncCanonicalize<FuncType>::Sig()
                         << "`. Expected " << N << " but got " << num_args << " arguments";
  }
  using IdxSeq = std::make_index_sequence<N>;
  using RetType = typename FuncCanonicalize<FuncType>::RetType;
  UnpackCall<RetType, typename Traits::ArgType>::template Run<FuncType>(
      &static_cast<const FuncImpl<FuncType> *>(obj)->func_, args, ret, IdxSeq{});
}

template <typename FuncType, typename = void> struct FuncAllocatorImpl {
  MLC_INLINE static FuncObj *Run(FuncType func) {
    using Allocator = typename FuncImpl<FuncType>::Allocator;
    return Allocator::New(std::forward<FuncType>(func), FuncCallUnpacked<FuncType>);
  }
};

template <typename FuncType> struct FuncAllocatorImpl<FuncType, std::enable_if_t<FuncCanonicalize<FuncType>::packed>> {
  MLC_INLINE static FuncObj *Run(FuncType func) {
    using Allocator = typename FuncImpl<FuncType>::Allocator;
    return Allocator::New(std::forward<FuncType>(func), FuncCallPacked<FuncType>);
  }
};

template <typename Obj, typename R, typename... Args>
struct FuncAllocatorImpl<R (Obj::*)(Args...) const,
                         std::enable_if_t<FuncCanonicalize<R(const Obj *, Args...)>::unpacked>> {
  using FuncType = R (Obj::*)(Args...) const;
  MLC_INLINE static FuncObj *Run(FuncType func) {
    auto wrapped = [func](const Obj *self, Args... args) { return ((*self).*func)(std::forward<Args>(args)...); };
    return FuncAllocatorImpl<decltype(wrapped)>::Run(wrapped);
  }
};

template <typename Obj, typename R, typename... Args>
struct FuncAllocatorImpl<R (Obj::*)(Args...), //
                         std::enable_if_t<FuncCanonicalize<R(Obj *, Args...)>::unpacked>> {
  using FuncType = R (Obj::*)(Args...);
  MLC_INLINE static FuncObj *Run(FuncType func) {
    auto wrapped = [func](Obj *self, Args... args) { return ((*self).*func)(std::forward<Args>(args)...); };
    return FuncAllocatorImpl<decltype(wrapped)>::Run(wrapped);
  }
};

} // namespace core
} // namespace mlc

namespace mlc {
template <typename FuncType, typename> MLC_INLINE FuncObj *FuncObj::Allocator::New(FuncType func) {
  return ::mlc::core::FuncAllocatorImpl<::mlc::base::RemoveCR<FuncType>>::Run(std::forward<FuncType>(func));
}
inline Ref<FuncObj> FuncObj::FromForeign(void *self, MLCDeleterType deleter, MLCFuncSafeCallType safe_call) {
  if (deleter == nullptr) {
    return Ref<FuncObj>::New([self, safe_call](int32_t num_args, const MLCAny *args, MLCAny *ret) {
      if (int32_t err_code = safe_call(self, num_args, args, ret)) {
        ::mlc::base::FuncCallCheckError(err_code, ret);
      }
    });
  } else {
    return Ref<FuncObj>::New(
        [self = std::shared_ptr<void>(self, deleter), safe_call](int32_t num_args, const MLCAny *args, MLCAny *ret) {
          if (int32_t err_code = safe_call(self.get(), num_args, args, ret)) {
            ::mlc::base::FuncCallCheckError(err_code, ret);
          }
        });
  }
}
} // namespace mlc

#endif // MLC_CORE_FUNC_DETAILS_H_
