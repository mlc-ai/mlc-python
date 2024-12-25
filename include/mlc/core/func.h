#ifndef MLC_CORE_FUNC_H_
#define MLC_CORE_FUNC_H_

#include "./error.h"
#include "./str.h"
#include <array>
#include <type_traits>

#define MLC_REGISTER_FUNC(name) [[maybe_unused]] static auto MLC_UNIQUE_ID() = ::mlc::core::FuncRegistryHelper(name)

namespace mlc {
struct FuncObj : public MLCFunc {
  using Call = void(const FuncObj *, int32_t, const AnyView *, Any *);
  using SafeCall = int32_t(const FuncObj *, int32_t, const AnyView *, Any *);
  struct Allocator;

  template <typename... Args> MLC_INLINE Any operator()(Args &&...args) const {
    constexpr size_t N = sizeof...(Args);
    AnyViewArray<N> stack_args;
    Any ret;
    stack_args.Fill(std::forward<Args>(args)...);
    ::mlc::base::FuncCall(this, N, stack_args.v, &ret);
    return ret;
  }

  static Ref<FuncObj> FromForeign(void *self, MLCDeleterType deleter, MLCFuncSafeCallType safe_call);

  static int32_t SafeCallImpl(const FuncObj *self, int32_t num_args, const AnyView *args, Any *ret) {
    MLC_SAFE_CALL_BEGIN();
    self->call(self, num_args, args, ret);
    MLC_SAFE_CALL_END(ret);
  }

  MLC_INLINE FuncObj(Call *f) : MLCFunc() {
    this->MLCFunc::call = reinterpret_cast<MLCFuncCallType>(f);
    this->MLCFunc::safe_call = reinterpret_cast<MLCFuncSafeCallType>(FuncObj::SafeCallImpl);
  }

  MLC_DEF_STATIC_TYPE(FuncObj, Object, MLCTypeIndex::kMLCFunc, "object.Func");
};

struct FuncObj::Allocator {
public:
  template <typename FuncType, typename = std::enable_if_t<::mlc::base::HasFuncTraits<FuncType>>>
  MLC_INLINE static FuncObj *New(FuncType func);
};

struct Func : public ObjectRef {
  template <typename... Args> MLC_INLINE Any operator()(Args &&...args) const {
    return get()->operator()(std::forward<Args>(args)...);
  }
  template <typename FuncType, typename = std::enable_if_t<::mlc::base::HasFuncTraits<FuncType>>>
  Func(FuncType func) : Func(FuncObj::Allocator::New(std::move(func))) {}
  static Any GetGlobal(const char *name) {
    Any ret;
    ::MLCFuncGetGlobal(nullptr, name, &ret);
    return ret;
  }
  // A dummy function to trigger reflection - otherwise reflection registration will be skipped
  MLC_DEF_OBJ_REF(Func, FuncObj, ObjectRef).StaticFn("__nothing__", []() {});
};
} // namespace mlc

namespace mlc {
namespace core {
template <typename FuncType> struct FuncImpl : public FuncObj {
  using TSelf = FuncImpl<FuncType>;
  using Allocator = ::mlc::DefaultObjectAllocator<TSelf>;
  MLC_INLINE FuncImpl(FuncType func, FuncObj::Call *f) : FuncObj(f), func_(std::forward<FuncType>(func)) {}
  mutable std::decay_t<FuncType> func_;
};

struct FuncRegistryHelper {
  explicit FuncRegistryHelper(const char *name) : name(name) {}
  template <typename FuncType> FuncRegistryHelper &set_body(FuncType func, bool allow_override = false) {
    ::MLCFuncSetGlobal(nullptr, name, Any(Ref<FuncObj>::New(std::move(func))), allow_override);
    return *this;
  }
  const char *name;
};

MLC_INLINE void HandleSafeCallError(int32_t err_code, MLCAny *ret) noexcept(false) {
  if (err_code == -1) { // string errors
    MLC_THROW(InternalError) << "Error: " << *static_cast<Any *>(ret);
  } else if (err_code == -2) { // error objects
    throw Exception(static_cast<Any *>(ret)->operator Ref<ErrorObj>()->AppendWith(MLC_TRACEBACK_HERE()));
  } else { // error code
    MLC_THROW(InternalError) << "Error code: " << err_code;
  }
  MLC_UNREACHABLE();
}
} // namespace core
} // namespace mlc

namespace mlc {
namespace base {
MLC_INLINE void FuncCall(const void *self, int32_t num_args, const MLCAny *args, MLCAny *ret) {
  const MLCFunc *func = static_cast<const MLCFunc *>(self);
  if (func->call && reinterpret_cast<void *>(func->safe_call) == reinterpret_cast<void *>(FuncObj::SafeCallImpl)) {
    func->call(func, num_args, args, ret);
  } else if (int32_t err_code = func->safe_call(func, num_args, args, ret)) {
    ::mlc::core::HandleSafeCallError(err_code, ret);
  }
}
template <int32_t num_args> inline auto GetGlobalFuncCall(const char *name) {
  using ArrayType = std::array<AnyView, num_args>;
  FuncObj *func = Func::GetGlobal(name);
  return [func](ArrayType &&args) {
    Any ret;
    ::mlc::base::FuncCall(func, num_args, std::move(args).data(), &ret);
    return ret;
  };
}
template <typename Callable> inline Any CallableToAny(Callable &&callable) {
  return Ref<FuncObj>::New(std::forward<Callable>(callable));
}
} // namespace base
} // namespace mlc

#endif // MLC_CORE_FUNC_H_
