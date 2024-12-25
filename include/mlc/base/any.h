#ifndef MLC_BASE_ANY_H_
#define MLC_BASE_ANY_H_
#include "./base_traits.h"
#include "./utils.h"
#include <cstring>
#include <type_traits>
#include <utility>

namespace mlc {

struct AnyView : public MLCAny {
  static constexpr ::mlc::base::TypeKind _type_kind = ::mlc::base::TypeKind::kAny;
  template <typename TObj> using DObj = std::enable_if_t<::mlc::base::IsObj<TObj>>;
  template <typename T> using DPODOrObjPtrOrObjRef = std::enable_if_t<::mlc::base::IsPODOrObjPtrOrObjRef<T>>;
  /***** Section 1. Default constructor/destructors *****/
  MLC_INLINE AnyView() : MLCAny() {}
  MLC_INLINE AnyView(std::nullptr_t) : MLCAny() {}
  MLC_INLINE AnyView(NullType) : MLCAny() {}
  MLC_INLINE AnyView(const AnyView &src) = default;
  MLC_INLINE AnyView(AnyView &&src) = default;
  /* Defining "operator=" */
  // clang-format off
  MLC_INLINE AnyView &operator=(const AnyView &src) = default;
  MLC_INLINE AnyView &operator=(AnyView &&src) = default;
  MLC_INLINE AnyView &operator=(std::nullptr_t) { this->Reset(); return *this; }
  MLC_INLINE AnyView &operator=(NullType) { this->Reset(); return *this; }
  MLC_INLINE ~AnyView() = default;
  MLC_INLINE AnyView &Reset() { *(static_cast<MLCAny *>(this)) = MLCAny(); return *this; }
  // clang-format on
  /***** Section 2. Conversion from Any, POD, ObjRef, ObjPtr, `Ref<T>`, `Optional<T>` *****/
  MLC_INLINE AnyView(const Any &src);
  MLC_INLINE AnyView(Any &&src);
  template <typename T, typename = DPODOrObjPtrOrObjRef<T>> AnyView(T &&src);
  template <typename T, typename = DObj<T>> MLC_INLINE AnyView(const T *src);
  template <typename T> MLC_INLINE AnyView(const Ref<T> &src);
  template <typename T> MLC_INLINE AnyView(Ref<T> &&src);
  template <typename T> MLC_INLINE AnyView(const Optional<T> &src);
  template <typename T> MLC_INLINE AnyView(Optional<T> &&src);
  /* Defining "operator=" */
  MLC_INLINE AnyView &operator=(const Any &src);
  MLC_INLINE AnyView &operator=(Any &&src);
  template <typename T, typename = DPODOrObjPtrOrObjRef<T>> MLC_INLINE AnyView &operator=(T &&src);
  template <typename T, typename = DObj<T>> MLC_INLINE AnyView &operator=(const T *src);
  template <typename T> MLC_INLINE AnyView &operator=(const Ref<T> &src);
  template <typename T> MLC_INLINE AnyView &operator=(Ref<T> &&src);
  template <typename T> MLC_INLINE AnyView &operator=(const Optional<T> &src);
  template <typename T> MLC_INLINE AnyView &operator=(Optional<T> &&src);
  /***** Section 3. Conversion to `TPOD` `TObj *`, `Ref<T>`, `Optional<T>` *****/
  template <typename T, typename = DPODOrObjPtrOrObjRef<T>> operator T() const;
  template <typename T> operator Ref<T>() const;
  template <typename T> operator Optional<T>() const;
  /***** Section 4. accessors, comparators and stringify *****/
  MLC_INLINE bool defined() const { return !::mlc::base::IsTypeIndexNone(this->type_index); }
  MLC_INLINE bool has_value() const { return !::mlc::base::IsTypeIndexNone(this->type_index); }
  MLC_INLINE bool operator==(std::nullptr_t) const { return !this->defined(); }
  MLC_INLINE bool operator!=(std::nullptr_t) const { return this->defined(); }
  Str str() const;
  friend std::ostream &operator<<(std::ostream &os, const AnyView &src);
  /***** Section 5. Runtime-type information *****/
  template <typename DerivedObj> MLC_INLINE bool IsInstance() const {
    static_assert(::mlc::base::IsObj<DerivedObj>, "DerivedObj must be an object type");
    if (::mlc::base::IsTypeIndexPOD(this->type_index)) {
      return false;
    }
    return ::mlc::base::IsInstanceOf<DerivedObj>(this->v.v_obj);
  }
  template <typename DerivedObj> MLC_INLINE DerivedObj *TryCast() {
    return this->IsInstance<DerivedObj>() ? reinterpret_cast<DerivedObj *>(this->v.v_obj) : nullptr;
  }
  template <typename DerivedObj> MLC_INLINE const DerivedObj *TryCast() const {
    return this->IsInstance<DerivedObj>() ? reinterpret_cast<const DerivedObj *>(this->v.v_obj) : nullptr;
  }
  template <typename DerivedObj> inline DerivedObj *Cast() {
    if (!this->IsInstance<DerivedObj>()) {
      MLC_THROW(TypeError) << "Cannot cast from type `" << ::mlc::base::TypeIndex2TypeKey(this->type_index)
                           << "` to type `" << ::mlc::base::Type2Str<DerivedObj>::Run() << "`";
    }
    return reinterpret_cast<DerivedObj *>(this->v.v_obj);
  }
  template <typename DerivedObj> MLC_INLINE const DerivedObj *Cast() const {
    if (!this->IsInstance<DerivedObj>()) {
      MLC_THROW(TypeError) << "Cannot cast from type `" << ::mlc::base::TypeIndex2TypeKey(this->type_index)
                           << "` to type `" << ::mlc::base::Type2Str<DerivedObj>::Run() << "`";
    }
    return reinterpret_cast<const DerivedObj *>(this->v.v_obj);
  }
  int32_t GetTypeIndex() const { return this->type_index; }
  const char *GetTypeKey() const { return ::mlc::base::TypeIndex2TypeInfo(this->type_index)->type_key; }

  template <typename T, typename = std::enable_if_t<::mlc::base::Anyable<::mlc::base::RemoveCR<T>>>>
  MLC_INLINE_NO_MSVC T _CastWithStorage(Any *storage) const; // TODO: reemove this

protected:
  MLC_INLINE void Swap(MLCAny &src) {
    MLCAny tmp = *this;
    *static_cast<MLCAny *>(this) = src;
    src = tmp;
  }
};

struct Any : public MLCAny {
  static constexpr ::mlc::base::TypeKind _type_kind = ::mlc::base::TypeKind::kAny;
  template <typename TObj> using DObj = std::enable_if_t<::mlc::base::IsObj<TObj>>;
  template <typename T> using DPODOrObjPtrOrObjRef = std::enable_if_t<::mlc::base::IsPODOrObjPtrOrObjRef<T>>;
  /***** Section 1. Default constructor/destructors *****/
  MLC_INLINE Any() : MLCAny() {}
  MLC_INLINE Any(std::nullptr_t) : MLCAny() {}
  MLC_INLINE Any(NullType) : MLCAny() {}
  MLC_INLINE Any(const Any &src) : MLCAny(static_cast<const MLCAny &>(src)) { this->IncRef(); }
  MLC_INLINE Any(Any &&src) : MLCAny(static_cast<const MLCAny &>(src)) { *static_cast<MLCAny *>(&src) = MLCAny(); }
  /* Defining "operator=" */
  // clang-format off
  MLC_INLINE Any &operator=(const Any &src) { Any(src).Swap(*this); return *this; }
  MLC_INLINE Any &operator=(Any &&src) { Any(std::move(src)).Swap(*this); return *this; }
  MLC_INLINE Any &operator=(std::nullptr_t) { this->Reset(); return *this; }
  MLC_INLINE Any &operator=(NullType) { this->Reset(); return *this; }
  MLC_INLINE ~Any() { this->Reset(); }
  MLC_INLINE Any &Reset() { this->DecRef(); *(static_cast<MLCAny *>(this)) = MLCAny(); return *this; }
  // clang-format on
  /***** Section 2. Conversion from AnyView, POD, ObjRef, ObjPtr, `Ref<T>`, `Optional<T>` *****/
  // clang-format off
  MLC_INLINE Any(const AnyView &src) : MLCAny(static_cast<const MLCAny &>(src)) { this->SwitchFromRawStr(); this->IncRef(); }
  MLC_INLINE Any(AnyView &&src) : MLCAny(static_cast<const MLCAny &>(src)) { *static_cast<MLCAny *>(&src) = MLCAny(); this->SwitchFromRawStr(); this->IncRef(); }
  // clang-format on
  template <typename T, typename = DPODOrObjPtrOrObjRef<T>> Any(T &&src) : Any(AnyView(std::forward<T>(src))) {}
  template <typename T, typename = DObj<T>> MLC_INLINE Any(const T *src) : Any(AnyView(src)) {}
  template <typename T> MLC_INLINE Any(const Ref<T> &src) : Any(AnyView(src)) {}
  template <typename T> MLC_INLINE Any(Ref<T> &&src) : Any(AnyView(std::move(src))) {}
  template <typename T> MLC_INLINE Any(const Optional<T> &src) : Any(AnyView(src)) {}
  template <typename T> MLC_INLINE Any(Optional<T> &&src) : Any(AnyView(std::move(src))) {}
  /* Defining "operator=" */
  // clang-format off
  MLC_INLINE Any &operator=(const AnyView &src) { Any(src).Swap(*this); return *this; }
  MLC_INLINE Any &operator=(AnyView &&src) { Any(std::move(src)).Swap(*this); return *this; }
  template <typename T, typename = DPODOrObjPtrOrObjRef<T>> MLC_INLINE Any &operator=(T &&src) { Any(std::forward<T>(src)).Swap(*this); return *this; }
  template <typename T, typename = DObj<T>> MLC_INLINE Any &operator=(const T *src) { Any(AnyView(src)).Swap(*this); return *this; }
  template <typename T> MLC_INLINE Any &operator=(const Ref<T> &src) { Any(AnyView(src)).Swap(*this); return *this; }
  template <typename T> MLC_INLINE Any &operator=(Ref<T> &&src) { Any(AnyView(std::move(src))).Swap(*this); return *this; }
  template <typename T> MLC_INLINE Any &operator=(const Optional<T> &src) { Any(AnyView(src)).Swap(*this); return *this; }
  template <typename T> MLC_INLINE Any &operator=(Optional<T> &&src) { Any(AnyView(std::move(src))).Swap(*this); return *this; }
  // clang-format on
  /***** Section 3. Conversion to `TPOD` `TObj *`, `Ref<T>`, `Optional<T>` *****/
  template <typename T, typename = DPODOrObjPtrOrObjRef<T>> operator T() const;
  template <typename T> operator Ref<T>() const;
  template <typename T> operator Optional<T>() const;
  /***** Section 4. accessors, comparators and stringify *****/
  MLC_INLINE bool defined() const { return !::mlc::base::IsTypeIndexNone(this->type_index); }
  MLC_INLINE bool has_value() const { return !::mlc::base::IsTypeIndexNone(this->type_index); }
  MLC_INLINE bool operator==(std::nullptr_t) const { return !this->defined(); }
  MLC_INLINE bool operator!=(std::nullptr_t) const { return this->defined(); }
  Str str() const;
  friend std::ostream &operator<<(std::ostream &os, const Any &src);
  /***** Section 5. Runtime-type information *****/
  template <typename DerivedObj> MLC_INLINE bool IsInstance() const {
    static_assert(::mlc::base::IsObj<DerivedObj>, "DerivedObj must be an object type");
    if (::mlc::base::IsTypeIndexPOD(this->type_index)) {
      return false;
    }
    return ::mlc::base::IsInstanceOf<DerivedObj>(this->v.v_obj);
  }
  template <typename DerivedObj> MLC_INLINE DerivedObj *TryCast() {
    return this->IsInstance<DerivedObj>() ? reinterpret_cast<DerivedObj *>(this->v.v_obj) : nullptr;
  }
  template <typename DerivedObj> MLC_INLINE const DerivedObj *TryCast() const {
    return this->IsInstance<DerivedObj>() ? reinterpret_cast<const DerivedObj *>(this->v.v_obj) : nullptr;
  }
  template <typename DerivedObj> inline DerivedObj *Cast() {
    if (!this->IsInstance<DerivedObj>()) {
      MLC_THROW(TypeError) << "Cannot cast from type `" << ::mlc::base::TypeIndex2TypeKey(this->type_index)
                           << "` to type `" << ::mlc::base::Type2Str<DerivedObj>::Run() << "`";
    }
    return reinterpret_cast<DerivedObj *>(this->v.v_obj);
  }
  template <typename DerivedObj> MLC_INLINE const DerivedObj *Cast() const {
    if (!this->IsInstance<DerivedObj>()) {
      MLC_THROW(TypeError) << "Cannot cast from type `" << ::mlc::base::TypeIndex2TypeKey(this->type_index)
                           << "` to type `" << ::mlc::base::Type2Str<DerivedObj>::Run() << "`";
    }
    return reinterpret_cast<const DerivedObj *>(this->v.v_obj);
  }
  int32_t GetTypeIndex() const { return this->type_index; }
  const char *GetTypeKey() const { return ::mlc::base::TypeIndex2TypeInfo(this->type_index)->type_key; }

protected:
  MLC_INLINE void Swap(MLCAny &src) {
    MLCAny tmp = *this;
    *static_cast<MLCAny *>(this) = src;
    src = tmp;
  }
  MLC_INLINE void IncRef() {
    if (!::mlc::base::IsTypeIndexPOD(this->type_index)) {
      ::mlc::base::IncRef(this->v.v_obj);
    }
  }
  MLC_INLINE void DecRef() {
    if (!::mlc::base::IsTypeIndexPOD(this->type_index)) {
      ::mlc::base::DecRef(this->v.v_obj);
    }
  }
  MLC_INLINE void SwitchFromRawStr() {
    if (this->type_index == static_cast<int32_t>(MLCTypeIndex::kMLCRawStr)) {
      this->type_index = static_cast<int32_t>(MLCTypeIndex::kMLCStr);
      this->v.v_obj =
          reinterpret_cast<MLCAny *>(::mlc::base::StrCopyFromCharArray(this->v.v_str, std::strlen(this->v.v_str)));
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
} // namespace mlc

namespace mlc {
namespace base {
template <typename... Args> struct AnyViewArrayFill {
  template <std::size_t i, typename Arg> MLC_INLINE static void Apply(AnyView *v, Arg &&arg) {
    v[i] = AnyView(std::forward<Arg>(arg));
  }
  template <std::size_t... I> MLC_INLINE static void Run(AnyView *v, Args &&...args, std::index_sequence<I...>) {
    using TExpander = int[];
    (void)TExpander{0, (Apply<I>(v, std::forward<Args>(args)), 0)...};
  }
};
} // namespace base
} // namespace mlc

#endif // MLC_BASE_ANY_H_
