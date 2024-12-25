#ifndef MLC_CORE_LIST_H_
#define MLC_CORE_LIST_H_

#include "./list_base.h"
#include "./object.h"
#include <iterator>
#include <type_traits>

namespace mlc {

struct UListObj : protected core::ListBase {
  using MLCList::_mlc_header;
  using TElem = Any;

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

  template <typename State, typename Value> struct Iter {
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
  using iterator = Iter<IterStateMut, Any>;
  using const_iterator = Iter<IterStateConst, const Any>;
  using reverse_iterator = std::reverse_iterator<UListObj::iterator>;
  using const_reverse_iterator = std::reverse_iterator<UListObj::const_iterator>;

  MLC_INLINE iterator begin() { return iterator(core::ListBase::Begin()); }
  MLC_INLINE iterator end() { return iterator(core::ListBase::End()); }
  MLC_INLINE const_iterator begin() const { return const_iterator(core::ListBase::Begin()); }
  MLC_INLINE const_iterator end() const { return const_iterator(core::ListBase::End()); }
  MLC_INLINE reverse_iterator rbegin() { return reverse_iterator(end()); }
  MLC_INLINE reverse_iterator rend() { return reverse_iterator(begin()); }
  MLC_INLINE const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  MLC_INLINE const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

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

  MLC_DEF_STATIC_TYPE(UListObj, Object, MLCTypeIndex::kMLCList, "object.List");
};

struct UList : public ObjectRef {
  using iterator = UListObj::iterator;
  using reverse_iterator = UListObj::reverse_iterator;
  using const_iterator = UListObj::const_iterator;
  using const_reverse_iterator = UListObj::const_reverse_iterator;
  using TElem = Any;
  friend std::ostream &operator<<(std::ostream &os, const UList &self) {
    os << AnyView(self);
    return os;
  }
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
  MLC_DEF_OBJ_REF(UList, UListObj, ObjectRef)
      .FieldReadOnly("size", &MLCList::size)
      .FieldReadOnly("capacity", &MLCList::capacity)
      .FieldReadOnly("data", &MLCList::data)
      .StaticFn("__init__", &::mlc::core::ListBase::Accessor<UListObj>::New)
      .MemFn("__str__", &UListObj::__str__)
      .MemFn("__iter_at__", &::mlc::core::ListBase::Accessor<UListObj>::At);
};

template <typename T> struct ListObj : protected UListObj {
  static_assert(::mlc::base::IsContainerElement<T>);
  template <typename, typename> friend struct ::mlc::base::TypeTraits;
  using TElem = T;
  using UListObj::_mlc_header;
  using UListObj::_type_ancestor_types;
  using UListObj::_type_ancestors;
  using UListObj::_type_depth;
  using UListObj::_type_index;
  using UListObj::_type_info;
  using UListObj::_type_key;
  using UListObj::_type_kind;
  using UListObj::_type_parent;
  using UListObj::capacity;
  using UListObj::clear;
  using UListObj::data;
  using UListObj::empty;
  using UListObj::erase;
  using UListObj::pop_back;
  using UListObj::reserve;
  using UListObj::size;
  using iterator = UListObj::iterator;
  using const_iterator = UListObj::const_iterator;
  using reverse_iterator = UListObj::reverse_iterator;
  using const_reverse_iterator = UListObj::const_reverse_iterator;

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
  MLC_INLINE const T front() { return UListObj::front(); }
  MLC_INLINE const T back() { return UListObj::back(); }
  MLC_INLINE const T front() const { return UListObj::front(); }
  MLC_INLINE const T back() const { return UListObj::back(); }
  MLC_INLINE const T operator[](int64_t i) const { return UListObj::operator[](i); }
  MLC_INLINE void Set(int64_t i, T data) { UListObj::operator[](i) = Any(std::forward<T>(data)); }
  MLC_INLINE void resize(int64_t new_size) {
    int64_t cur_size = size();
    UListObj::resize(new_size);
    if constexpr (!(::mlc::base::IsObjRef<T> || ::mlc::base::IsRef<T>)) {
      for (int64_t i = cur_size; i < new_size; ++i) {
        UListObj::operator[](i) = Any(T{});
      }
    }
  }
  MLC_INLINE iterator begin() { return UListObj::begin(); }
  MLC_INLINE iterator end() { return UListObj::end(); }
  MLC_INLINE const_iterator begin() const { return UListObj::begin(); }
  MLC_INLINE const_iterator end() const { return UListObj::end(); }
  MLC_INLINE reverse_iterator rbegin() { return UListObj::rbegin(); }
  MLC_INLINE reverse_iterator rend() { return UListObj::rend(); }
  MLC_INLINE const_reverse_iterator rbegin() const { return UListObj::rbegin(); }
  MLC_INLINE const_reverse_iterator rend() const { return UListObj::rend(); }
};

template <typename T> struct List : protected UList {
  static_assert(::mlc::base::IsContainerElement<T>);
  using TElem = T;
  using iterator = UListObj::iterator;
  using reverse_iterator = UListObj::reverse_iterator;
  using const_iterator = UListObj::const_iterator;
  using const_reverse_iterator = UListObj::const_reverse_iterator;
  friend std::ostream &operator<<(std::ostream &os, const List &self) {
    os << AnyView(self);
    return os;
  }
  /* clang-format off */
  MLC_INLINE List() : List(::mlc::base::AllocatorOf<ListObj<T>>::New()) {}
  MLC_INLINE List(ListObj<T>* p) : UList(p) {}
  MLC_INLINE List(std::initializer_list<T> init) : List(::mlc::base::AllocatorOf<ListObj<T>>::New(init)) {}
  template <typename Iter> MLC_INLINE List(Iter first, Iter last) : List(::mlc::base::AllocatorOf<ListObj<T>>::New(first, last)) {}
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
  MLC_INLINE iterator begin() { return get()->begin(); }
  MLC_INLINE iterator end() { return get()->end(); }
  MLC_INLINE reverse_iterator rbegin() { return get()->rbegin(); }
  MLC_INLINE reverse_iterator rend() { return get()->rend(); }
  MLC_INLINE const_iterator begin() const { return get()->begin(); }
  MLC_INLINE const_iterator end() const { return get()->end(); }
  MLC_INLINE const_reverse_iterator rbegin() const { return get()->rbegin(); }
  MLC_INLINE const_reverse_iterator rend() const { return get()->rend(); }

  MLC_DEF_OBJ_REF(List, ListObj<T>, UList);
};

template <typename T> MLC_INLINE_NO_MSVC List<T> UList::AsTyped() const { return List<T>(get()->AsTyped<T>()); }

template <typename T> MLC_INLINE_NO_MSVC ListObj<T> *UListObj::AsTyped() const {
  UListObj *self = const_cast<UListObj *>(this);
  try {
    AnyView view(self);
    core::NestedTypeCheck<List<T>>::Run(view);
  } catch (core::NestedTypeError &e) {
    std::ostringstream os;
    e.Format(os, base::Type2Str<List<T>>::Run());
    MLC_THROW(NestedTypeError) << os.str();
  }
  return reinterpret_cast<ListObj<T> *>(self);
}
} // namespace mlc

#endif // MLC_CORE_LIST_H_
