#ifndef MLC_REF_H_
#define MLC_REF_H_

#include "./utils.h"

namespace mlc {
namespace ffi {

struct NullType {};
constexpr NullType Null{};

template <typename TRef> struct ObjPtrHelper {
  using TObj = typename TRef::TObj;

  MLC_INLINE_NO_MSVC static TObj *TryConvert(const MLCAny *v) {
    MLC_TRY_CONVERT(ObjPtrTraits<TObj>::AnyToOwnedPtr(v), v->type_index, Type2Str<TObj *>::Run());
  }

  template <typename U, typename ObjPtrType> MLC_INLINE static MLCObject *GetPtr(const ObjPtrType *self) {
    static_assert(details::IsDerivedFrom<U, TObj>, "U must be derived from T");
    return self->ptr;
  }

  template <typename U, typename ObjPtrType> MLC_INLINE static MLCObject *MovePtr(ObjPtrType *self) {
    static_assert(details::IsDerivedFrom<U, TObj>, "U must be derived from T");
    MLCObject *ptr = self->ptr;
    self->ptr = nullptr;
    return ptr;
  }
};

struct ObjPtrBase : public MLCObjPtr {
protected:
  MLC_INLINE ~ObjPtrBase() { this->Reset(); }
  MLC_INLINE ObjPtrBase() : MLCObjPtr{nullptr} {}
  MLC_INLINE ObjPtrBase(MLCObject *v) : MLCObjPtr{v} {}
  MLC_INLINE void _SetObjPtr(MLCObject *v) { this->ptr = v; }
  MLC_INLINE void Reset() {
    this->DecRef();
    this->ptr = nullptr;
  }
  MLC_INLINE void IncRef() { details::IncRef(this->ptr); }
  MLC_INLINE void DecRef() { details::DecRef(this->ptr); }
  MLC_INLINE void Swap(ObjPtrBase &other) { std::swap(this->ptr, other.ptr); }
  MLC_INLINE friend std::ostream &operator<<(std::ostream &os, const ObjPtrBase &src) {
    MLCAny v{};
    if (src.ptr != nullptr) {
      v.type_index = src.ptr->type_index;
      v.v_obj = src.ptr;
    }
    details::AnyView2Str(os, &v);
    return os;
  }
};

template <typename T> struct Ref : private ObjPtrBase {
private:
  using TObj = T;
  using TSelf = Ref<T>;
  using THelper = ObjPtrHelper<TSelf>;
  using ObjPtrBase::_SetObjPtr;
  template <typename U>
  using EnableDerived = std::enable_if_t<std::is_same_v<T, details::RemoveCR<T>> && details::IsDerivedFrom<U, T>>;
  friend struct Any;
  friend struct AnyView;
  template <typename> friend struct Type2Str;
  template <typename, typename> friend struct tag::TagImpl;
  template <typename> friend struct ObjPtrHelper;
  template <typename> friend struct Ref;

public:
  /***** Default methods *****/
  ~Ref() = default;
  MLC_INLINE Ref() : ObjPtrBase() {}
  MLC_INLINE Ref(std::nullptr_t) : ObjPtrBase() {}
  MLC_INLINE Ref(NullType) : ObjPtrBase() {}
  MLC_INLINE Ref &operator=(std::nullptr_t) {
    this->Reset();
    return *this;
  }
  MLC_INLINE Ref &operator=(NullType) {
    this->Reset();
    return *this;
  }
  /***** Constructors and Converters *****/
  // Dispatchers for constructors
  template <typename U, typename = std::enable_if_t<tag::Exists<U>>>
  MLC_INLINE Ref(const U &src) : Ref(tag::Tag<U>{}, src) {}
  template <typename U, typename = std::enable_if_t<tag::Exists<U>>>
  MLC_INLINE Ref(U &&src) : Ref(tag::Tag<U>{}, std::forward<U>(src)) {}
  template <typename U, typename = std::enable_if_t<tag::Exists<U>>>
  MLC_DEF_ASSIGN(Ref, const U &)
  template <typename U, typename = std::enable_if_t<tag::Exists<U>>>
  MLC_DEF_ASSIGN(Ref, U &&)
  // Case 1. Ref<T>
  MLC_INLINE Ref(const Ref<T> &src) : ObjPtrBase(src.ptr) {
    this->IncRef();
  }
  MLC_INLINE Ref(Ref<T> &&src) : ObjPtrBase(src.ptr) { src.ptr = nullptr; }
  MLC_DEF_ASSIGN(TSelf, const Ref<T> &)
  MLC_DEF_ASSIGN(TSelf, Ref<T> &&)
  // Case 2. ObjPtr (Ref<U>, TObjectRef), Raw ObjPtr (U*)
  template <typename U>
  MLC_INLINE Ref(tag::ObjPtr, const U &src) : ObjPtrBase(THelper::template GetPtr<typename U::TObj>(&src)) {
    this->IncRef();
  }
  template <typename U>
  MLC_INLINE Ref(tag::ObjPtr, U &&src) : ObjPtrBase(THelper::template MovePtr<typename U::TObj>(&src)) {}
  template <typename U, typename = EnableDerived<U>>
  MLC_INLINE Ref(tag::RawObjPtr, U *src) : ObjPtrBase(reinterpret_cast<MLCObject *>(src)) {
    this->IncRef();
  }
  /***** Factory: the `new` operator *****/
  template <typename... Args, typename = std::enable_if_t<details::Newable<T, Args...>>>
  MLC_INLINE static TSelf New(Args &&...args) {
    return TSelf(details::AllocatorOf<T>::New(std::forward<Args>(args)...));
  }
  template <size_t N> MLC_INLINE static TSelf New(const CharArray<N> &arg) {
    return TSelf(details::AllocatorOf<T>::template New<N>(arg));
  }
  /***** Accessors *****/
  MLC_INLINE const T *get() const { return reinterpret_cast<const T *>(this->ptr); }
  MLC_INLINE const T *operator->() const { return get(); }
  MLC_INLINE const T &operator*() const { return *get(); }
  MLC_INLINE T *get() { return reinterpret_cast<T *>(ptr); }
  MLC_INLINE T *operator->() { return get(); }
  MLC_INLINE T &operator*() { return *get(); }
  /***** Comparator *****/
  template <typename U> MLC_INLINE bool operator==(const Ref<U> &rhs) const { return this->ptr == rhs.ptr; }
  template <typename U> MLC_INLINE bool operator!=(const Ref<U> &rhs) const { return this->ptr != rhs.ptr; }
  MLC_INLINE bool defined() const { return this->ptr != nullptr; }
  MLC_INLINE bool operator==(std::nullptr_t) const { return this->ptr == nullptr; }
  MLC_INLINE bool operator!=(std::nullptr_t) const { return this->ptr != nullptr; }
  /***** Misc *****/
  Ref<StrObj> str() const;
};

template <typename T, typename... Args> Ref<Object> InitOf(Args &&...args) {
  T *ptr = details::AllocatorOf<T>::New(std::forward<Args>(args)...);
  return Ref<Object>(reinterpret_cast<Object *>(ptr));
}

struct ObjectRefDummyRoot : protected ObjPtrBase {
  ObjectRefDummyRoot(NullType) : ObjPtrBase() {}
};

#define MLC_DEF_OBJ_REF(RefType, ObjType, ParentRefType)                                                               \
private:                                                                                                               \
  using TObj = ObjType;                                                                                                \
  using TSelf = RefType;                                                                                               \
  using THelper = ::mlc::ffi::ObjPtrHelper<RefType>;                                                                   \
  template <typename U>                                                                                                \
  using EnableDerived = std::enable_if_t<                                                                              \
      std::is_same_v<TObj, ::mlc::ffi::details::RemoveCR<TObj>> && ::mlc::ffi::details::IsDerivedFrom<U, TObj>>;       \
  template <typename, typename> friend struct ::mlc::ffi::tag::TagImpl;                                                \
  template <typename> friend struct ::mlc::ffi::ObjPtrHelper;                                                          \
  template <typename> friend struct ::mlc::ffi::Ref;                                                                   \
  friend struct ::mlc::ffi::Any;                                                                                       \
  friend struct ::mlc::ffi::AnyView;                                                                                   \
  template <typename> friend struct ::mlc::ffi::Type2Str;                                                              \
                                                                                                                       \
public:                                                                                                                \
  /***** Default methods *****/                                                                                        \
  MLC_INLINE ~RefType() = default;                                                                                     \
  MLC_INLINE RefType(::mlc::ffi::NullType) : ParentRefType(::mlc::ffi::Null) { this->_SetObjPtr(nullptr); }            \
  RefType(std::nullptr_t) = delete;                                                                                    \
  template <typename... Args, typename = std::enable_if_t<::mlc::ffi::details::Newable<TObj, Args...>>>                \
  MLC_INLINE RefType(Args &&...args)                                                                                   \
      : RefType(::mlc::ffi::details::AllocatorOf<TObj>::New(std::forward<Args>(args)...)) {}                           \
  /***** Accessors *****/                                                                                              \
  MLC_INLINE const TObj *get() const { return reinterpret_cast<const TObj *>(this->ptr); }                             \
  MLC_INLINE const TObj *operator->() const { return get(); }                                                          \
  MLC_INLINE const TObj &operator*() const { return *get(); }                                                          \
  MLC_INLINE TObj *get() { return reinterpret_cast<TObj *>(ptr); }                                                     \
  MLC_INLINE TObj *operator->() { return get(); }                                                                      \
  MLC_INLINE TObj &operator*() { return *get(); }                                                                      \
  /***** Comparator *****/                                                                                             \
  template <typename U> MLC_INLINE bool operator==(const TSelf &rhs) const { return this->ptr == rhs.ptr; }            \
  template <typename U> MLC_INLINE bool operator!=(const TSelf &rhs) const { return this->ptr != rhs.ptr; }            \
  MLC_INLINE bool defined() const { return this->ptr != nullptr; }                                                     \
  MLC_INLINE bool operator==(std::nullptr_t) const { return this->ptr == nullptr; }                                    \
  MLC_INLINE bool operator!=(std::nullptr_t) const { return this->ptr != nullptr; }                                    \
  /***** Constructors and Converters *****/                                                                            \
  MLC_INLINE RefType(const RefType &src) : RefType(::mlc::ffi::Null) {                                                 \
    this->_SetObjPtr(src.ptr);                                                                                         \
    this->IncRef();                                                                                                    \
  }                                                                                                                    \
  MLC_INLINE RefType(RefType &&src) : RefType(::mlc::ffi::Null) {                                                      \
    this->_SetObjPtr(src.ptr);                                                                                         \
    src.ptr = nullptr;                                                                                                 \
  }                                                                                                                    \
  MLC_DEF_ASSIGN(RefType, const RefType &)                                                                             \
  MLC_DEF_ASSIGN(RefType, RefType &&)                                                                                  \
  MLC_INLINE RefType(const ::mlc::ffi::Ref<TObj> &src) : RefType(::mlc::ffi::Null) {                                   \
    this->_SetObjPtr(THelper::template GetPtr<TObj>(&src));                                                            \
    this->IncRef();                                                                                                    \
  }                                                                                                                    \
  MLC_INLINE RefType(::mlc::ffi::Ref<TObj> &&src) : RefType(::mlc::ffi::Null) {                                        \
    this->_SetObjPtr(THelper::template MovePtr<TObj>(&src));                                                           \
  }                                                                                                                    \
  template <typename U, typename = EnableDerived<U>> MLC_INLINE RefType(U *src) : RefType(::mlc::ffi::Null) {          \
    this->_SetObjPtr(reinterpret_cast<MLCObject *>(src));                                                              \
    this->IncRef();                                                                                                    \
  }                                                                                                                    \
  friend struct Any;                                                                                                   \
  friend struct AnyView;                                                                                               \
  MLC_DEF_REFLECTION(ObjType)

namespace details {
template <typename> struct IsRefImpl {
  static constexpr bool value = false;
};
template <typename T> struct IsRefImpl<Ref<T>> {
  static constexpr bool value = true;
};
} // namespace details
} // namespace ffi
} // namespace mlc

#endif // MLC_REF_H_
