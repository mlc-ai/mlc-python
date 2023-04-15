#ifndef MLC_CORE_OBJECT_H_
#define MLC_CORE_OBJECT_H_

#include "./utils.h"

namespace mlc {

#define MLC_DEF_TYPE_COMMON_(SelfType, ParentType, TypeIndex, TypeKey)                                                 \
public:                                                                                                                \
  template <typename> friend struct ::mlc::DefaultObjectAllocator;                                                     \
  template <typename> friend struct ::mlc::Ref;                                                                        \
  template <typename> friend struct ::mlc::base::ObjPtrTraitsDefault;                                                  \
  friend struct ::mlc::Any;                                                                                            \
  friend struct ::mlc::AnyView;                                                                                        \
  template <typename DerivedType> MLC_INLINE bool IsInstance() const {                                                 \
    return ::mlc::base::IsInstanceOf<DerivedType, SelfType>(reinterpret_cast<const MLCAny *>(this));                   \
  }                                                                                                                    \
  MLC_INLINE const char *GetTypeKey() const {                                                                          \
    return ::mlc::base::TypeIndex2TypeKey(reinterpret_cast<const MLCAny *>(this));                                     \
  }                                                                                                                    \
  MLC_INLINE int32_t GetTypeIndex() const { return reinterpret_cast<const MLCAny *>(this)->type_index; }               \
  [[maybe_unused]] static constexpr const char *_type_key = TypeKey;                                                   \
  [[maybe_unused]] static inline MLCTypeInfo *_type_info =                                                             \
      ::mlc::core::TypeRegister<SelfType>(static_cast<int32_t>(ParentType::_type_index), /**/                          \
                                          static_cast<int32_t>(TypeIndex), TypeKey);                                   \
  [[maybe_unused]] static inline int32_t *_type_ancestors = _type_info->type_ancestors;                                \
  [[maybe_unused]] static constexpr int32_t _type_depth = ParentType::_type_depth + 1;                                 \
  using _type_parent [[maybe_unused]] = ParentType;                                                                    \
  static_assert(sizeof(::mlc::Ref<SelfType>) == sizeof(MLCObjPtr), "Size mismatch")

#define MLC_DEF_STATIC_TYPE(SelfType, ParentType, TypeIndex, TypeKey)                                                  \
public:                                                                                                                \
  MLC_DEF_TYPE_COMMON_(SelfType, ParentType, TypeIndex, TypeKey);                                                      \
  [[maybe_unused]] static constexpr int32_t _type_index = static_cast<int32_t>(TypeIndex);                             \
  static inline const int32_t _type_reflect =                                                                          \
      ::mlc::core::ReflectionHelper(static_cast<int32_t>(SelfType::_type_index)).Init<SelfType>()

#define MLC_DEF_DYN_TYPE(SelfType, ParentType, TypeKey)                                                                \
public:                                                                                                                \
  MLC_DEF_TYPE_COMMON_(SelfType, ParentType, -1, TypeKey);                                                             \
  [[maybe_unused]] static inline int32_t _type_index = _type_info->type_index;                                         \
  static inline const int32_t _type_reflect =                                                                          \
      ::mlc::core::ReflectionHelper(static_cast<int32_t>(SelfType::_type_index)).Init<SelfType>()

#define MLC_DEF_OBJ_REF(SelfType, ObjType, ParentType)                                                                 \
public:                                                                                                                \
  MLC_INLINE const ObjType *get() const { return reinterpret_cast<const ObjType *>(this->ptr); }                       \
  MLC_INLINE ObjType *get() { return reinterpret_cast<ObjType *>(ptr); }                                               \
  MLC_DEF_OBJ_PTR_METHODS_(SelfType, ObjType, ParentType);                                                             \
                                                                                                                       \
private:                                                                                                               \
  MLC_INLINE void CheckNull() const {                                                                                  \
    if (this->ptr == nullptr) {                                                                                        \
      MLC_THROW(TypeError) << "Cannot convert from type `None` to non-nullable `"                                      \
                           << ::mlc::base::Type2Str<SelfType>::Run() << "`";                                           \
    }                                                                                                                  \
  }                                                                                                                    \
                                                                                                                       \
public:                                                                                                                \
  /***** Section 1. Default constructor/destructors *****/                                                             \
  SelfType(std::nullptr_t) = delete;                                                                                   \
  SelfType &operator=(std::nullptr_t) = delete;                                                                        \
  /***** Section 2. Conversion between `Ref<U>` / `U *` where `U` is derived from `ObjType` *****/                     \
  template <typename U, typename = Derived<U>>                                                                         \
  MLC_INLINE SelfType(const ::mlc::Ref<U> &src) : ParentType(::mlc::Null) {                                            \
    this->_SetObjPtr(::mlc::base::ObjPtrHelper<::mlc::Ref<U>>::GetPtr(&src));                                          \
    this->CheckNull();                                                                                                 \
    this->IncRef();                                                                                                    \
  }                                                                                                                    \
  template <typename U, typename = Derived<U>> /**/                                                                    \
  MLC_INLINE SelfType(::mlc::Ref<U> &&src) : ParentType(::mlc::Null) {                                                 \
    this->_SetObjPtr(::mlc::base::ObjPtrHelper<::mlc::Ref<U>>::MovePtr(&src));                                         \
    this->CheckNull();                                                                                                 \
  }                                                                                                                    \
  template <typename U, typename = Derived<U>> /**/                                                                    \
  MLC_INLINE TSelf &operator=(const ::mlc::Ref<U> &other) {                                                            \
    TSelf(other).Swap(*this);                                                                                          \
    return *this;                                                                                                      \
  }                                                                                                                    \
  template <typename U, typename = Derived<U>> /**/                                                                    \
  MLC_INLINE TSelf &operator=(::mlc::Ref<U> &&other) {                                                                 \
    TSelf(std::move(other)).Swap(*this);                                                                               \
    return *this;                                                                                                      \
  }                                                                                                                    \
  template <typename U, typename = Derived<U>> /**/                                                                    \
  MLC_INLINE explicit SelfType(U *src) : ParentType(::mlc::Null) {                                                     \
    this->_SetObjPtr(reinterpret_cast<MLCAny *>(src));                                                                 \
    this->CheckNull();                                                                                                 \
    this->IncRef();                                                                                                    \
  }                                                                                                                    \
  /***** Section 3. The `new` operator *****/                                                                          \
  template <typename... Args, typename = std::enable_if_t<::mlc::base::Newable<ObjType, Args...>>>                     \
  MLC_INLINE SelfType(Args &&...args)                                                                                  \
      : SelfType(::mlc::base::AllocatorOf<ObjType>::New(std::forward<Args>(args)...)) {}                               \
  /***** Section 4. Conversion between AnyView/Any *****/                                                              \
  MLC_INLINE SelfType(const ::mlc::AnyView &src) : ParentType(::mlc::Null) {                                           \
    TBase::_Init<ObjType>(src);                                                                                        \
    this->CheckNull();                                                                                                 \
  }                                                                                                                    \
  MLC_INLINE SelfType(const ::mlc::Any &src) : ParentType(::mlc::Null) {                                               \
    TBase::_Init<ObjType>(src);                                                                                        \
    this->CheckNull();                                                                                                 \
  }                                                                                                                    \
  MLC_INLINE operator ::mlc::AnyView() const { return ::mlc::AnyView(this->get()); };                                  \
  MLC_INLINE operator ::mlc::Any() const { return ::mlc::Any(this->get()); }                                           \
  static inline const int32_t _type_reflect =                                                                          \
      ::mlc::core::ReflectionHelper(static_cast<int32_t>(ObjType::_type_index)).Init<ObjType>()

struct Object {
  MLCAny _mlc_header;

  MLC_INLINE Object() : _mlc_header() {}
  MLC_INLINE Object(const Object &) : _mlc_header() {}
  MLC_INLINE Object(Object &&) {}
  MLC_INLINE Object &operator=(const Object &) { return *this; }
  MLC_INLINE Object &operator=(Object &&) { return *this; }
  Str str() const;
  friend std::ostream &operator<<(std::ostream &os, const Object &src);

  MLC_DEF_STATIC_TYPE(Object, ::mlc::base::ObjectDummyRoot, MLCTypeIndex::kMLCObject, "object.Object");
};

struct ObjectRef : protected ::mlc::base::ObjectRefDummyRoot {
  MLC_DEF_OBJ_REF(ObjectRef, Object, ::mlc::base::ObjectRefDummyRoot) //
      .StaticFn("__init__", InitOf<Object>);
  friend std::ostream &operator<<(std::ostream &os, const TSelf &src);
};

} // namespace mlc

#endif // MLC_CORE_OBJECT_H_
