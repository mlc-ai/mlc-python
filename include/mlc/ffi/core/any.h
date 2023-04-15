#ifndef MLC_ANY_H_
#define MLC_ANY_H_
#include "./utils.h"
#include <cstring>
#include <type_traits>
#include <utility>

namespace mlc {
namespace ffi {

struct AnyView : public MLCAny {
  friend struct Any;
  /***** Default methods *****/
  MLC_INLINE ~AnyView() = default;
  MLC_INLINE AnyView() : MLCAny() {}
  MLC_INLINE void Reset() { *(static_cast<MLCAny *>(this)) = MLCAny(); }
  /***** Constructors and Converters *****/
  // Dispatchers for constructors
  // clang-format off
  template <typename T, typename = std::enable_if_t<tag::Exists<T>>> MLC_INLINE AnyView(const T &src) : AnyView(tag::Tag<T>{}, src) {}
  template <typename T, typename = std::enable_if_t<tag::Exists<T>>> MLC_INLINE AnyView(T &&src) : AnyView(tag::Tag<T>{}, std::forward<T>(src)) {}
  template <typename T, typename = std::enable_if_t<tag::Exists<T>>> MLC_DEF_ASSIGN(AnyView, const T &)
  template <typename T, typename = std::enable_if_t<tag::Exists<T>>> MLC_DEF_ASSIGN(AnyView, T &&)
  template <typename T, typename = std::enable_if_t<tag::Exists<T>>> MLC_INLINE operator T() const { return this->Cast<T>(tag::Tag<T>{}); }
  // clang-format on
  // Case 1. AnyView, Any
  MLC_INLINE AnyView(const AnyView &src) = default;
  MLC_INLINE AnyView(AnyView &&src) = default;
  MLC_INLINE AnyView &operator=(const AnyView &src) = default;
  MLC_INLINE AnyView &operator=(AnyView &&src) = default;
  MLC_INLINE AnyView(const Any &src);
  MLC_INLINE AnyView(Any &&src);
  MLC_INLINE AnyView &operator=(const Any &src);
  MLC_INLINE AnyView &operator=(Any &&src);
  // Case 2. ObjPtr (Ref<T>, TObjectRef)
  MLC_INLINE AnyView(tag::ObjPtr, const MLCObjPtr &src);
  MLC_INLINE AnyView(tag::ObjPtr, MLCObjPtr &&src);
  template <typename T> MLC_INLINE T Cast(tag::ObjPtr) const;
  // Case 3. POD types
  template <typename T> MLC_INLINE AnyView(tag::POD, const T &src);
  template <typename T> MLC_INLINE AnyView(tag::POD, T &&src);
  template <typename T> MLC_INLINE_NO_MSVC T Cast(tag::POD) const;
  // Case 4. Raw pointers (T*)
  MLC_INLINE AnyView(std::nullptr_t) : MLCAny() {}
  MLC_INLINE AnyView &operator=(std::nullptr_t) {
    this->Reset();
    return *this;
  }
  template <typename T> MLC_INLINE AnyView(tag::RawObjPtr, T *src);
  template <typename T> MLC_INLINE_NO_MSVC T Cast(tag::RawObjPtr) const;
  template <typename T> MLC_INLINE_NO_MSVC T *CastWithStorage(Any *storage) const;
  /***** Misc *****/
  Ref<StrObj> str() const;
  MLC_INLINE friend std::ostream &operator<<(std::ostream &os, const AnyView &src) {
    details::AnyView2Str(os, &src);
    return os;
  }
  template <typename T> T Cast() const;

protected:
  MLC_INLINE void Swap(MLCAny &src) {
    MLCAny tmp = *this;
    *static_cast<MLCAny *>(this) = src;
    src = tmp;
  }
};

struct Any : public MLCAny {
  friend struct AnyView;
  /***** Default methods *****/
  MLC_INLINE ~Any() { this->Reset(); }
  MLC_INLINE Any() : MLCAny() {}
  MLC_INLINE void Reset() {
    this->DecRef();
    *(static_cast<MLCAny *>(this)) = MLCAny();
  }
  /***** Constructors and Converters *****/
  // Dispatchers for constructors
  // clang-format off
  template <typename T, typename = std::enable_if_t<tag::Exists<T>>> MLC_INLINE Any(const T &src) : Any(tag::Tag<T>{}, src) {}
  template <typename T, typename = std::enable_if_t<tag::Exists<T>>> MLC_INLINE Any(T &&src) : Any(tag::Tag<T>{}, std::forward<T>(src)) {}
  template <typename T, typename = std::enable_if_t<tag::Exists<T>>> MLC_DEF_ASSIGN(Any, const T &)
  template <typename T, typename = std::enable_if_t<tag::Exists<T>>> MLC_DEF_ASSIGN(Any, T &&)
  template <typename T, typename = std::enable_if_t<tag::Exists<T>>> MLC_INLINE operator T() const { return this->Cast<T>(tag::Tag<T>{}); }
  // clang-format on
  // Case 1. AnyView, Any
  MLC_INLINE Any(const Any &src) : MLCAny(*static_cast<const MLCAny *>(&src)) { this->IncRef(); }
  MLC_INLINE Any(Any &&src) : MLCAny(*static_cast<const MLCAny *>(&src)) { *static_cast<MLCAny *>(&src) = MLCAny(); }
  MLC_INLINE Any(const AnyView &src) : MLCAny(*static_cast<const MLCAny *>(&src)) {
    if (this->type_index == static_cast<int32_t>(MLCTypeIndex::kMLCRawStr)) {
      // Special case: handle the case where `Any` needs to own a raw string.
      this->type_index = static_cast<int32_t>(MLCTypeIndex::kMLCStr);
      this->v_obj = details::StrCopyFromCharArray(this->v_str, std::strlen(this->v_str));
    }
    this->IncRef();
  }
  MLC_INLINE Any(AnyView &&src) : Any(static_cast<const AnyView &>(src)) { src.Reset(); }
  MLC_DEF_ASSIGN(Any, const Any &)
  MLC_DEF_ASSIGN(Any, Any &&)
  MLC_DEF_ASSIGN(Any, const AnyView &)
  MLC_DEF_ASSIGN(Any, AnyView &&)
  // Case 2. ObjPtr (Ref<T>, TObjectRef)
  MLC_INLINE Any(tag::ObjPtr, const MLCObjPtr &src);
  MLC_INLINE Any(tag::ObjPtr, MLCObjPtr &&src);
  template <typename T> MLC_INLINE T Cast(tag::ObjPtr) const;
  // Case 3. POD types
  template <typename T> MLC_INLINE Any(tag::POD, const T &src) : Any(AnyView(src)) {}
  template <typename T> MLC_INLINE Any(tag::POD, T &&src) : Any(AnyView(std::move(src))) {}
  template <typename T> MLC_INLINE_NO_MSVC T Cast(tag::POD) const;
  // Case 4. Raw pointers (T*)
  MLC_INLINE Any(std::nullptr_t) : MLCAny() {}
  MLC_INLINE Any &operator=(std::nullptr_t) {
    this->Reset();
    return *this;
  }
  template <typename T> MLC_INLINE Any(tag::RawObjPtr, T *src);
  template <typename T> MLC_INLINE_NO_MSVC T Cast(tag::RawObjPtr) const;
  template <typename T> MLC_INLINE_NO_MSVC T *CastWithStorage(Any *storage) const;
  /***** Misc *****/
  Ref<StrObj> str() const;
  MLC_INLINE friend std::ostream &operator<<(std::ostream &os, const Any &src) {
    details::AnyView2Str(os, &src);
    return os;
  }
  template <typename T> T Cast() const;

protected:
  MLC_INLINE void Swap(MLCAny &src) {
    MLCAny tmp = *this;
    *static_cast<MLCAny *>(this) = src;
    src = tmp;
  }
  MLC_INLINE void IncRef() {
    if (!details::IsTypeIndexPOD(this->type_index)) {
      details::IncRef(this->v_obj);
    }
  }
  MLC_INLINE void DecRef() {
    if (!details::IsTypeIndexPOD(this->type_index)) {
      details::DecRef(this->v_obj);
    }
  }
};

template <std::size_t N> struct AnyViewArray {
  template <typename... Args> void Fill(Args &&...args);
  AnyView v[N];
};

template <> struct AnyViewArray<0> {
  template <typename... Args> void Fill(Args &&...args);
  AnyView *v = nullptr;
};

template <typename T> T AnyView::Cast() const {
  if constexpr (std::is_same_v<T, Any> || std::is_same_v<T, AnyView>) {
    return *this;
  } else {
    return this->operator T();
  }
}

template <typename T> T Any::Cast() const {
  if constexpr (std::is_same_v<T, Any> || std::is_same_v<T, AnyView>) {
    return *this;
  } else {
    return this->operator T();
  }
}

namespace details {
template <typename... Args> struct AnyViewArrayFill {
  template <std::size_t i, typename Arg> MLC_INLINE static void Apply(AnyView *v, Arg &&arg) {
    v[i] = AnyView(std::forward<Arg>(arg));
  }
  template <std::size_t... I> MLC_INLINE static void Run(AnyView *v, Args &&...args, std::index_sequence<I...>) {
    using TExpander = int[];
    (void)TExpander{0, (Apply<I>(v, std::forward<Args>(args)), 0)...};
  }
};
} // namespace details

template <std::size_t N> template <typename... Args> MLC_INLINE void AnyViewArray<N>::Fill(Args &&...args) {
  static_assert(sizeof...(args) == N, "Invalid number of arguments");
  if constexpr (N > 0) {
    using IdxSeq = std::make_index_sequence<N>;
    details::AnyViewArrayFill<Args...>::Run(v, std::forward<Args>(args)..., IdxSeq{});
  }
}

template <typename... Args> MLC_INLINE void AnyViewArray<0>::Fill(Args &&...args) {
  static_assert(sizeof...(args) == 0, "Invalid number of arguments");
}

} // namespace ffi
} // namespace mlc

#endif // MLC_CORE_IMPL_H_
