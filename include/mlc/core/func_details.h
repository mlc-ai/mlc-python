#ifndef MLC_CORE_FUNC_DETAILS_H_
#define MLC_CORE_FUNC_DETAILS_H_

#include "./func.h"
#include <cstring>
#include <iomanip>
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

/********** Section 4. ReflectionHelper *********/

template <typename FieldType> struct ReflectGetterSetter {
  static int32_t Getter(MLCTypeField *, void *addr, MLCAny *ret) {
    MLC_SAFE_CALL_BEGIN();
    *static_cast<Any *>(ret) = *static_cast<FieldType *>(addr);
    MLC_SAFE_CALL_END(static_cast<Any *>(ret));
  }
  static int32_t Setter(MLCTypeField *, void *addr, MLCAny *src) {
    MLC_SAFE_CALL_BEGIN();
    *static_cast<FieldType *>(addr) = (static_cast<Any *>(src))->operator FieldType();
    MLC_SAFE_CALL_END(static_cast<Any *>(src));
  }
};

template <> struct ReflectGetterSetter<char *> : public ReflectGetterSetter<const char *> {};

template <typename Super, typename FieldType>
inline MLCTypeField ReflectionHelper::PrepareField(const char *name, FieldType Super::*field) {
  int64_t field_offset = static_cast<int64_t>(ReflectOffset(field));
  MLCTypeInfo **ann = [&]() {
    this->type_annotation_pool.emplace_back();
    std::vector<MLCTypeInfo *> &type_annotation = this->type_annotation_pool.back();
    base::Type2Str<FieldType>::GetTypeAnnotation(&type_annotation);
    type_annotation.push_back(nullptr);
    return type_annotation.data();
  }();
  int32_t is_read_only = false;
  int32_t is_owned_obj_ptr = base::IsObjRef<FieldType> || base::IsRef<FieldType>;
  return MLCTypeField{
      name, field_offset, nullptr, nullptr, ann, is_read_only, is_owned_obj_ptr,
  };
}

template <typename Super, typename FieldType>
inline ReflectionHelper &ReflectionHelper::FieldReadOnly(const char *name, FieldType Super::*field) {
  MLCTypeField f = this->PrepareField<Super, FieldType>(name, field);
  using P = ::mlc::core::ReflectGetterSetter<FieldType>;
  f.getter = &P::Getter;
  f.setter = nullptr;
  f.is_read_only = true;
  this->fields.emplace_back(f);
  return *this;
}

template <typename Super, typename FieldType>
inline ReflectionHelper &ReflectionHelper::Field(const char *name, FieldType Super::*field) {
  MLCTypeField f = this->PrepareField<Super, FieldType>(name, field);
  using P = ::mlc::core::ReflectGetterSetter<FieldType>;
  f.getter = &P::Getter;
  f.setter = &P::Setter;
  this->fields.emplace_back(f);
  return *this;
}

inline ReflectionHelper::ReflectionHelper(int32_t type_index)
    : type_index(type_index), fields(), methods(), method_pool() {}

inline std::string ReflectionHelper::DefaultStrMethod(AnyView any) {
  std::ostringstream os;
  os << ::mlc::base::TypeIndex2TypeKey(any.type_index) //
     << "@0x" << std::setfill('0') << std::setw(12)    //
     << std::hex << (uintptr_t)(any.v_ptr);
  return os.str();
}

template <typename Callable>
inline MLCTypeMethod ReflectionHelper::PrepareMethod(const char *name, Callable &&callable) {
  Ref<FuncObj> func_ref = Ref<FuncObj>::New(std::forward<Callable>(callable));
  MLCTypeMethod ret{name, func_ref.get(), -1};
  this->method_pool.push_back(std::move(func_ref));
  return ret;
}

template <typename Callable> inline ReflectionHelper &ReflectionHelper::MemFn(const char *name, Callable &&callable) {
  constexpr int32_t MemFn = 0;
  MLCTypeMethod m = this->PrepareMethod(name, std::forward<Callable>(callable));
  m.kind = MemFn;
  this->methods.emplace_back(m);
  return *this;
}

template <typename Callable> ReflectionHelper &ReflectionHelper::StaticFn(const char *name, Callable &&callable) {
  constexpr int32_t StaticFn = 1;
  MLCTypeMethod m = this->PrepareMethod(name, std::forward<Callable>(callable));
  m.kind = StaticFn;
  this->methods.emplace_back(m);
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
      this->MemFn("__str__", &ReflectionHelper::DefaultStrMethod);
    }
    MLCTypeDefReflection(nullptr, this->type_index,                //
                         this->fields.size(), this->fields.data(), //
                         this->methods.size(), this->methods.data());
  }
  return 0;
}
} // namespace core
} // namespace mlc

namespace mlc {
template <typename R, typename... Args> MLC_INLINE std::string FuncTraitsImpl<R, Args...>::Sig() {
  return ::mlc::core::Func2Str<R, Args...>::Run(std::index_sequence_for<Args...>{});
}
template <typename FuncType, typename> MLC_INLINE FuncObj *FuncObj::Allocator::New(FuncType func) {
  return ::mlc::core::FuncAllocatorImpl<FuncType>::Run(std::forward<FuncType>(func));
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
