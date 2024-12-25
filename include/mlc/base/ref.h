#ifndef MLC_BASE_REF_H_
#define MLC_BASE_REF_H_

#include "./alloc.h"
#include "./utils.h"
#include <type_traits>

#define MLC_DEF_RTTI_METHODS(CheckNulll, ObjPtrMut, ObjPtrConst)                                                       \
  template <typename DerivedObj> MLC_INLINE bool IsInstance() const {                                                  \
    if constexpr (CheckNulll) {                                                                                        \
      if ((ObjPtrConst) == nullptr) {                                                                                  \
        return false;                                                                                                  \
      }                                                                                                                \
    }                                                                                                                  \
    return ::mlc::base::IsInstanceOf<DerivedObj, TObj>(ObjPtrConst);                                                   \
  }                                                                                                                    \
  template <typename DerivedObj> MLC_INLINE DerivedObj *TryCast() {                                                    \
    return this->IsInstance<DerivedObj>() ? reinterpret_cast<DerivedObj *>(ObjPtrMut) : nullptr;                       \
  }                                                                                                                    \
  template <typename DerivedObj> MLC_INLINE const DerivedObj *TryCast() const {                                        \
    return this->IsInstance<DerivedObj>() ? reinterpret_cast<const DerivedObj *>(ObjPtrConst) : nullptr;               \
  }                                                                                                                    \
  template <typename DerivedObj> inline DerivedObj *Cast() {                                                           \
    if constexpr (CheckNulll) {                                                                                        \
      if ((ObjPtrConst) == nullptr) {                                                                                  \
        return nullptr;                                                                                                \
      }                                                                                                                \
    }                                                                                                                  \
    if (::mlc::base::IsInstanceOf<DerivedObj, TObj>(ObjPtrMut)) {                                                      \
      return reinterpret_cast<DerivedObj *>(ObjPtrMut);                                                                \
    }                                                                                                                  \
    MLC_THROW(TypeError) << "Cannot cast from type `" << ::mlc::base::TypeIndex2TypeKey(ObjPtrMut) << "` to type `"    \
                         << ::mlc::base::Type2Str<DerivedObj>::Run() << "`";                                           \
    MLC_UNREACHABLE();                                                                                                 \
  }                                                                                                                    \
  template <typename DerivedObj> MLC_INLINE const DerivedObj *Cast() const {                                           \
    if constexpr (CheckNulll) {                                                                                        \
      if ((ObjPtrConst) == nullptr) {                                                                                  \
        return nullptr;                                                                                                \
      }                                                                                                                \
    }                                                                                                                  \
    if (::mlc::base::IsInstanceOf<DerivedObj, TObj>(ObjPtrConst)) {                                                    \
      return reinterpret_cast<const DerivedObj *>(ObjPtrConst);                                                        \
    }                                                                                                                  \
    MLC_THROW(TypeError) << "Cannot cast from type `" << ::mlc::base::TypeIndex2TypeKey(ObjPtrConst) << "` to type `"  \
                         << ::mlc::base::Type2Str<DerivedObj>::Run() << "`";                                           \
    MLC_UNREACHABLE();                                                                                                 \
  }                                                                                                                    \
  MLC_INLINE const char *GetTypeKey() const { return ::mlc::base::TypeIndex2TypeKey(ObjPtrConst); }                    \
  MLC_INLINE int32_t GetTypeIndex() const { return ::mlc::base::TypeIndexOf(ObjPtrConst); }

namespace mlc {
namespace base {

struct PtrBase : public MLCObjPtr {
protected:
  MLC_INLINE ~PtrBase() { this->Reset(); }
  MLC_INLINE PtrBase() : MLCObjPtr{nullptr} {}
  MLC_INLINE PtrBase(::mlc::NullType) : MLCObjPtr{nullptr} {}
  MLC_INLINE PtrBase(MLCAny *v) : MLCObjPtr{v} {}
  MLC_INLINE PtrBase(const MLCAny *v) : MLCObjPtr{const_cast<MLCAny *>(v)} {}
  MLC_INLINE void _Set(MLCAny *v) { this->ptr = v; }
  MLC_INLINE void _Set(const MLCAny *v) { this->ptr = const_cast<MLCAny *>(v); }
  MLC_INLINE void _Set(MLCObjPtr &v) { this->ptr = v.ptr; }
  MLC_INLINE void _Set(const MLCObjPtr &v) { this->ptr = const_cast<MLCAny *>(v.ptr); }
  MLC_INLINE void _Move(MLCObjPtr &&v) {
    this->ptr = v.ptr;
    v.ptr = nullptr;
  }
  MLC_INLINE void IncRef() { ::mlc::base::IncRef(this->ptr); }
  MLC_INLINE void DecRef() { ::mlc::base::DecRef(this->ptr); }
  MLC_INLINE void Swap(PtrBase &other) { std::swap(this->ptr, other.ptr); }
  template <typename T> inline void _Init(const MLCAny *v) {
    if (::mlc::base::IsTypeIndexNone(v->type_index)) {
      this->ptr = nullptr;
      return;
    }
    T *ret = [v]() -> T * {
      MLC_TRY_CONVERT(TypeTraits<T *>::AnyToTypeOwned(v), v->type_index, Type2Str<T *>::Run());
    }();
    this->ptr = reinterpret_cast<MLCAny *>(ret);
    this->IncRef();
  }
  template <typename T> MLC_INLINE void _InitPOD(T v) {
    this->ptr = ::mlc::PODAllocator<T>::New(v);
    this->IncRef();
  }
  MLC_INLINE void Reset() {
    this->DecRef();
    this->ptr = nullptr;
  }
  template <typename T> MLC_INLINE void CheckNull() {
    if (this->ptr == nullptr) {
      MLC_THROW(TypeError) << "Cannot convert from type `None` to non-nullable `" << ::mlc::base::Type2Str<T>::Run()
                           << "`";
    }
  }
};

} // namespace base
} // namespace mlc

namespace mlc {
template <typename _TObj> struct Ref : protected ::mlc::base::PtrBase {
public:
  using TObj = _TObj;
  using TSelf = Ref<TObj>;
  [[maybe_unused]] static constexpr ::mlc::base::TypeKind _type_kind = ::mlc::base::TypeKind::kRef;

private:
  using TBase = ::mlc::base::PtrBase;
  template <typename U> using DObj = std::enable_if_t<::mlc::base::IsDerivedFrom<U, TObj>>;
  template <typename U> using DObjRef = std::enable_if_t<::mlc::base::IsDerivedFromObjRef<U, TObj>>;
  template <typename U> using DOpt = std::enable_if_t<::mlc::base::IsDerivedFromOpt<U, TObj>>;

public:
  /***** Section 1. Default constructor/destructors *****/
  MLC_INLINE Ref() : TBase() {}
  MLC_INLINE Ref(std::nullptr_t) : Ref(::mlc::Null) {}
  MLC_INLINE Ref(::mlc::NullType) : TBase(::mlc::Null) {}
  MLC_INLINE Ref &operator=(std::nullptr_t) { return (*this = ::mlc::Null); }
  MLC_INLINE TSelf &operator=(::mlc::NullType) { return this->Reset(); }
  MLC_INLINE ~Ref() = default;
  // clang-format off
  MLC_INLINE Ref(const TSelf &src) : TBase(::mlc::Null) { this->_Set(src.ptr); this->IncRef(); }
  MLC_INLINE Ref(TSelf &&src) : TBase(::mlc::Null) { this->_Set(src.ptr); src.ptr = nullptr; }
  MLC_INLINE TSelf &operator=(const TSelf &other) { TSelf(other).Swap(*this); return *this; }
  MLC_INLINE TSelf &operator=(TSelf &&other) { TSelf(std ::move(other)).Swap(*this); return *this; }
  MLC_INLINE TSelf &Reset() { TBase::Reset(); return *this; }
  // clang-format on
  /***** Section 2. The `new` operator *****/
  template <typename... Args, typename = std::enable_if_t<::mlc::base::Allocatable<TObj, Args...>>>
  MLC_INLINE static TSelf New(Args &&...args) {
    return TSelf(::mlc::base::AllocatorOf<TObj>::New(std::forward<Args>(args)...));
  }
  template <size_t N> MLC_INLINE static TSelf New(const ::mlc::base::CharArray<N> &arg) {
    return TSelf(::mlc::base::AllocatorOf<TObj>::template New<N>(arg));
  }
  /***** Section 3. From derived *****/
  // clang-format off
  template <typename U, typename = DObj<U>> MLC_INLINE explicit Ref(const U *src) : TBase() { TBase::_Set(reinterpret_cast<const MLCAny *>(src)); this->IncRef(); }
  template <typename U, typename = DObj<U>> MLC_INLINE Ref(const Ref<U> &src) : TBase() { TBase::_Set(reinterpret_cast<const MLCObjPtr&>(src)); this->IncRef(); }
  template <typename U, typename = DObj<U>> MLC_INLINE Ref(Ref<U> &&src) : TBase() { TBase::_Move(reinterpret_cast<MLCObjPtr&&>(src)); }
  template <typename U, typename = DObjRef<U>> MLC_INLINE Ref(const U &src) : TBase() { TBase::_Set(reinterpret_cast<const MLCObjPtr&>(src)); this->IncRef(); }
  template <typename U, typename = DObjRef<U>> MLC_INLINE Ref(U &&src) : TBase() { TBase::_Move(reinterpret_cast<MLCObjPtr&&>(src)); }
  template <typename U, typename = DOpt<U>> MLC_INLINE Ref(const Optional<U> &src) : TBase() { TBase::_Set(reinterpret_cast<const MLCObjPtr&>(src)); this->IncRef(); }
  template <typename U, typename = DOpt<U>> MLC_INLINE Ref(Optional<U> &&src) : TBase() { TBase::_Move(reinterpret_cast<MLCObjPtr&&>(src)); }
  // Defining "operator="
  template <typename U, typename = DObj<U>> MLC_INLINE TSelf &operator=(const U *other) { TSelf(other).Swap(*this); return *this; }
  template <typename U, typename = DObj<U>> MLC_INLINE TSelf &operator=(const Ref<U> &other) { TSelf(other).Swap(*this); return *this; }
  template <typename U, typename = DObj<U>> MLC_INLINE TSelf &operator=(Ref<U> &&other) { TSelf(std::move(other)).Swap(*this); return *this; }
  template <typename U, typename = DObjRef<U>> MLC_INLINE TSelf &operator=(const U &other) { TSelf(other).Swap(*this); return *this; }
  template <typename U, typename = DObjRef<U>> MLC_INLINE TSelf &operator=(U &&other) { TSelf(std::move(other)).Swap(*this); return *this; }
  template <typename U, typename = DOpt<U>> MLC_INLINE TSelf &operator=(const Optional<U> &other) { TSelf(other).Swap(*this); return *this; }
  template <typename U, typename = DOpt<U>> MLC_INLINE TSelf &operator=(Optional<U> &&other) { TSelf(std::move(other)).Swap(*this); return *this; }
  // clang-format on
  /***** Section 4. Accessor, comparators and stringify *****/
  MLC_INLINE const TObj *get() const { return reinterpret_cast<const TObj *>(TBase::ptr); }
  MLC_INLINE TObj *get() { return reinterpret_cast<TObj *>(TBase::ptr); }
  MLC_INLINE const TObj *operator->() const { return get(); }
  MLC_INLINE TObj *operator->() { return get(); }
  MLC_INLINE const TObj &operator*() const {
    if (const TObj *ret = get()) {
      return *ret;
    }
    MLC_THROW(ValueError) << "Attempt to dereference a null pointer";
    MLC_UNREACHABLE();
  }
  MLC_INLINE TObj &operator*() {
    if (TObj *ret = get()) {
      return *ret;
    }
    MLC_THROW(ValueError) << "Attempt to dereference a null pointer";
    MLC_UNREACHABLE();
  }
  MLC_INLINE bool defined() const { return TBase::ptr != nullptr; }
  MLC_INLINE bool has_value() const { return TBase::ptr != nullptr; }
  MLC_INLINE bool operator==(std::nullptr_t) const { return TBase::ptr == nullptr; }
  MLC_INLINE bool operator!=(std::nullptr_t) const { return TBase::ptr != nullptr; }
  MLC_INLINE bool operator==(const TSelf &rhs) const { return TBase::ptr == rhs.TBase::ptr; }
  MLC_INLINE bool operator!=(const TSelf &rhs) const { return TBase::ptr != rhs.TBase::ptr; }
  Str str() const;
  /***** Section 5. Runtime-type information *****/
  MLC_DEF_RTTI_METHODS(true, this->ptr, this->ptr)
};

#define MLC_DEFINE_POD_REF(T, Field)                                                                                   \
  template <> struct Ref<T> : protected ::mlc::base::PtrBase {                                                         \
  private:                                                                                                             \
    using TBase = ::mlc::base::PtrBase;                                                                                \
                                                                                                                       \
  public:                                                                                                              \
    using TSelf = Ref<T>;                                                                                              \
    using TObj = T;                                                                                                    \
    [[maybe_unused]] static constexpr ::mlc::base::TypeKind _type_kind = ::mlc::base::TypeKind::kRef;                  \
    /***** Section 1. Default constructor/destructors *****/                                                           \
    MLC_INLINE Ref() : TBase() {}                                                                                      \
    MLC_INLINE Ref(std::nullptr_t) : TBase() {}                                                                        \
    MLC_INLINE Ref(::mlc::NullType) : TBase(::mlc::Null) {}                                                            \
    MLC_INLINE TSelf &operator=(std::nullptr_t) { return this->Reset(); }                                              \
    MLC_INLINE TSelf &operator=(::mlc::NullType) { return this->Reset(); }                                             \
    MLC_INLINE ~Ref() = default;                                                                                       \
    /* clang-format off */                                                                                    \
    MLC_INLINE Ref(const TSelf &src) : TBase(::mlc::Null) { this->_Set(src.ptr); this->IncRef(); }            \
    MLC_INLINE Ref(TSelf &&src) : TBase(::mlc::Null) { this->_Set(src.ptr); src.ptr = nullptr; }              \
    MLC_INLINE TSelf &operator=(const TSelf &other) { TSelf(other).Swap(*this); return *this; }               \
    MLC_INLINE TSelf &operator=(TSelf &&other) { TSelf(std::move(other)).Swap(*this); return *this; }         \
    MLC_INLINE TSelf &Reset() { TBase::Reset(); return *this; }        \
    /* clang-format on */                                                                                              \
    /***** Section 2. The `new` operator *****/                                                                        \
    MLC_INLINE_NO_MSVC static TSelf New(T arg) { return TSelf(arg); }                                                  \
    /***** Section 3. From derived pointers *****/                                                                     \
    MLC_INLINE Ref(T src) { TBase::_InitPOD<T>(src); }                                                                 \
    MLC_INLINE Ref &operator=(T other) {                                                                               \
      TSelf(other).Swap(*this);                                                                                        \
      return *this;                                                                                                    \
    }                                                                                                                  \
    MLC_INLINE Ref(const Optional<TObj> &src);                                                                         \
    MLC_INLINE TSelf &operator=(const Optional<TObj> &other);                                                          \
    /***** Section 4. Accessor, comparators and stringify *****/                                                       \
    MLC_INLINE const T *get() const {                                                                                  \
      return TBase::ptr ? &reinterpret_cast<const MLCBoxedPOD *>(TBase::ptr)->data.Field : nullptr;                    \
    }                                                                                                                  \
    MLC_INLINE T *get() { /**/                                                                                         \
      return TBase::ptr ? &reinterpret_cast<MLCBoxedPOD *>(TBase::ptr)->data.Field : nullptr;                          \
    }                                                                                                                  \
    MLC_INLINE const T *operator->() const { return get(); }                                                           \
    MLC_INLINE T *operator->() { return get(); }                                                                       \
    MLC_INLINE const T &operator*() const {                                                                            \
      if (const T *ret = get()) {                                                                                      \
        return *ret;                                                                                                   \
      }                                                                                                                \
      MLC_THROW(ValueError) << "Attempt to dereference a null pointer";                                                \
      MLC_UNREACHABLE();                                                                                               \
    }                                                                                                                  \
    MLC_INLINE T &operator*() {                                                                                        \
      if (T *ret = get()) {                                                                                            \
        return *ret;                                                                                                   \
      }                                                                                                                \
      MLC_THROW(ValueError) << "Attempt to dereference a null pointer";                                                \
      MLC_UNREACHABLE();                                                                                               \
    }                                                                                                                  \
    MLC_INLINE bool defined() const { return TBase::ptr != nullptr; }                                                  \
    MLC_INLINE bool has_value() const { return TBase::ptr != nullptr; }                                                \
    MLC_INLINE T &value() { return operator*(); }                                                                      \
    MLC_INLINE const T &value() const { return operator*(); }                                                          \
    MLC_INLINE bool operator==(std::nullptr_t) const { return TBase::ptr == nullptr; }                                 \
    MLC_INLINE bool operator!=(std::nullptr_t) const { return TBase::ptr != nullptr; }                                 \
    MLC_INLINE bool operator==(const TSelf &rhs) const { return TBase::ptr == rhs.TBase::ptr; }                        \
    MLC_INLINE bool operator!=(const TSelf &rhs) const { return TBase::ptr != rhs.TBase::ptr; }                        \
    Str str() const;                                                                                                   \
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
