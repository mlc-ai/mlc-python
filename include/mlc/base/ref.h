#ifndef MLC_BASE_REF_H_
#define MLC_BASE_REF_H_

#include "./utils.h"
#include <type_traits>

#define MLC_DEF_OBJ_PTR_METHODS_(SelfType, ObjType)                                                                    \
private:                                                                                                               \
  friend struct ::mlc::Any;                                                                                            \
  friend struct ::mlc::AnyView;                                                                                        \
  template <typename> friend struct ::mlc::Ref;                                                                        \
  template <typename> friend struct ::mlc::base::Type2Str;                                                             \
  template <typename> friend struct ::mlc::base::ObjPtrHelper;                                                         \
  using TBase = ::mlc::base::ObjPtrBase;                                                                               \
  using TSelf = SelfType;                                                                                              \
  using TObj = ObjType;                                                                                                \
  using THelper = ::mlc::base::ObjPtrHelper<SelfType>;                                                                 \
                                                                                                                       \
public:                                                                                                                \
  MLC_INLINE const SelfType::TObj *get() const { return reinterpret_cast<const SelfType::TObj *>(this->ptr); }         \
  MLC_INLINE const SelfType::TObj *operator->() const { return get(); }                                                \
  MLC_INLINE const SelfType::TObj &operator*() const { return *get(); }                                                \
  MLC_INLINE SelfType::TObj *get() { return reinterpret_cast<SelfType::TObj *>(ptr); }                                 \
  MLC_INLINE SelfType::TObj *operator->() { return get(); }                                                            \
  MLC_INLINE SelfType::TObj &operator*() { return *get(); }                                                            \
  using ::mlc::base::ObjPtrBase::operator==;                                                                           \
  using ::mlc::base::ObjPtrBase::operator!=;                                                                           \
  using ::mlc::base::ObjPtrBase::defined

namespace mlc {
namespace base {

template <typename TRef> struct ObjPtrHelper {
  using TObj = typename TRef::TObj;
  template <typename ObjPtrType> MLC_INLINE static MLCObject *GetPtr(const ObjPtrType *self) { return self->ptr; }
  template <typename ObjPtrType> MLC_INLINE static MLCObject *MovePtr(ObjPtrType *self) {
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
  MLC_INLINE void IncRef() { ::mlc::base::IncRef(this->ptr); }
  MLC_INLINE void DecRef() { ::mlc::base::DecRef(this->ptr); }
  MLC_INLINE void Swap(ObjPtrBase &other) { std::swap(this->ptr, other.ptr); }
  MLC_INLINE bool defined() const { return this->ptr != nullptr; }
  MLC_INLINE bool operator==(std::nullptr_t) const { return this->ptr == nullptr; }
  MLC_INLINE bool operator!=(std::nullptr_t) const { return this->ptr != nullptr; }
  template <typename T> MLC_INLINE void _Init(const MLCAny &v) {
    T *ret = [&v]() -> T * {
      MLC_TRY_CONVERT(TypeTraits<T *>::AnyToTypeOwned(&v), v.type_index, Type2Str<T *>::Run());
    }();
    this->ptr = reinterpret_cast<MLCObject *>(ret);
    this->IncRef();
  }
  friend std::ostream &operator<<(std::ostream &os, const ObjPtrBase &src);
};

struct ObjectRefDummyRoot : protected ObjPtrBase {
  ObjectRefDummyRoot(NullType) : ObjPtrBase() {}
};
} // namespace base
} // namespace mlc

namespace mlc {
template <typename T> struct Ref : private ::mlc::base::ObjPtrBase {
  MLC_DEF_OBJ_PTR_METHODS_(Ref<T>, T);
  /***** Section 1. Default constructor/destructors *****/
  ~Ref() = default;
  MLC_INLINE Ref() : TBase() {}
  MLC_INLINE Ref(std::nullptr_t) : TBase() {}
  MLC_INLINE Ref(NullType) : TBase() {}
  MLC_INLINE Ref &operator=(std::nullptr_t) { return this->Reset(); }
  MLC_INLINE Ref &operator=(NullType) { return this->Reset(); }
  MLC_INLINE TSelf &Reset() {
    TBase::Reset();
    return *this;
  }
  /***** Section 2. Conversion between `SelfType = Ref<T>` *****/
  MLC_INLINE Ref(const Ref<T> &src) : TBase(src.ptr) { this->IncRef(); }
  MLC_INLINE Ref(Ref<T> &&src) : TBase(src.ptr) { src.ptr = nullptr; }
  MLC_INLINE TSelf &operator=(const Ref<T> &other) {
    TSelf(other).Swap(*this);
    return *this;
  }
  MLC_INLINE TSelf &operator=(Ref<T> &&other) {
    TSelf(std::move(other)).Swap(*this);
    return *this;
  }
  /***** Section 3. Conversion between `Ref<U>` / `U *` where `U` is derived from `T` *****/
  template <typename U, typename = std::enable_if_t<::mlc::base::IsDerivedFrom<U, T>>>
  MLC_INLINE Ref(const Ref<U> &src) : TBase(src.ptr) {
    this->IncRef();
  }
  template <typename U, typename = std::enable_if_t<::mlc::base::IsDerivedFrom<U, T>>>
  MLC_INLINE Ref(Ref<U> &&src) : TBase(src.ptr) {
    src.ptr = nullptr;
  }
  template <typename U, typename = std::enable_if_t<::mlc::base::IsDerivedFrom<U, T>>>
  MLC_INLINE TSelf &operator=(const Ref<U> &other) {
    TSelf(other).Swap(*this);
    return *this;
  }
  template <typename U, typename = std::enable_if_t<::mlc::base::IsDerivedFrom<U, T>>>
  MLC_INLINE TSelf &operator=(Ref<U> &&other) {
    TSelf(std::move(other)).Swap(*this);
    return *this;
  }
  template <typename U, typename = std::enable_if_t<::mlc::base::IsDerivedFrom<U, T>>>
  MLC_INLINE Ref(U *src) : TBase(reinterpret_cast<MLCObject *>(src)) {
    this->IncRef();
  }
  /***** Section 4. The `new` operator *****/
  template <typename... Args, typename = std::enable_if_t<::mlc::base::Newable<T, Args...>>>
  MLC_INLINE static TSelf New(Args &&...args) {
    return TSelf(::mlc::base::AllocatorOf<T>::New(std::forward<Args>(args)...));
  }
  template <size_t N> MLC_INLINE static TSelf New(const ::mlc::base::CharArray<N> &arg) {
    return TSelf(::mlc::base::AllocatorOf<T>::template New<N>(arg));
  }
  /***** Section 5. Conversion between AnyView/Any *****/
  MLC_INLINE Ref(const AnyView &src);
  MLC_INLINE Ref(const Any &src);
  MLC_INLINE operator AnyView() const;
  MLC_INLINE operator Any() const &;
  MLC_INLINE operator Any() &&;
  /***** Misc *****/
  Str str() const;
  MLC_INLINE bool operator==(const TSelf &rhs) const { return this->ptr == rhs.ptr; }
  MLC_INLINE bool operator!=(const TSelf &rhs) const { return this->ptr != rhs.ptr; }
};

template <typename T, typename... Args> inline Ref<Object> InitOf(Args &&...args) {
  T *ptr = ::mlc::base::AllocatorOf<T>::New(std::forward<Args>(args)...);
  return Ref<Object>(reinterpret_cast<Object *>(ptr));
}
} // namespace mlc

#endif // MLC_BASE_REF_H_
