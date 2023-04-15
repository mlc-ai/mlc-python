#ifndef MLC_BASE_REF_H_
#define MLC_BASE_REF_H_

#include "./alloc.h"
#include "./any.h"
#include "./utils.h"
#include <type_traits>

#define MLC_DEF_OBJ_PTR_METHODS_(SelfType, ObjType, ParentType)                                                        \
private:                                                                                                               \
  template <typename> friend struct ::mlc::Ref;                                                                        \
  template <typename> friend struct ::mlc::base::Type2Str;                                                             \
  template <typename> friend struct ::mlc::base::ObjPtrHelper;                                                         \
  using TBase = ::mlc::base::PtrBase;                                                                                  \
  using TSelf = SelfType;                                                                                              \
  using THelper = ::mlc::base::ObjPtrHelper<SelfType>;                                                                 \
  template <typename U> using Derived = std::enable_if_t<::mlc::base::IsDerivedFrom<U, ObjType>>;                      \
  template <typename U> using DerivedObjRef = std::enable_if_t<::mlc::base::IsObjRefDerivedFrom<U, ObjType>>;          \
                                                                                                                       \
public:                                                                                                                \
  using TObj = ObjType;                                                                                                \
  MLC_INLINE ~SelfType() = default;                                                                                    \
  MLC_INLINE SelfType(::mlc::NullType) : ParentType(::mlc::Null) {}                                                    \
  MLC_INLINE SelfType &operator=(::mlc::NullType) { return this->Reset(); }                                            \
  MLC_INLINE const ObjType *operator->() const { return get(); }                                                       \
  MLC_INLINE ObjType *operator->() { return get(); }                                                                   \
  MLC_INLINE const ObjType &operator*() const {                                                                        \
    if (const ObjType *ret = get()) {                                                                                  \
      return *ret;                                                                                                     \
    }                                                                                                                  \
    MLC_THROW(ValueError) << "Attempt to dereference a null pointer";                                                  \
  }                                                                                                                    \
  MLC_INLINE ObjType &operator*() {                                                                                    \
    if (ObjType *ret = get()) {                                                                                        \
      return *ret;                                                                                                     \
    }                                                                                                                  \
    MLC_THROW(ValueError) << "Attempt to dereference a null pointer";                                                  \
    MLC_UNREACHABLE();                                                                                                 \
  }                                                                                                                    \
  MLC_INLINE SelfType &Reset() {                                                                                       \
    ParentType::Reset();                                                                                               \
    return *this;                                                                                                      \
  }                                                                                                                    \
  MLC_INLINE SelfType(const SelfType &src) : ParentType(::mlc::Null) {                                                 \
    this->_SetObjPtr(src.ptr);                                                                                         \
    this->IncRef();                                                                                                    \
  }                                                                                                                    \
  MLC_INLINE SelfType(SelfType &&src) : ParentType(::mlc::Null) {                                                      \
    this->_SetObjPtr(src.ptr);                                                                                         \
    src.ptr = nullptr;                                                                                                 \
  }                                                                                                                    \
  MLC_INLINE TSelf &operator=(const SelfType &other) {                                                                 \
    TSelf(other).Swap(*this);                                                                                          \
    return *this;                                                                                                      \
  }                                                                                                                    \
  MLC_INLINE TSelf &operator=(SelfType &&other) {                                                                      \
    TSelf(std::move(other)).Swap(*this);                                                                               \
    return *this;                                                                                                      \
  }                                                                                                                    \
  using ::mlc::base::PtrBase::operator==;                                                                              \
  using ::mlc::base::PtrBase::operator!=;                                                                              \
  using ::mlc::base::PtrBase::defined

namespace mlc {
namespace base {

template <typename TRef> struct ObjPtrHelper {
  using TObj = typename TRef::TObj;
  template <typename ObjPtrType> MLC_INLINE static MLCAny *GetPtr(const ObjPtrType *self) { return self->ptr; }
  template <typename ObjPtrType> MLC_INLINE static MLCAny *MovePtr(ObjPtrType *self) {
    MLCAny *ptr = self->ptr;
    self->ptr = nullptr;
    return ptr;
  }
};

struct PtrBase : public MLCObjPtr {
protected:
  MLC_INLINE ~PtrBase() { this->Reset(); }
  MLC_INLINE PtrBase() : MLCObjPtr{nullptr} {}
  MLC_INLINE PtrBase(::mlc::NullType) : MLCObjPtr{nullptr} {}
  MLC_INLINE PtrBase(MLCAny *v) : MLCObjPtr{v} {}
  MLC_INLINE void _SetObjPtr(MLCAny *v) { this->ptr = v; }
  MLC_INLINE void Reset() {
    this->DecRef();
    this->ptr = nullptr;
  }
  MLC_INLINE void IncRef() { ::mlc::base::IncRef(this->ptr); }
  MLC_INLINE void DecRef() { ::mlc::base::DecRef(this->ptr); }
  MLC_INLINE void Swap(PtrBase &other) { std::swap(this->ptr, other.ptr); }
  MLC_INLINE bool defined() const { return this->ptr != nullptr; }
  MLC_INLINE bool operator==(std::nullptr_t) const { return this->ptr == nullptr; }
  MLC_INLINE bool operator!=(std::nullptr_t) const { return this->ptr != nullptr; }
  template <typename T> MLC_INLINE void _Init(const MLCAny &v) {
    T *ret = [&v]() -> T * {
      MLC_TRY_CONVERT(TypeTraits<T *>::AnyToTypeOwned(&v), v.type_index, Type2Str<T *>::Run());
    }();
    this->ptr = reinterpret_cast<MLCAny *>(ret);
    this->IncRef();
  }
  template <typename T> MLC_INLINE void _InitPOD(T v) {
    using Alloc = ::mlc::PODAllocator<T>;
    this->ptr = Alloc::New(v);
    this->IncRef();
  }
};

struct ObjectRefDummyRoot : protected PtrBase {
  ObjectRefDummyRoot() : PtrBase() {}
  ObjectRefDummyRoot(NullType) : PtrBase() {}
};
} // namespace base
} // namespace mlc

namespace mlc {
template <typename T> struct Ref : protected ::mlc::base::PtrBase {
  MLC_DEF_OBJ_PTR_METHODS_(Ref<T>, T, ::mlc::base::PtrBase);
  /***** Section 1. Default constructor/destructors *****/
  MLC_INLINE Ref() : TBase() {}
  MLC_INLINE Ref(std::nullptr_t) : Ref(::mlc::Null) {}
  MLC_INLINE Ref &operator=(std::nullptr_t) { return (*this = ::mlc::Null); }
  /***** Section 2. From `U * / Ref<U> / ObjRef::U` where `U` is derived from `T` *****/
  template <typename U, typename = Derived<U>> /**/
  MLC_INLINE Ref(U *src) : TBase(reinterpret_cast<MLCAny *>(src)) {
    this->IncRef();
  }
  template <typename U, typename = Derived<U>> /**/
  MLC_INLINE Ref(const Ref<U> &src) : TBase(src.ptr) {
    this->IncRef();
  }
  template <typename U, typename = Derived<U>> /**/
  MLC_INLINE Ref(Ref<U> &&src) : TBase(src.ptr) {
    src.ptr = nullptr;
  }
  template <typename U, typename = Derived<U>> /**/
  MLC_INLINE TSelf &operator=(const Ref<U> &other) {
    TSelf(other).Swap(*this);
    return *this;
  }
  template <typename U, typename = Derived<U>> /**/
  MLC_INLINE TSelf &operator=(Ref<U> &&other) {
    TSelf(std::move(other)).Swap(*this);
    return *this;
  }
  template <typename U, typename = DerivedObjRef<U>> /**/
  MLC_INLINE Ref(const U &src) : TBase(src.ptr) {
    this->IncRef();
  }
  template <typename U, typename = DerivedObjRef<U>> /**/
  MLC_INLINE Ref(U &&src) : TBase(src.ptr) {
    src.ptr = nullptr;
  }
  template <typename U, typename = DerivedObjRef<U>> /**/
  MLC_INLINE TSelf &operator=(const U &other) {
    TSelf(other).Swap(*this);
    return *this;
  }
  template <typename U, typename = DerivedObjRef<U>> /**/
  MLC_INLINE TSelf &operator=(U &&other) {
    TSelf(std::move(other)).Swap(*this);
    return *this;
  }
  /***** Section 3. The `new` operator *****/
  template <typename... Args, typename = std::enable_if_t<::mlc::base::Newable<T, Args...>>>
  MLC_INLINE static TSelf New(Args &&...args) {
    return TSelf(::mlc::base::AllocatorOf<T>::New(std::forward<Args>(args)...));
  }
  template <size_t N> MLC_INLINE static TSelf New(const ::mlc::base::CharArray<N> &arg) {
    return TSelf(::mlc::base::AllocatorOf<T>::template New<N>(arg));
  }
  /***** Section 4. Conversion between AnyView/Any *****/
  MLC_INLINE Ref(const AnyView &src) : TBase() { TBase::_Init<T>(src); }
  MLC_INLINE Ref(const Any &src) : TBase() { TBase::_Init<T>(src); }
  MLC_INLINE operator AnyView() const { return AnyView(this->get()); }
  MLC_INLINE operator Any() const { return Any(this->get()); }
  /***** Section 5. Misc *****/
  Str str() const;
  MLC_INLINE bool operator==(const TSelf &rhs) const { return this->ptr == rhs.ptr; }
  MLC_INLINE bool operator!=(const TSelf &rhs) const { return this->ptr != rhs.ptr; }
  MLC_INLINE const T *get() const { return reinterpret_cast<const T *>(this->ptr); }
  MLC_INLINE T *get() { return reinterpret_cast<T *>(ptr); }
};

#define MLC_DEFINE_POD_REF(Type, Field)                                                                                \
  template <> struct Ref<Type> : protected ::mlc::base::PtrBase {                                                      \
    MLC_DEF_OBJ_PTR_METHODS_(Ref<Type>, Type, ::mlc::base::PtrBase);                                                   \
                                                                                                                       \
  public:                                                                                                              \
    /***** Section 1. Default constructor/destructors *****/                                                           \
    MLC_INLINE Ref() : TBase() {}                                                                                      \
    MLC_INLINE Ref(std::nullptr_t) : TBase() {}                                                                        \
    MLC_INLINE Ref &operator=(std::nullptr_t) { return this->Reset(); }                                                \
    /***** Section 2. The `new` operator *****/                                                                        \
    MLC_INLINE Ref(TObj arg) { TBase::_InitPOD<TObj>(arg); }                                                           \
    MLC_INLINE Ref &operator=(TObj arg) {                                                                              \
      this->Reset();                                                                                                   \
      this->_InitPOD<TObj>(arg);                                                                                       \
      return *this;                                                                                                    \
    }                                                                                                                  \
    MLC_INLINE_NO_MSVC static TSelf New(TObj arg) {                                                                    \
      TSelf ret;                                                                                                       \
      ret._InitPOD<TObj>(arg);                                                                                         \
      return ret;                                                                                                      \
    }                                                                                                                  \
    /***** Section 3. Conversion between AnyView/Any *****/                                                            \
    MLC_INLINE Ref(const AnyView &src) { TBase::_InitPOD<TObj>(src.operator TObj()); }                                 \
    MLC_INLINE Ref(const Any &src) { TBase::_InitPOD<TObj>(src.operator TObj()); }                                     \
    MLC_INLINE operator AnyView() const { return ::mlc::base::AnyViewFromPODPtr<TObj>(get()); }                        \
    MLC_INLINE operator Any() const { return ::mlc::base::AnyFromPODPtr<TObj>(get()); }                                \
    /***** Section 4. Misc *****/                                                                                      \
    Str str() const;                                                                                                   \
    MLC_INLINE bool operator==(const TSelf &rhs) const { return this->ptr == rhs.ptr; }                                \
    MLC_INLINE bool operator!=(const TSelf &rhs) const { return this->ptr != rhs.ptr; }                                \
    MLC_INLINE const TObj *get() const {                                                                               \
      return this->ptr ? &reinterpret_cast<const MLCBoxedPOD *>(this->ptr)->data.Field : nullptr;                      \
    }                                                                                                                  \
    MLC_INLINE TObj *get() { return this->ptr ? &reinterpret_cast<MLCBoxedPOD *>(this->ptr)->data.Field : nullptr; }   \
  }

MLC_DEFINE_POD_REF(int64_t, v_int64);
MLC_DEFINE_POD_REF(double, v_float64);
MLC_DEFINE_POD_REF(DLDevice, v_device);
MLC_DEFINE_POD_REF(DLDataType, v_dtype);
MLC_DEFINE_POD_REF(::mlc::base::VoidPtr, v_ptr);
#undef MLC_DEFINE_POD_REF

template <typename T, typename... Args> inline Ref<Object> InitOf(Args &&...args) {
  T *ptr = ::mlc::base::AllocatorOf<T>::New(std::forward<Args>(args)...);
  return Ref<Object>(reinterpret_cast<Object *>(ptr));
}
} // namespace mlc

#endif // MLC_BASE_REF_H_
