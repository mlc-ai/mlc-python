#ifndef MLC_FUNC_DETAILS_H_
#define MLC_FUNC_DETAILS_H_

#include "./func.h"
#include <cstring>
#include <functional>
#include <memory>
#include <mlc/ffi/core/core.h>

namespace mlc {
namespace ffi {
namespace details {

/********** Section 1. Function signature stringify *********/

template <typename R, typename... Args> struct Func2Str {
  template <size_t i> static void Apply(std::ostream &os) {
    using Arg = std::tuple_element_t<i, std::tuple<Args...>>;
    if constexpr (i != 0) {
      os << ", ";
    }
    os << i << ": " << Type2Str<Arg>::Run();
  }
  template <size_t... I> static std::string Run(std::index_sequence<I...>) {
    std::ostringstream ss;
    ss << "(";
    {
      using TExpander = int[];
      (void)TExpander{0, (Apply<I>(ss), 0)...};
    }
    ss << ") -> " << Type2Str<R>::Run();
    return ss.str();
  }
};

/********** Section 2. FuncTraits *********/

template <typename R, typename... Args> struct FuncTraitsImpl {
  using FType = R(Args...);
  using ArgType = std::tuple<Args...>;
  using RetType = R;
  static constexpr bool packed = std::is_convertible_v<FType, std::function<void(int32_t, const AnyView *, Any *)>>;
  static constexpr bool unpacked =
      (std::is_convertible_v<AnyView, Args> && ...) && (std::is_void_v<R> || std::is_convertible_v<R, Any>);
  static MLC_INLINE std::string Sig() { return Func2Str<R, Args...>::Run(std::index_sequence_for<Args...>{}); }
  static MLC_INLINE void CheckIsUnpacked() { CheckIsUnpackedRun(std::index_sequence_for<Args...>{}); }

protected:
  template <size_t i> static MLC_INLINE void CheckIsUnpackedApply() {
    using Arg = std::tuple_element_t<i, ArgType>;
    static_assert(std::is_convertible_v<AnyView, Arg>, "Invalid argument type");
  }
  template <size_t... I> static MLC_INLINE void CheckIsUnpackedRun(std::index_sequence<I...>) {
    static_assert((std::is_void_v<RetType> || std::is_convertible_v<RetType, Any>), "Invalid return type");
    using TExpander = int[];
    (void)TExpander{0, (CheckIsUnpackedApply<I>(), 0)...};
  }
};

template <typename, typename = void> struct FuncTraits;
template <typename Functor>
struct FuncTraits<Functor, std::void_t<decltype(&Functor::operator())>>
    : public FuncTraits<decltype(&Functor::operator())> {};
template <typename R, typename... Args> struct FuncTraits<R(Args...), void> : public FuncTraitsImpl<R, Args...> {};
template <typename R, typename... Args> struct FuncTraits<R (*)(Args...), void> : public FuncTraitsImpl<R, Args...> {};
template <typename Cls, typename R, typename... Args>
struct FuncTraits<R (Cls::*)(Args...) const> : public FuncTraitsImpl<R, Args...> {};
template <typename Cls, typename R, typename... Args>
struct FuncTraits<R (Cls::*)(Args...)> : public FuncTraitsImpl<R, Args...> {};
template <typename T> struct HasFuncTraitsImpl<T, std::void_t<decltype(FuncTraits<T>::packed)>> : std::true_type {};

/********** Section 3. Index sequence-related stuff *********/

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

/********** Section 4. UnpackCall: unpack an AnyView array *********/

template <typename T, typename = void> struct MayUseStorage {
  static constexpr int32_t value = 0;
};
template <typename T> struct MayUseStorage<T *, std::void_t<decltype(ObjPtrTraits<T>::AnyToOwnedPtrWithStorage)>> {
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
          using ObjType = std::remove_pointer_t<Type>;
          static_assert(storage_index >= 0, "Invalid storage index");
          return v.CastWithStorage<ObjType>(storage + storage_index);
        }
      } catch (const Exception &e) {
        if (strcmp(e.data_->kind(), "TypeError") == 0) {
          MLC_THROW(TypeError) << "Mismatched type on argument #" << i << " when calling: `"
                               << FuncTraits<Function>::Sig() << "`. Expected `" << Type2Str<Type>::Run()
                               << "` but got `" << details::TypeIndex2TypeKey(v.type_index) << "`";
        } else if (strcmp(e.data_->kind(), "NestedTypeError") == 0) {
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

/********** Section 5. Func::Allocator *********/

template <typename FuncType> void FuncCallPacked(const FuncObj *obj, int32_t num_args, const AnyView *args, Any *ret) {
  static_cast<const FuncImpl<FuncType> *>(obj)->func_(num_args, args, ret);
}

template <typename FuncType>
void FuncCallUnpacked(const FuncObj *obj, int32_t num_args, const AnyView *args, Any *ret) {
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

} // namespace details

template <typename FuncType, typename> MLC_INLINE FuncObj *FuncObj::Allocator::New(FuncType func) {
  return details::FuncAllocatorImpl<FuncType>::Run(std::forward<FuncType>(func));
}

inline Ref<FuncObj> FuncObj::FromForeign(void *self, MLCDeleterType deleter, MLCFuncSafeCallType safe_call) {
  if (deleter == nullptr) {
    return Ref<FuncObj>::New([self, safe_call](int32_t num_args, const MLCAny *args, MLCAny *ret) {
      if (int32_t err_code = safe_call(self, num_args, args, ret); err_code != 0) {
        details::HandleSafeCallError(err_code, ret);
      }
    });
  } else {
    return Ref<FuncObj>::New(
        [self = std::shared_ptr<void>(self, deleter), safe_call](int32_t num_args, const MLCAny *args, MLCAny *ret) {
          if (int32_t err_code = safe_call(self.get(), num_args, args, ret); err_code != 0) {
            details::HandleSafeCallError(err_code, ret);
          }
        });
  }
}

/********** Section 6. ReflectionHelper *********/

namespace details {

template <typename FieldType> struct ReflectGetter {
  static int32_t Run(void *addr, MLCAny *ret) {
    MLC_SAFE_CALL_BEGIN();
    *static_cast<Any *>(ret) = *static_cast<FieldType *>(addr);
    MLC_SAFE_CALL_END(static_cast<Any *>(ret));
  }
};
template <typename FieldType> struct ReflectSetter {
  static int32_t Run(void *addr, MLCAny *src) {
    MLC_SAFE_CALL_BEGIN();
    *static_cast<FieldType *>(addr) = (static_cast<Any *>(src))->operator FieldType();
    MLC_SAFE_CALL_END(static_cast<Any *>(src));
  }
};
template <> struct ReflectGetter<char *> {
  static int32_t Run(void *addr, MLCAny *ret) {
    MLC_SAFE_CALL_BEGIN();
    *static_cast<Any *>(ret) = static_cast<const char *>(*static_cast<char **>(addr));
    MLC_SAFE_CALL_END(static_cast<Any *>(ret));
  }
};
template <> struct ReflectSetter<char *> {
  static int32_t Run(void *addr, MLCAny *src) {
    MLC_SAFE_CALL_BEGIN();
    *static_cast<char **>(addr) = const_cast<char *>((static_cast<Any *>(src))->operator const char *());
    MLC_SAFE_CALL_END(static_cast<Any *>(src));
  }
};

template <typename Super, typename FieldType>
inline ReflectionHelper &ReflectionHelper::FieldReadOnly(const char *name, FieldType Super::*field) {
  using PGet = ReflectGetter<FieldType>;
  const int64_t field_offset = static_cast<int64_t>(ReflectOffset(field));
  this->fields.emplace_back(MLCTypeField{name, field_offset, &PGet::Run, nullptr});
  return *this;
}

template <typename Super, typename FieldType>
inline ReflectionHelper &ReflectionHelper::Field(const char *name, FieldType Super::*field) {
  using PGet = ReflectGetter<FieldType>;
  using PSet = ReflectSetter<FieldType>;
  const int64_t field_offset = static_cast<int64_t>(ReflectOffset(field));
  this->fields.emplace_back(MLCTypeField{name, field_offset, &PGet::Run, &PSet::Run});
  return *this;
}

inline ReflectionHelper::ReflectionHelper(int32_t type_index)
    : type_index(type_index), fields(), methods(), method_pool() {}

inline std::string ReflectionHelper::DefaultStrMethod(AnyView any) {
  std::ostringstream os;
  os << details::TypeIndex2TypeKey(any.type_index) << '@' << any.v_ptr;
  return os.str();
}

template <typename Callable> inline ReflectionHelper &ReflectionHelper::Method(const char *name, Callable &&callable) {
  Ref<FuncObj> func_ref = Ref<FuncObj>::New(std::forward<Callable>(callable));
  for (auto &entry : this->methods) {
    if (std::strcmp(entry.name, name) == 0) {
      entry.func = func_ref.get();
      this->method_pool.push_back(std::move(func_ref));
      return *this;
    }
  }
  this->methods.emplace_back(MLCTypeMethod{name, func_ref.get()});
  this->method_pool.push_back(std::move(func_ref));
  return *this;
}

inline ReflectionHelper::operator int32_t() {
  if (!this->fields.empty() || !this->methods.empty()) {
    auto has_method = [&](const char *name) {
      for (const auto &entry : this->methods) {
        if (std::strcmp(entry.name, name) == 0) {
          return true;
        }
      }
      return false;
    };
    if (!has_method("__str__")) {
      this->Method("__str__", &ReflectionHelper::DefaultStrMethod);
    }
    MLCTypeDefReflection(nullptr, this->type_index,                //
                         this->fields.size(), this->fields.data(), //
                         this->methods.size(), this->methods.data());
  }
  return 0;
}
} // namespace details
} // namespace ffi
} // namespace mlc

#endif // MLC_FUNC_DETAILS_H_
