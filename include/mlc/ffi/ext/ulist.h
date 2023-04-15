#ifndef MLC_ULIST_H_
#define MLC_ULIST_H_
#include "./ulist_base.h"
#include <initializer_list>
#include <iterator>
#include <mlc/ffi/core/core.h>

namespace mlc {
namespace ffi {

struct UListObj : protected ListBase {
  template <typename> friend struct ListBase::ffi;
  friend struct ListBase;
  using MLCList::_mlc_header;
  struct Iterator;
  struct ReverseIterator;

  MLC_INLINE UListObj() : ListBase() {}
  MLC_INLINE UListObj(std::initializer_list<Any> init) : ListBase(init) {}
  template <typename Iter> MLC_INLINE UListObj(Iter first, Iter last) : ListBase(first, last) {}
  template <typename Iter> MLC_INLINE void insert(int64_t i, Iter first, Iter last) {
    ListBase::insert(i, first, last);
  }
  MLC_INLINE ~UListObj() = default;
  MLC_INLINE void insert(int64_t i, Any data) { ListBase::insert(i, data); }
  MLC_INLINE void reserve(int64_t capacity) { ListBase::reserve(capacity); }
  MLC_INLINE void clear() { ListBase::clear(); }
  MLC_INLINE void resize(int64_t new_size) { ListBase::resize(new_size); }
  MLC_INLINE MLCAny *data() { return reinterpret_cast<MLCAny *>(MLCList::data); }
  MLC_INLINE const MLCAny *data() const { return reinterpret_cast<const MLCAny *>(MLCList::data); }
  MLC_INLINE void push_back(Any data) { ListBase::Append(data); }
  MLC_INLINE void pop_back() { this->Replace(this->MLCList::size - 1, this->MLCList::size, 0, nullptr); }
  MLC_INLINE void erase(int64_t i) { this->Replace(i, i + 1, 0, nullptr); }
  MLC_INLINE int64_t size() const { return this->MLCList::size; }
  MLC_INLINE int64_t capacity() const { return this->MLCList::capacity; }
  MLC_INLINE bool empty() const { return size() == 0; }
  MLC_INLINE Any &operator[](int64_t i) { return static_cast<Any *>(data())[i]; }
  MLC_INLINE const Any &operator[](int64_t i) const { return static_cast<const Any *>(data())[i]; }
  MLC_INLINE const Any &front() const { return this->operator[](0); }
  MLC_INLINE const Any &back() const { return this->operator[](this->MLCList::size - 1); }
  MLC_INLINE Any &front() { return this->operator[](0); }
  MLC_INLINE Any &back() { return this->operator[](this->MLCList::size - 1); }
  MLC_INLINE Iterator begin();
  MLC_INLINE Iterator end();
  MLC_INLINE ReverseIterator rbegin();
  MLC_INLINE ReverseIterator rend();
  template <typename T> MLC_INLINE_NO_MSVC ListObj<T> *AsTyped() const;

  std::string __str__() const {
    if (this->MLCList::size == 0) {
      return "[]";
    }
    std::ostringstream os;
    os << '[' << this->operator[](0);
    for (int64_t i = 1; i < this->MLCList::size; ++i) {
      os << ", " << this->operator[](i);
    }
    os << ']';
    return os.str();
  }

  MLC_DEF_STATIC_TYPE(UListObj, Object, MLCTypeIndex::kMLCList, "object.List")
      .FieldReadOnly("size", &MLCList::size)
      .FieldReadOnly("capacity", &MLCList::capacity)
      .FieldReadOnly("data", &MLCList::data)
      .Method("__str__", &UListObj::__str__)
      .Method("__init__", &ffi<UListObj>::New)
      .Method("__iter_at__", &ffi<UListObj>::At);
};

struct UListObj::Iterator : public ListBaseIterator<UListObj::Iterator> {
public:
  using pointer = Any *;
  using reference = Any &;
  MLC_INLINE reference operator[](difference_type n) const { return static_cast<UListObj *>(self)->operator[](i + n); }
  MLC_INLINE reference operator*() const { return static_cast<UListObj *>(self)->operator[](i); }
  MLC_INLINE pointer operator->() const { return &static_cast<UListObj *>(self)->operator[](i); }

protected:
  using ListBaseIterator::i;
  using ListBaseIterator::ListBaseIterator;
  using ListBaseIterator::self;
};

struct UListObj::ReverseIterator : public std::reverse_iterator<UListObj::Iterator> {
  using std::reverse_iterator<UListObj::Iterator>::reverse_iterator;
};

MLC_INLINE UListObj::Iterator UListObj::begin() { return Iterator(static_cast<ListBase *>(this), 0); }
MLC_INLINE UListObj::Iterator UListObj::end() { return Iterator(static_cast<ListBase *>(this), this->MLCList::size); }
MLC_INLINE UListObj::ReverseIterator UListObj::rbegin() { return ReverseIterator(end()); }
MLC_INLINE UListObj::ReverseIterator UListObj::rend() { return ReverseIterator(begin()); }

struct UList : public ObjectRef {
  using Iterator = UListObj::Iterator;
  using ReverseIterator = UListObj::ReverseIterator;
  /* clang-format off */
  MLC_INLINE UList() : UList(details::AllocatorOf<UListObj>::New()) {}
  MLC_INLINE UList(std::initializer_list<Any> init) : UList(details::AllocatorOf<UListObj>::New(init)) {}
  template <typename Iter> MLC_INLINE UList(Iter first, Iter last) : UList(details::AllocatorOf<UListObj>::New(first, last)) {}
  template <typename Iter> MLC_INLINE void insert(int64_t i, Iter first, Iter last) { get()->insert(i, first, last); }
  /* clang-format on */
  template <typename T> MLC_INLINE_NO_MSVC List<T> AsTyped() const;
  MLC_INLINE void insert(int64_t i, Any data) { get()->insert(i, data); }
  MLC_INLINE void reserve(int64_t capacity) { get()->reserve(capacity); }
  MLC_INLINE void clear() { get()->clear(); }
  MLC_INLINE void resize(int64_t new_size) { get()->resize(new_size); }
  MLC_INLINE MLCAny *data() { return get()->data(); }
  MLC_INLINE const MLCAny *data() const { return get()->data(); }
  MLC_INLINE void push_back(Any data) { get()->push_back(data); }
  MLC_INLINE void pop_back() { get()->pop_back(); }
  MLC_INLINE void erase(int64_t i) { get()->erase(i); }
  MLC_INLINE int64_t size() const { return get()->size(); }
  MLC_INLINE int64_t capacity() const { return get()->capacity(); }
  MLC_INLINE bool empty() const { return get()->empty(); }
  MLC_INLINE Any &operator[](int64_t i) { return get()->operator[](i); }
  MLC_INLINE const Any &operator[](int64_t i) const { return static_cast<const Any *>(data())[i]; }
  MLC_INLINE const Any &front() const { return get()->front(); }
  MLC_INLINE const Any &back() const { return get()->back(); }
  MLC_INLINE Any &front() { return get()->front(); }
  MLC_INLINE Any &back() { return get()->back(); }
  MLC_INLINE Iterator begin() { return get()->begin(); }
  MLC_INLINE Iterator end() { return get()->end(); }
  MLC_INLINE ReverseIterator rbegin() { return get()->rbegin(); }
  MLC_INLINE ReverseIterator rend() { return get()->rend(); }
  MLC_DEF_OBJ_REF(UList, UListObj, ObjectRef);
};

} // namespace ffi
} // namespace mlc

#endif // MLC_ULIST_H_
