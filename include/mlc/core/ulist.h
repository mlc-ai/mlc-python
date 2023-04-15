#ifndef MLC_CORE_ULIST_H_
#define MLC_CORE_ULIST_H_
#include "./ulist_base.h"
#include <initializer_list>
#include <iterator>
#include <type_traits>

namespace mlc {

struct UListObj : protected core::ListBase {
  template <typename> friend struct ::mlc::core::ListBase::ffi;
  friend struct ::mlc::core::ListBase;
  using MLCList::_mlc_header;
  using TElem = Any;
  template <typename State, typename Value> struct Iter;
  using iterator = Iter<IterStateMut, Any>;
  using const_iterator = Iter<IterStateConst, const Any>;
  using reverse_iterator = std::reverse_iterator<UListObj::iterator>;
  using const_reverse_iterator = std::reverse_iterator<UListObj::const_iterator>;

  MLC_INLINE UListObj() : ListBase() {}
  MLC_INLINE UListObj(std::initializer_list<Any> init) : ListBase(init) {}
  template <typename Iter> MLC_INLINE UListObj(Iter first, Iter last) : ListBase(first, last) {}
  template <typename Iter> MLC_INLINE void insert(int64_t i, Iter first, Iter last) {
    ListBase::Insert(i, first, last);
  }
  MLC_INLINE ~UListObj() = default;
  MLC_INLINE void insert(int64_t i, Any data) { ListBase::Insert(i, data); }
  MLC_INLINE void reserve(int64_t capacity) { ListBase::Reserve(capacity); }
  MLC_INLINE void clear() { ListBase::Clear(); }
  MLC_INLINE void resize(int64_t new_size) { ListBase::Resize(new_size); }
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
  MLC_INLINE Any &at(int64_t i) { return this->operator[](i); }
  MLC_INLINE const Any &at(int64_t i) const { return this->operator[](i); }
  MLC_INLINE const Any &front() const { return this->operator[](0); }
  MLC_INLINE const Any &back() const { return this->operator[](this->MLCList::size - 1); }
  MLC_INLINE Any &front() { return this->operator[](0); }
  MLC_INLINE Any &back() { return this->operator[](this->MLCList::size - 1); }
  MLC_INLINE iterator begin();
  MLC_INLINE iterator end();
  MLC_INLINE const_iterator begin() const;
  MLC_INLINE const_iterator end() const;
  MLC_INLINE reverse_iterator rbegin();
  MLC_INLINE reverse_iterator rend();
  MLC_INLINE const_reverse_iterator rbegin() const;
  MLC_INLINE const_reverse_iterator rend() const;
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
      .StaticFn("__init__", &ffi<UListObj>::New)
      .MemFn("__str__", &UListObj::__str__)
      .MemFn("__iter_at__", &ffi<UListObj>::At);
};

template <typename State, typename Value> struct UListObj::Iter {
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = Value;
  using pointer = Value *;
  using reference = std::add_lvalue_reference_t<Value>;
  MLC_INLINE Iter(State i) : i(i) {}
  MLC_INLINE Iter() = default;
  MLC_INLINE Iter(const Iter &) = default;
  MLC_INLINE Iter(Iter &&) = default;
  MLC_INLINE Iter &operator=(const Iter &) = default;
  MLC_INLINE Iter &operator=(Iter &&) = default;
  /* clang-format off */
  MLC_INLINE Iter &operator++() { this->i = this->i.Add(1); return *this; }
  MLC_INLINE Iter &operator--() { this->i = this->i.Add(-1); return *this; }
  MLC_INLINE Iter operator++(int) { State ret = this->i; this->i = this->i.Add(1); return ret; }
  MLC_INLINE Iter operator--(int) { State ret = this->i; this->i = this->i.Add(-1); return ret; }
  MLC_INLINE Iter operator+(difference_type n) const { State ret = this->i; ret.i += n; return ret; }
  MLC_INLINE Iter operator-(difference_type n) const { State ret = this->i; ret.i -= n; return ret; }
  MLC_INLINE Iter &operator+=(difference_type n) { this->i = this->i.Add(n); return *this; }
  MLC_INLINE Iter &operator-=(difference_type n) { this->i = this->i.Add(-n); return *this; }
  /* clang-format on */
  MLC_INLINE difference_type operator-(const Iter &other) const { return i.i - other.i.i; }
  MLC_INLINE bool operator==(const Iter &other) const { return i.i == other.i.i; }
  MLC_INLINE bool operator!=(const Iter &other) const { return i.i != other.i.i; }
  MLC_INLINE reference operator[](difference_type n) const { return this->i.Add(n).At(); }
  MLC_INLINE reference operator*() const { return i.At(); }
  MLC_INLINE pointer operator->() const { return &i.At(); }

protected:
  State i;
};

MLC_INLINE UListObj::iterator UListObj::begin() { return iterator(core::ListBase::Begin()); }
MLC_INLINE UListObj::iterator UListObj::end() { return iterator(core::ListBase::End()); }
MLC_INLINE UListObj::const_iterator UListObj::begin() const { return const_iterator(core::ListBase::Begin()); }
MLC_INLINE UListObj::const_iterator UListObj::end() const { return const_iterator(core::ListBase::End()); }
MLC_INLINE UListObj::reverse_iterator UListObj::rbegin() { return reverse_iterator(end()); }
MLC_INLINE UListObj::reverse_iterator UListObj::rend() { return reverse_iterator(begin()); }
MLC_INLINE UListObj::const_reverse_iterator UListObj::rbegin() const { return const_reverse_iterator(end()); }
MLC_INLINE UListObj::const_reverse_iterator UListObj::rend() const { return const_reverse_iterator(begin()); }

struct UList : public ObjectRef {
  using iterator = UListObj::iterator;
  using reverse_iterator = UListObj::reverse_iterator;
  using const_iterator = UListObj::const_iterator;
  using const_reverse_iterator = UListObj::const_reverse_iterator;
  using TElem = Any;
  /* clang-format off */
  MLC_INLINE UList() : UList(::mlc::base::AllocatorOf<UListObj>::New()) {}
  MLC_INLINE UList(std::initializer_list<Any> init) : UList(::mlc::base::AllocatorOf<UListObj>::New(init)) {}
  template <typename Iter> MLC_INLINE UList(Iter first, Iter last) : UList(::mlc::base::AllocatorOf<UListObj>::New(first, last)) {}
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
  MLC_INLINE iterator begin() { return get()->begin(); }
  MLC_INLINE iterator end() { return get()->end(); }
  MLC_INLINE reverse_iterator rbegin() { return get()->rbegin(); }
  MLC_INLINE reverse_iterator rend() { return get()->rend(); }
  MLC_INLINE const_iterator begin() const { return get()->begin(); }
  MLC_INLINE const_iterator end() const { return get()->end(); }
  MLC_INLINE const_reverse_iterator rbegin() const { return get()->rbegin(); }
  MLC_INLINE const_reverse_iterator rend() const { return get()->rend(); }
  MLC_DEF_OBJ_REF(UList, UListObj, ObjectRef);
};

} // namespace mlc

#endif // MLC_CORE_ULIST_H_
