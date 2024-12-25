#ifndef MLC_BASE_OPTIONAL_H_
#define MLC_BASE_OPTIONAL_H_
#include "./ref.h"
#include "./utils.h"
#include <type_traits>

namespace mlc {

template <typename _TObjRef> struct Optional : protected ::mlc::base::PtrBase {
public:
  using TObj = typename _TObjRef::TObj;
  using TObjRef = _TObjRef;
  using TSelf = Optional<_TObjRef>;
  [[maybe_unused]] static constexpr ::mlc::base::TypeKind _type_kind = ::mlc::base::TypeKind::kOptional;

private:
  using TBase = ::mlc::base::PtrBase;
  template <typename U> using DObj = std::enable_if_t<::mlc::base::IsDerivedFrom<U, TObj>>;
  template <typename U> using DObjRef = std::enable_if_t<::mlc::base::IsDerivedFromObjRef<U, TObj>>;
  template <typename U> using DOpt = std::enable_if_t<::mlc::base::IsDerivedFromOpt<U, TObj>>;

public:
  /***** Section 1. Default constructor/destructors *****/
  MLC_INLINE Optional() : TBase() {}
  MLC_INLINE Optional(std::nullptr_t) : Optional(::mlc::Null) {}
  MLC_INLINE Optional(::mlc::NullType) : TBase(::mlc::Null) {}
  MLC_INLINE Optional &operator=(std::nullptr_t) { return (*this = ::mlc::Null); }
  MLC_INLINE TSelf &operator=(::mlc::NullType) { return this->Reset(); }
  MLC_INLINE ~Optional() = default;
  // clang-format off
  MLC_INLINE Optional(const TSelf &src) : TBase(::mlc::Null) { this->_Set(src.ptr); this->IncRef(); }
  MLC_INLINE Optional(TSelf &&src) : TBase(::mlc::Null) { this->_Set(src.ptr); src.ptr = nullptr; }
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
  template <typename U, typename = DObj<U>> MLC_INLINE explicit Optional(const U *src) : TBase() { TBase::_Set(reinterpret_cast<const MLCAny *>(src)); this->IncRef(); }
  template <typename U, typename = DObj<U>> MLC_INLINE Optional(const Ref<U> &src) : TBase() { TBase::_Set(reinterpret_cast<const MLCObjPtr&>(src)); this->IncRef(); }
  template <typename U, typename = DObj<U>> MLC_INLINE Optional(Ref<U> &&src) : TBase() { TBase::_Move(reinterpret_cast<MLCObjPtr&&>(src)); }
  template <typename U, typename = DObjRef<U>> MLC_INLINE Optional(const U &src) : TBase() { TBase::_Set(reinterpret_cast<const MLCObjPtr&>(src)); this->IncRef(); }
  template <typename U, typename = DObjRef<U>> MLC_INLINE Optional(U &&src) : TBase() { TBase::_Move(reinterpret_cast<MLCObjPtr&&>(src)); }
  template <typename U, typename = DOpt<U>> MLC_INLINE Optional(const Optional<U> &src) : TBase() { TBase::_Set(reinterpret_cast<const MLCObjPtr&>(src)); this->IncRef(); }
  template <typename U, typename = DOpt<U>> MLC_INLINE Optional(Optional<U> &&src) : TBase() { TBase::_Move(reinterpret_cast<MLCObjPtr&&>(src)); }
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
  MLC_INLINE TObjRef value() const {
    if (const TObj *ret = get()) {
      return TObjRef(const_cast<TObj *>(ret));
    }
    MLC_THROW(ValueError) << "Attempt to dereference a null pointer";
    MLC_UNREACHABLE();
  }
  // Str str() const;  // TODO: recover this
  /***** Section 5. Runtime-type information *****/
  MLC_DEF_RTTI_METHODS(true, this->ptr, this->ptr)
};

#define MLC_DEFINE_POD_OPT(T, Field)                                                                                   \
  template <> struct Optional<T> : protected ::mlc::base::PtrBase {                                                    \
  private:                                                                                                             \
    using TBase = ::mlc::base::PtrBase;                                                                                \
                                                                                                                       \
  public:                                                                                                              \
    using TSelf = Optional<T>;                                                                                         \
    using TObj = T;                                                                                                    \
    [[maybe_unused]] static constexpr ::mlc::base::TypeKind _type_kind = ::mlc::base::TypeKind::kOptional;             \
    /***** Section 1. Default constructor/destructors *****/                                                           \
    MLC_INLINE Optional() : TBase() {}                                                                                 \
    MLC_INLINE Optional(std::nullptr_t) : TBase() {}                                                                   \
    MLC_INLINE Optional(::mlc::NullType) : TBase(::mlc::Null) {}                                                       \
    MLC_INLINE TSelf &operator=(std::nullptr_t) { return this->Reset(); }                                              \
    MLC_INLINE TSelf &operator=(::mlc::NullType) { return this->Reset(); }                                             \
    MLC_INLINE ~Optional() = default;                                                                                  \
    /* clang-format off */\
    MLC_INLINE Optional(const TSelf &src) : TBase(::mlc::Null) { this->_Set(src.ptr); this->IncRef(); }\
    MLC_INLINE Optional(TSelf &&src) : TBase(::mlc::Null) { this->_Set(src.ptr); src.ptr = nullptr; }\
    MLC_INLINE TSelf &operator=(const TSelf &other) { TSelf(other).Swap(*this); return *this; }\
    MLC_INLINE TSelf &operator=(TSelf &&other) { TSelf(std::move(other)).Swap(*this); return *this; }\
    MLC_INLINE TSelf &Reset() { TBase::Reset(); return *this; }                                                                                            \
    /* clang-format on */                                                                                              \
    /***** Section 2. The `new` operator *****/                                                                        \
    MLC_INLINE_NO_MSVC static TSelf New(T arg) { return TSelf(arg); }                                                  \
    /***** Section 3. From derived pointers *****/                                                                     \
    MLC_INLINE Optional(T src) { TBase::_InitPOD<T>(src); }                                                            \
    MLC_INLINE Optional &operator=(T other) {                                                                          \
      TSelf(other).Swap(*this);                                                                                        \
      return *this;                                                                                                    \
    }                                                                                                                  \
    MLC_INLINE Optional(const Ref<TObj> &src);                                                                         \
    MLC_INLINE TSelf &operator=(const Ref<TObj> &other);                                                               \
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
  };                                                                                                                   \
  MLC_INLINE Ref<T>::Ref(const Optional<T> &src) : Ref() {                                                             \
    if (const T *v = src.get())                                                                                        \
      this->_InitPOD<T>(*v);                                                                                           \
  }                                                                                                                    \
  MLC_INLINE Ref<T> &Ref<T>::operator=(const Optional<T> &other) {                                                     \
    Ref<T>(other).Swap(*this);                                                                                         \
    return *this;                                                                                                      \
  }                                                                                                                    \
  MLC_INLINE Optional<T>::Optional(const Ref<T> &src) {                                                                \
    if (const T *v = src.get())                                                                                        \
      this->_InitPOD<T>(*v);                                                                                           \
  }                                                                                                                    \
  MLC_INLINE Optional<T> &Optional<T>::operator=(const Ref<T> &other) {                                                \
    Optional<T>(other).Swap(*this);                                                                                    \
    return *this;                                                                                                      \
  }

MLC_DEFINE_POD_OPT(int64_t, v_int64)
MLC_DEFINE_POD_OPT(double, v_float64)
MLC_DEFINE_POD_OPT(DLDevice, v_device)
MLC_DEFINE_POD_OPT(DLDataType, v_dtype)
MLC_DEFINE_POD_OPT(::mlc::base::VoidPtr, v_ptr)
#undef MLC_DEFINE_POD_OPT

} // namespace mlc

#endif // MLC_BASE_OPTIONAL_H_
