#ifndef MLC_BASE_OPTIONAL_H_
#define MLC_BASE_OPTIONAL_H_
#include "./ref.h"
#include "./utils.h"
#include <type_traits>

namespace mlc {

template <typename T> struct Optional {
  using TObj = ::mlc::base::ObjFromRef<T>;
  using TSelf = Optional<T>;

  template <typename> friend struct ::mlc::Optional;
  template <typename> friend struct ::mlc::base::Type2Str;
  template <typename U>
  using Forward = std::enable_if_t<std::is_constructible_v<Ref<TObj>, U> && //
                                   !std::is_same_v<std::decay_t<U>, Any> && //
                                   !std::is_same_v<std::decay_t<U>, AnyView>>;

  /***** Section 1. Default constructor/destructors *****/
  MLC_INLINE ~Optional() = default;
  MLC_INLINE Optional() = default;
  MLC_INLINE Optional(::mlc::NullType) : ref_() {}
  MLC_INLINE Optional(std::nullptr_t) : ref_() {}
  MLC_INLINE Optional &operator=(::mlc::NullType) { return this->Reset(); }
  MLC_INLINE Optional &operator=(std::nullptr_t) { return this->Reset(); }
  MLC_INLINE Optional(const TSelf &src) : ref_(src.ref_) {}
  MLC_INLINE Optional(TSelf &&src) : ref_(std::move(src.ref_)) {}
  MLC_INLINE TSelf &operator=(const TSelf &other) {
    this->ref_ = other.ref_;
    return *this;
  }
  MLC_INLINE TSelf &operator=(TSelf &&other) {
    this->ref_ = std::move(other.ref_);
    return *this;
  }
  /***** Section 2. Accessor and bool operator *****/
  MLC_INLINE const TObj *get() const { return ref_.get(); }
  MLC_INLINE const TObj *value() const { return ref_.get(); }
  MLC_INLINE TObj *get() { return ref_.get(); }
  MLC_INLINE TObj *value() { return ref_.get(); }
  MLC_INLINE const TObj *operator->() const { return ref_.operator->(); }
  MLC_INLINE const TObj &operator*() const { return ref_.operator*(); }
  MLC_INLINE TObj *operator->() { return ref_.operator->(); }
  MLC_INLINE TObj &operator*() { return ref_.operator*(); }
  MLC_INLINE bool defined() const { return ref_.defined(); }
  MLC_INLINE bool has_value() const { return ref_.defined(); }
  MLC_INLINE bool operator==(std::nullptr_t) const { return !defined(); }
  MLC_INLINE bool operator!=(std::nullptr_t) const { return defined(); }
  MLC_INLINE operator bool() const { return defined(); }
  /***** Section 3. From anything that could construct `Ref<TObj>` *****/
  template <typename U, typename = Forward<U>> MLC_INLINE Optional(U &&src) : ref_(std::forward<U>(src)) {}
  template <typename U, typename = Forward<U>> MLC_INLINE TSelf &operator=(U &&src) {
    TSelf(std::forward<U>(src)).Swap(*this);
    return *this;
  }
  /***** Section 4. Conversion between AnyView/Any *****/
  MLC_INLINE Optional(const Any &src) : ref_() {
    if (src.type_index != static_cast<int32_t>(MLCTypeIndex::kMLCNone)) {
      this->ref_ = Ref<TObj>(src);
    }
  }
  MLC_INLINE Optional(const AnyView &src) : ref_() {
    if (src.type_index != static_cast<int32_t>(MLCTypeIndex::kMLCNone)) {
      this->ref_ = Ref<TObj>(src);
    }
  }
  MLC_INLINE TSelf &operator=(const Any &src) {
    TSelf(src).Swap(*this);
    return *this;
  }
  MLC_INLINE TSelf &operator=(const AnyView &src) {
    TSelf(src).Swap(*this);
    return *this;
  }
  MLC_INLINE operator AnyView() const { return ref_.operator AnyView(); }
  MLC_INLINE operator Any() const { return ref_.operator Any(); }
  /***** Section 5. Misc *****/
  MLC_INLINE TSelf &Reset() {
    ref_.Reset();
    return *this;
  }

protected:
  Ref<TObj> ref_;
  void Swap(TSelf &other) { std::swap(this->ref_, other.ref_); }
};

} // namespace mlc

#endif // MLC_BASE_OPTIONAL_H_
