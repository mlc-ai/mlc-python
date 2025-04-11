#ifndef MLC_CORE_OBJECT_H_
#define MLC_CORE_OBJECT_H_

#include "./reflection.h"
#include <iomanip>

/******************* Section 0. Dummy root *******************/

namespace mlc {
namespace core {
struct ObjectDummyRoot {
  static constexpr int32_t _type_depth = -1;
  static constexpr int32_t _type_index = -1;
  using _type_ancestor_types = std::tuple<ObjectDummyRoot>;
};
struct ObjectRefDummyRoot : protected ::mlc::base::PtrBase {
  ObjectRefDummyRoot() : PtrBase() {}
  ObjectRefDummyRoot(NullType) : PtrBase() {}
};
} // namespace core
} // namespace mlc

/******************* Section 1. Object *******************/

#define MLC_DEF_TYPE_COMMON_(IS_EXPORT, SelfType, ParentType, TypeIndex, TypeKey)                                      \
public:                                                                                                                \
  template <typename> friend struct ::mlc::DefaultObjectAllocator;                                                     \
  template <typename> friend struct ::mlc::base::ObjPtrTraitsDefault;                                                  \
  template <typename, typename> friend struct ::mlc::base::TypeTraits;                                                 \
  using TObj = SelfType;                                                                                               \
  MLC_DEF_RTTI_METHODS(false, reinterpret_cast<MLCAny *>(this), reinterpret_cast<const MLCAny *>(this))                \
  [[maybe_unused]] static constexpr const char *_type_key = TypeKey;                                                   \
  [[maybe_unused]] static inline MLCTypeInfo *_type_info =                                                             \
      (IS_EXPORT) ? ::mlc::Lib::TypeRegister(static_cast<int32_t>(ParentType::_type_index),                            \
                                             static_cast<int32_t>(TypeIndex), TypeKey)                                 \
                  : ::mlc::Lib::GetTypeInfo(TypeKey);                                                                  \
  [[maybe_unused]] static inline int32_t *_type_ancestors = _type_info->type_ancestors;                                \
  [[maybe_unused]] static constexpr int32_t _type_depth = ParentType::_type_depth + 1;                                 \
  [[maybe_unused]] static constexpr ::mlc::base::TypeKind _type_kind = ::mlc::base::TypeKind::kObj;                    \
  using _type_parent [[maybe_unused]] = ParentType;                                                                    \
  using _type_ancestor_types [[maybe_unused]] =                                                                        \
      ::mlc::base::TupleAppend<typename ParentType::_type_ancestor_types, SelfType>

#define MLC_DEF_STATIC_TYPE(IS_EXPORT, SelfType, ParentType, TypeIndex, TypeKey)                                       \
public:                                                                                                                \
  MLC_DEF_TYPE_COMMON_(IS_EXPORT, SelfType, ParentType, TypeIndex, TypeKey);                                           \
  [[maybe_unused]] static constexpr int32_t _type_index = static_cast<int32_t>(TypeIndex)

#define MLC_DEF_DYN_TYPE(IS_EXPORT, SelfType, ParentType, TypeKey)                                                     \
public:                                                                                                                \
  MLC_DEF_TYPE_COMMON_(IS_EXPORT, SelfType, ParentType, -1, TypeKey);                                                  \
  [[maybe_unused]] static inline int32_t _type_index = _type_info->type_index

namespace mlc {
struct Object {
  MLCAny _mlc_header;

  MLC_INLINE Object() : _mlc_header() {}
  MLC_INLINE Object(const Object &) : _mlc_header() {}
  MLC_INLINE Object(Object &&) {}
  MLC_INLINE Object &operator=(const Object &) { return *this; }
  MLC_INLINE Object &operator=(Object &&) { return *this; }
  Str str() const;
  std::string __str__() const {
    std::ostringstream os;
    os << this->GetTypeKey() << "@0x" << std::setfill('0') << std::setw(12) << std::hex
       << (uintptr_t)(this->_mlc_header.v.v_ptr);
    return os.str();
  }
  friend std::ostream &operator<<(std::ostream &os, const Object &src);

  MLC_DEF_STATIC_TYPE(MLC_EXPORTS, Object, ::mlc::core::ObjectDummyRoot, MLCTypeIndex::kMLCObject, "object.Object");
};
} // namespace mlc

/******************* Section 2. Object Ref *******************/

namespace mlc {
#define MLC_DEF_OBJ_REF_(IS_EXPORT, SelfType, ObjType, ParentType)                                                     \
private:                                                                                                               \
  using TBase = ::mlc::base::PtrBase;                                                                                  \
  template <typename U> using DObj = std::enable_if_t<::mlc::base::IsDerivedFrom<U, TObj>>;                            \
  template <typename U> using DObjRef = std::enable_if_t<::mlc::base::IsDerivedFromObjRef<U, TObj>>;                   \
  template <typename U> using DOpt = std::enable_if_t<::mlc::base::IsDerivedFromOpt<U, TObj>>;                         \
                                                                                                                       \
public:                                                                                                                \
  /***** Section 1. Default constructor/destructors *****/                                                             \
  MLC_INLINE SelfType(std::nullptr_t) = delete;                                                                        \
  MLC_INLINE SelfType(::mlc::NullType) : ParentType(::mlc::Null) {}                                                    \
  MLC_INLINE SelfType &operator=(std::nullptr_t) = delete;                                                             \
  MLC_INLINE SelfType &operator=(::mlc::NullType) { return this->Reset(); }                                            \
  MLC_INLINE ~SelfType() = default;                                                                                    \
  /* clang-format off */                                                                                    \
  MLC_INLINE SelfType(const TSelf &src) : ParentType(::mlc::Null) { this->_Set(src.ptr); TBase::IncRef(); } \
  MLC_INLINE SelfType(TSelf &&src) : ParentType(::mlc::Null) { this->_Set(src.ptr); src.ptr = nullptr; }    \
  MLC_INLINE TSelf &operator=(const TSelf &other) { TSelf(other).Swap(*this); return *this; }               \
  MLC_INLINE TSelf &operator=(TSelf &&other) { TSelf(std::move(other)).Swap(*this); return *this; }         \
  MLC_INLINE TSelf &Reset() { TBase::Reset(); return *this; }          \
  /* clang-format on */                                                                                                \
  /***** Section 3. From derived pointers *****/                                                                       \
  /* clang-format off */                                                                                                                                                                                                          \
  template <typename U, typename = DObj<U>> MLC_INLINE explicit SelfType(const U *src) : ParentType(::mlc::Null) { this->_Set(reinterpret_cast<const MLCAny *>(src)); TBase::IncRef(); TBase::CheckNull<TSelf>(); }               \
  template <typename U, typename = DObj<U>> MLC_INLINE SelfType(const ::mlc::Ref<U> &src) : ParentType(::mlc::Null) { TBase::_Set(reinterpret_cast<const MLCObjPtr &>(src)); TBase::IncRef(); TBase::CheckNull<TSelf>(); }        \
  template <typename U, typename = DObj<U>> MLC_INLINE SelfType(::mlc::Ref<U> &&src) : ParentType(::mlc::Null) { TBase::_Move(reinterpret_cast<MLCObjPtr &&>(src)); TBase::CheckNull<TSelf>(); }                                  \
  template <typename U, typename = DObj<U>> MLC_INLINE SelfType(const ::mlc::Optional<U> &src) : ParentType(::mlc::Null) { TBase::_Set(reinterpret_cast<const MLCObjPtr &>(src)); TBase::IncRef(); TBase::CheckNull<TSelf>(); }   \
  template <typename U, typename = DObj<U>> MLC_INLINE SelfType(::mlc::Optional<U> &&src) : ParentType(::mlc::Null) { TBase::_Move(reinterpret_cast<MLCObjPtr &&>(src)); TBase::CheckNull<TSelf>(); }                             \
  /* Defining "operator=" */                                                                                                                                                                                                      \
  template <typename U, typename = DObj<U>> MLC_INLINE TSelf &operator=(const U *other) { TSelf(other).Swap(*this); return *this; }                                                                                               \
  template <typename U, typename = DObj<U>> MLC_INLINE TSelf &operator=(const ::mlc::Ref<U> &other) { TSelf(other).Swap(*this); return *this; }                                                                                   \
  template <typename U, typename = DObj<U>> MLC_INLINE TSelf &operator=(::mlc::Ref<U> &&other) { TSelf(std::move(other)).Swap(*this); return *this; }                                                                             \
  template <typename U, typename = DObj<U>> MLC_INLINE TSelf &operator=(const ::mlc::Optional<U> &other) { TSelf(other).Swap(*this); return *this; }                                                                              \
  template <typename U, typename = DObj<U>> MLC_INLINE TSelf &operator=(::mlc::Optional<U> &&other) { TSelf(std::move(other)).Swap(*this); return *this; }                                                                                 \
  /* clang-format on */                                                                                                \
  /***** Section 4. Conversion between AnyView/Any *****/                                                              \
  MLC_INLINE SelfType(const ::mlc::AnyView *src) : ParentType(::mlc::Null) {                                           \
    TBase::_Init<TObj>(src);                                                                                           \
    TBase::CheckNull<TSelf>();                                                                                         \
  }                                                                                                                    \
  MLC_INLINE SelfType(const ::mlc::Any *src) : ParentType(::mlc::Null) {                                               \
    TBase::_Init<TObj>(src);                                                                                           \
    TBase::CheckNull<TSelf>();                                                                                         \
  }                                                                                                                    \
  MLC_INLINE operator ::mlc::AnyView() const { return ::mlc::AnyView(this->get()); };                                  \
  MLC_INLINE operator ::mlc::Any() const { return ::mlc::Any(this->get()); }                                           \
  /***** Section 5. Accessor, comparators and stringify *****/                                                         \
  MLC_INLINE const TObj *get() const { return reinterpret_cast<const TObj *>(this->ptr); }                             \
  MLC_INLINE TObj *get() { return reinterpret_cast<TObj *>(ptr); }                                                     \
  MLC_INLINE const TObj *operator->() const { return get(); }                                                          \
  MLC_INLINE TObj *operator->() { return get(); }                                                                      \
  MLC_INLINE const TObj &operator*() const {                                                                           \
    if (const TObj *ret = get()) {                                                                                     \
      return *ret;                                                                                                     \
    }                                                                                                                  \
    MLC_THROW(ValueError) << "Attempt to dereference a null pointer";                                                  \
    MLC_UNREACHABLE();                                                                                                 \
  }                                                                                                                    \
  MLC_INLINE TObj &operator*() {                                                                                       \
    if (TObj *ret = get()) {                                                                                           \
      return *ret;                                                                                                     \
    }                                                                                                                  \
    MLC_THROW(ValueError) << "Attempt to dereference a null pointer";                                                  \
    MLC_UNREACHABLE();                                                                                                 \
  }                                                                                                                    \
  MLC_INLINE bool defined() const { return TBase::ptr != nullptr; }                                                    \
  MLC_INLINE bool has_value() const { return TBase::ptr != nullptr; }                                                  \
  MLC_INLINE bool operator==(std::nullptr_t) const { return TBase::ptr == nullptr; }                                   \
  MLC_INLINE bool operator!=(std::nullptr_t) const { return TBase::ptr != nullptr; }                                   \
  MLC_INLINE bool same_as(const ObjectRef &other) const {                                                              \
    return static_cast<const void *>(this->get()) == static_cast<const void *>(other.get());                           \
  }                                                                                                                    \
  /***** Section 6. Runtime-type information *****/                                                                    \
  MLC_DEF_RTTI_METHODS(true, TBase::ptr, TBase::ptr)                                                                   \
  static inline const int32_t _type_reflect =                                                                          \
      ::mlc::core::Reflect<IS_EXPORT>(static_cast<int32_t>(TObj::_type_index)).Init<TObj>()

#define MLC_DEF_OBJ_REF(IS_EXPORT, SelfType, ObjType, ParentType)                                                      \
private:                                                                                                               \
  template <typename... Args, typename = std::enable_if_t<std::is_constructible_v<ObjType, Args...>>>                  \
  MLC_INLINE static ObjType *New(Args &&...args) {                                                                     \
    return ::mlc::base::AllocatorOf<ObjType>::New(std::forward<Args>(args)...);                                        \
  }                                                                                                                    \
                                                                                                                       \
public:                                                                                                                \
  [[maybe_unused]] static constexpr ::mlc::base::TypeKind _type_kind = ::mlc::base::TypeKind::kObjRef;                 \
  using TSelf = SelfType;                                                                                              \
  using TObj = ObjType;                                                                                                \
  using TObjRef = SelfType;                                                                                            \
  MLC_DEF_OBJ_REF_(IS_EXPORT, SelfType, ObjType, ParentType)

#define MLC_DEF_OBJ_REF_COW_()                                                                                         \
  inline TObj *CopyOnWrite() {                                                                                         \
    ::mlc::base::PtrBase *obj_ref = this;                                                                              \
    if (::mlc::base::RefCount(obj_ref->ptr) > 1) {                                                                     \
      Ref<TObj>::New(*reinterpret_cast<TObj *>(obj_ref->ptr)).Swap(*obj_ref);                                          \
    }                                                                                                                  \
    return reinterpret_cast<TObj *>(obj_ref->ptr);                                                                     \
  }

#define MLC_DEF_OBJ_REF_FWD_NEW(SelfType)                                                                              \
  template <typename... Args, typename = std::enable_if_t<std::is_constructible_v<SelfType::TObj, Args...>>>           \
  MLC_INLINE explicit SelfType(Args &&...args) : SelfType(SelfType::New(std::forward<Args>(args)...)) {}

struct ObjectRef : protected ::mlc::core::ObjectRefDummyRoot {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, ObjectRef, Object, ::mlc::core::ObjectRefDummyRoot)
      .StaticFn("__init__", InitOf<Object>)
      .MemFn("__str__", &Object::__str__);
  explicit ObjectRef() : ObjectRef(ObjectRef::New()) {}
  friend std::ostream &operator<<(std::ostream &os, const TSelf &src);
};
} // namespace mlc

namespace mlc {
struct Exception : public std::exception {
  Exception(Ref<ErrorObj> data);
  Exception(const Exception &other) : data_(other.data_) {}
  Exception(Exception &&other) : data_(std::move(other.data_)) {}
  Exception &operator=(const Exception &other) {
    this->data_ = other.data_;
    return *this;
  }
  Exception &operator=(Exception &&other) {
    this->data_ = std::move(other.data_);
    return *this;
  }
  const ErrorObj *Obj() const { return reinterpret_cast<const ErrorObj *>(data_.get()); }
  const char *what() const noexcept(true) override;
  void FormatExc(std::ostream &os) const;
  Ref<Object> data_;
};
struct ObjRefHash {
  std::size_t operator()(const ObjectRef &obj) const { return std::hash<const void *>{}(obj.get()); }
};
struct ObjRefEqual {
  bool operator()(const ObjectRef &a, const ObjectRef &b) const { return a.get() == b.get(); }
};
struct StructuralHash {
  std::size_t operator()(const ObjectRef &obj) const { return ::mlc::Lib::StructuralHash(obj); }
};
template <bool bind_free_vars> struct StructuralEqual {
  bool operator()(const ObjectRef &a, const ObjectRef &b) const {
    return ::mlc::Lib::StructuralEqual(a, b, bind_free_vars, false);
  }
};
} // namespace mlc

#endif // MLC_CORE_OBJECT_H_
