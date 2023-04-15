#ifndef MLC_LIST_H_
#define MLC_LIST_H_

#include "./error.h"
#include "./ulist.h"
#include <iterator>
#include <type_traits>

namespace mlc {
namespace ffi {

template <typename T> struct ListObj : protected UListObj {
  static_assert(IsContainerElement<T>);
  using TElem = T;
  using UListObj::_mlc_header;
  using UListObj::_type_ancestors;
  using UListObj::_type_depth;
  using UListObj::_type_index;
  using UListObj::_type_info;
  using UListObj::_type_key;
  using UListObj::_type_parent;
  using UListObj::capacity;
  using UListObj::clear;
  using UListObj::data;
  using UListObj::empty;
  using UListObj::erase;
  using UListObj::pop_back;
  using UListObj::reserve;
  using UListObj::size;
  struct Iterator;
  struct ReverseIterator;

  MLC_INLINE ListObj() : UListObj() {}
  MLC_INLINE ListObj(std::initializer_list<T> init) : UListObj(init.begin(), init.end()) {}
  template <typename Iter> MLC_INLINE ListObj(Iter first, Iter last) : UListObj(first, last) {
    static_assert(std::is_convertible_v<typename std::iterator_traits<Iter>::value_type, T>);
  }
  template <typename Iter> MLC_INLINE void insert(int64_t i, Iter first, Iter last) {
    static_assert(std::is_convertible_v<typename std::iterator_traits<Iter>::value_type, T>);
    UListObj::insert(i, first, last);
  }
  MLC_INLINE void insert(int64_t i, T source) { UListObj::insert(i, Any(std::forward<T>(source))); }
  MLC_INLINE void push_back(T data) { UListObj::push_back(Any(std::forward<T>(data))); }
  MLC_INLINE const T front() { return UListObj::front().template Cast<T>(); }
  MLC_INLINE const T back() { return UListObj::back().template Cast<T>(); }
  MLC_INLINE const T front() const { return UListObj::front().template Cast<T>(); }
  MLC_INLINE const T back() const { return UListObj::back().template Cast<T>(); }
  MLC_INLINE const T operator[](int64_t i) const { return UListObj::operator[](i).template Cast<T>(); }
  MLC_INLINE void Set(int64_t i, T data) { UListObj::operator[](i) = Any(std::forward<T>(data)); }
  MLC_INLINE void resize(int64_t new_size) {
    int64_t cur_size = size();
    UListObj::resize(new_size);
    if constexpr (!std::is_same_v<tag::Tag<T>, tag::ObjPtr>) {
      for (int64_t i = cur_size; i < new_size; ++i) {
        UListObj::operator[](i) = T{};
      }
    }
  }
  MLC_INLINE Iterator begin();
  MLC_INLINE Iterator end();
  MLC_INLINE ReverseIterator rbegin();
  MLC_INLINE ReverseIterator rend();
};

template <typename T> struct ListObj<T>::Iterator : public ListBaseIterator<ListObj<T>::Iterator> {
protected:
  using TParent = ListBaseIterator<ListObj<T>::Iterator>;
  using TParent::i;
  using TParent::self;
  using TParent::TParent;

public:
  using pointer = void;
  using reference = const T;
  using difference_type = typename TParent::difference_type;
  MLC_INLINE reference operator[](difference_type n) const {
    return static_cast<UListObj *>(self)->operator[](i + n).template Cast<T>;
  }
  MLC_INLINE reference operator*() const { return static_cast<UListObj *>(self)->operator[](i); }
  pointer operator->() const = delete; // Use `operator*` instead
};

template <typename T> struct ListObj<T>::ReverseIterator : public std::reverse_iterator<Iterator> {
  using std::reverse_iterator<Iterator>::reverse_iterator;
};

/* clang-format off */
template <typename T> MLC_INLINE auto ListObj<T>::begin() -> ListObj<T>::Iterator { return Iterator(static_cast<ListBase *>(this), 0); }
template <typename T> MLC_INLINE auto ListObj<T>::end() -> ListObj<T>::Iterator { return Iterator(static_cast<ListBase *>(this), this->MLCList::size); }
template <typename T> MLC_INLINE auto ListObj<T>::rbegin() -> ListObj<T>::ReverseIterator { return ReverseIterator(end()); }
template <typename T> MLC_INLINE auto ListObj<T>::rend() -> ListObj<T>::ReverseIterator { return ReverseIterator(begin()); }
/* clang-format on */

template <typename T> struct List : protected UList {
  static_assert(IsContainerElement<T>);
  using TElem = T;
  using Iterator = typename ListObj<T>::Iterator;
  using ReverseIterator = typename ListObj<T>::ReverseIterator;
  /* clang-format off */
  MLC_INLINE List() : List(details::AllocatorOf<ListObj<T>>::New()) {}
  MLC_INLINE List(std::initializer_list<T> init) : List(details::AllocatorOf<ListObj<T>>::New(init)) {}
  template <typename Iter> MLC_INLINE List(Iter first, Iter last) : List(details::AllocatorOf<ListObj<T>>::New(first, last)) {}
  template <typename Iter> MLC_INLINE void insert(int64_t i, Iter first, Iter last) { get()->insert(i, first, last); }
  /* clang-format on */
  MLC_INLINE void insert(int64_t i, T source) { get()->insert(i, std::forward<T>(source)); }
  MLC_INLINE void reserve(int64_t capacity) { get()->reserve(capacity); }
  MLC_INLINE void clear() { get()->clear(); }
  MLC_INLINE void resize(int64_t new_size) { get()->resize(new_size); }
  MLC_INLINE const T operator[](int64_t i) const { return get()->operator[](i); }
  MLC_INLINE void Set(int64_t i, T data) { get()->Set(i, std::forward<T>(data)); }
  MLC_INLINE void push_back(T data) { get()->push_back(std::forward<T>(data)); }
  MLC_INLINE const T front() { return get()->front(); }
  MLC_INLINE const T back() { return get()->back(); }
  MLC_INLINE const T front() const { return get()->front(); }
  MLC_INLINE const T back() const { return get()->back(); }
  MLC_INLINE void pop_back() { get()->pop_back(); }
  MLC_INLINE void erase(int64_t i) { get()->erase(i); }
  MLC_INLINE int64_t size() const { return get()->size(); }
  MLC_INLINE int64_t capacity() const { return get()->capacity(); }
  MLC_INLINE bool empty() const { return get()->empty(); }
  MLC_INLINE Iterator begin() { return get()->begin(); }
  MLC_INLINE Iterator end() { return get()->end(); }
  MLC_INLINE ReverseIterator rbegin() { return get()->rbegin(); }
  MLC_INLINE ReverseIterator rend() { return get()->rend(); }

  MLC_DEF_OBJ_REF(List, ListObj<T>, UList);
};

namespace details {
template <typename T> MLC_INLINE_NO_MSVC void NestedTypeCheck<List<T>>::Run(const MLCAny &any) {
  try {
    static_cast<const AnyView &>(any).Cast<UList>();
  } catch (const Exception &e) {
    throw NestedTypeError(e.what()).NewFrame(Type2Str<UList>::Run());
  }
  if constexpr (!std::is_same_v<T, Any>) {
    UListObj *list = reinterpret_cast<UListObj *>(any.v_obj);
    int64_t size = list->size();
    for (int32_t i = 0; i < size; ++i) {
      try {
        NestedTypeCheck<T>::Run(list->data()[i]);
      } catch (NestedTypeError &e) {
        throw e.NewIndex(i);
      }
    }
  }
}
} // namespace details

template <typename T> MLC_INLINE_NO_MSVC List<T> UList::AsTyped() const { return List<T>(get()->AsTyped<T>()); }

template <typename T> MLC_INLINE_NO_MSVC ListObj<T> *UListObj::AsTyped() const {
  UListObj *self = const_cast<UListObj *>(this);
  try {
    AnyView view(self);
    details::NestedTypeCheck<List<T>>::Run(view);
  } catch (details::NestedTypeError &e) {
    std::ostringstream os;
    e.Format(os, Type2Str<List<T>>::Run());
    MLC_THROW(NestedTypeError) << os.str();
  }
  return reinterpret_cast<ListObj<T> *>(self);
}

template <typename Elem> struct ObjPtrTraits<ListObj<Elem>> {
  using T = ListObj<Elem>;
  MLC_INLINE static void PtrToAnyView(const T *v, MLCAny *ret) { ObjPtrTraitsDefault<T>::PtrToAnyView(v, ret); }
  MLC_INLINE static T *AnyToOwnedPtr(const MLCAny *v) { return AnyToUnownedPtr(v); }
  MLC_INLINE static T *AnyToUnownedPtr(const MLCAny *v) {
    return ObjPtrTraitsDefault<UListObj>::AnyToUnownedPtr(v)->AsTyped<Elem>();
  }
};

} // namespace ffi
} // namespace mlc

#endif // MLC_LIST_H_
