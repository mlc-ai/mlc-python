#ifndef MLC_CORE_DICT_H_
#define MLC_CORE_DICT_H_
#include "./dict_base.h"
#include "./object.h"
#include <initializer_list>
#include <type_traits>

namespace mlc {
struct UDictObj : protected ::mlc::core::DictBase {
  template <typename> friend struct ::mlc::core::DictBase::Accessor;
  using Acc = ::mlc::core::DictBase::Accessor<UDictObj>;
  using TKey = Any;
  using TValue = Any;
  using MLCDict::_mlc_header;

  MLC_INLINE ~UDictObj() = default;
  MLC_INLINE UDictObj(int64_t capacity) : DictBase(capacity) {}
  MLC_INLINE UDictObj() : DictBase() {}
  template <typename Iter> MLC_INLINE UDictObj(Iter begin, Iter end) : DictBase(std::distance(begin, end) * 2) {
    Acc::InsertRange(this, begin, end);
  }
  MLC_INLINE Any &at(const Any &key) { return Acc::At(this, key); }
  MLC_INLINE const Any &at(const Any &key) const { return Acc::At(this, key); }
  MLC_INLINE Any &operator[](const Any &key) { return Acc::Bracket(this, key); }
  MLC_INLINE const Any &operator[](const Any &key) const { return Acc::Bracket(this, key); }
  MLC_INLINE int64_t size() const { return this->MLCDict::size; }
  MLC_INLINE bool empty() const { return this->MLCDict::size == 0; }
  MLC_INLINE int64_t count(const Any &key) const { return Acc::Lookup(this, key).IsNone() ? 0 : 1; }
  MLC_INLINE void clear() { this->Clear(); }

  template <typename State, typename Value> struct Iter {
    friend struct UDictObj;
    using iterator_category = std::bidirectional_iterator_tag;
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
    MLC_INLINE Iter &operator++() { this->i = this->i.Add(); return *this; }
    MLC_INLINE Iter &operator--() { this->i = this->i.Sub(); return *this; }
    MLC_INLINE Iter operator++(int) { State ret = this->i; this->i = this->i.Add(); return ret; }
    MLC_INLINE Iter operator--(int) { State ret = this->i; this->i = this->i.Sub(); return ret; }
    /* clang-format on */
    MLC_INLINE bool operator==(const Iter &other) const { return i.i == other.i.i; }
    MLC_INLINE bool operator!=(const Iter &other) const { return i.i != other.i.i; }
    MLC_INLINE reference operator*() const { return i.At(); }
    MLC_INLINE pointer operator->() const { return &i.At(); }

  protected:
    State i;
  };
  using iterator = Iter<IterStateMut, std::pair<const Any, Any>>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_iterator = Iter<IterStateConst, const std::pair<const Any, const Any>>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  MLC_INLINE iterator begin() { return iterator(::mlc::core::DictBase::Begin()); }
  MLC_INLINE iterator end() { return iterator(::mlc::core::DictBase::End()); }
  MLC_INLINE const_iterator begin() const { return const_iterator(::mlc::core::DictBase::Begin()); }
  MLC_INLINE const_iterator end() const { return const_iterator(::mlc::core::DictBase::End()); }
  MLC_INLINE reverse_iterator rbegin() { return reverse_iterator(end()); }
  MLC_INLINE reverse_iterator rend() { return reverse_iterator(begin()); }
  MLC_INLINE const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  MLC_INLINE const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
  // clang-format off
  MLC_INLINE iterator find(const Any &key) { return iterator(IterStateMut{this, Acc::Find(this, key)}); }
  MLC_INLINE const_iterator find(const Any &key) const { return const_iterator(IterStateConst{this, Acc::Find(this, key)}); }
  MLC_INLINE void erase(const Any &key) { Acc::Erase(this, key); }
  MLC_INLINE void erase(const iterator &it) { Acc::Erase(this, it.i.i); }
  MLC_INLINE void erase(const const_iterator &it) { Acc::Erase(this, it.i.i); }
  // clang-format on
  template <typename K, typename V> MLC_INLINE_NO_MSVC DictObj<K, V> *AsTyped() const;

  std::string __str__() const {
    std::ostringstream os;
    bool is_first = true;
    os << '{';
    this->IterateAll([&](uint8_t *, MLCAny *key, MLCAny *value) {
      if (is_first) {
        is_first = false;
      } else {
        os << ", ";
      }
      os << *static_cast<AnyView *>(key) << ": " << *static_cast<AnyView *>(value);
    });
    os << '}';
    return os.str();
  }

  MLC_INLINE static uint64_t Hash(const MLCAny &a) { return ::mlc::base::AnyHash(a); }
  MLC_INLINE static bool Equal(const MLCAny &a, const MLCAny &b) { return ::mlc::base::AnyEqual(a, b); }

  MLC_DEF_STATIC_TYPE(UDictObj, Object, MLCTypeIndex::kMLCDict, "object.Dict");
};

struct UDict : public ObjectRef {
  using TKey = Any;
  using TValue = Any;
  using iterator = UDictObj::iterator;
  using reverse_iterator = UDictObj::reverse_iterator;
  using const_iterator = UDictObj::const_iterator;
  using const_reverse_iterator = UDictObj::const_reverse_iterator;
  friend std::ostream &operator<<(std::ostream &os, const UDict &self) {
    os << AnyView(self);
    return os;
  }

  MLC_INLINE UDict() : UDict(::mlc::base::AllocatorOf<UDictObj>::New()) {}
  MLC_INLINE UDict(std::initializer_list<std::pair<Any, Any>> init)
      : UDict(::mlc::base::AllocatorOf<UDictObj>::New(init.begin(), init.end())) {}
  template <typename Iter>
  MLC_INLINE UDict(Iter begin, Iter end) : UDict(::mlc::base::AllocatorOf<UDictObj>::New(begin, end)) {}
  template <typename K, typename V> MLC_INLINE_NO_MSVC Dict<K, V> AsTyped() const;
  MLC_INLINE iterator find(const Any &key) { return get()->find(key); }
  MLC_INLINE const_iterator find(const Any &key) const { return get()->find(key); }
  MLC_INLINE void erase(const Any &key) { get()->erase(key); }
  MLC_INLINE void erase(const iterator &it) { get()->erase(it); }
  MLC_INLINE void erase(const const_iterator &it) { get()->erase(it); }
  MLC_INLINE Any &at(const Any &key) { return get()->at(key); }
  MLC_INLINE const Any &at(const Any &key) const { return get()->at(key); }
  MLC_INLINE Any &operator[](const Any &key) { return get()->operator[](key); }
  MLC_INLINE const Any &operator[](const Any &key) const { return get()->operator[](key); }
  MLC_INLINE int64_t size() const { return get()->size(); }
  MLC_INLINE bool empty() const { return get()->empty(); }
  MLC_INLINE int64_t count(const Any &key) const { return get()->count(key); }
  MLC_INLINE void clear() { get()->clear(); }
  MLC_INLINE iterator begin() { return get()->begin(); }
  MLC_INLINE iterator end() { return get()->end(); }
  MLC_INLINE reverse_iterator rbegin() { return get()->rbegin(); }
  MLC_INLINE reverse_iterator rend() { return get()->rend(); }
  MLC_INLINE const_iterator begin() const { return get()->begin(); }
  MLC_INLINE const_iterator end() const { return get()->end(); }
  MLC_INLINE const_reverse_iterator rbegin() const { return get()->rbegin(); }
  MLC_INLINE const_reverse_iterator rend() const { return get()->rend(); }
  MLC_DEF_OBJ_REF(UDict, UDictObj, ObjectRef)
      .FieldReadOnly("capacity", &MLCDict::capacity)
      .FieldReadOnly("size", &MLCDict::size)
      .FieldReadOnly("data", &MLCDict::data)
      .StaticFn("__init__", ::mlc::core::DictBase::Accessor<UDictObj>::New)
      .MemFn("__str__", &UDictObj::__str__)
      .MemFn("__getitem__", ::mlc::core::DictBase::Accessor<UDictObj>::GetItem)
      .MemFn("__iter_get_key__", ::mlc::core::DictBase::Accessor<UDictObj>::GetKey)
      .MemFn("__iter_get_value__", ::mlc::core::DictBase::Accessor<UDictObj>::GetValue)
      .MemFn("__iter_advance__", ::mlc::core::DictBase::Accessor<UDictObj>::Advance);
};

template <typename K, typename V> struct DictObj : protected UDictObj {
  static_assert(::mlc::base::IsContainerElement<K>);
  static_assert(::mlc::base::IsContainerElement<V>);
  template <typename, typename> friend struct ::mlc::base::TypeTraits;
  using Acc = ::mlc::core::DictBase::Accessor<UDictObj>;
  using TKey = K;
  using TValue = V;
  using UDictObj::_mlc_header;
  using UDictObj::_type_ancestor_types;
  using UDictObj::_type_ancestors;
  using UDictObj::_type_depth;
  using UDictObj::_type_index;
  using UDictObj::_type_info;
  using UDictObj::_type_key;
  using UDictObj::_type_kind;
  using UDictObj::_type_parent;
  using UDictObj::capacity;
  using UDictObj::clear;
  using UDictObj::empty;
  using UDictObj::size;

  template <bool is_const> struct Iter {
  private:
    template <typename, typename> friend struct DictObj;
    using State = typename ::mlc::core::DictBase::IterState<is_const>;
    using _ValueType = std::pair<const K, V>;
    using ValueType = std::conditional_t<is_const, const _ValueType, _ValueType>;
    State i;

  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = ValueType;
    using pointer = void;
    using reference = value_type;
    MLC_INLINE Iter(State i) : i(i) {}
    MLC_INLINE Iter() = default;
    MLC_INLINE Iter(const Iter &) = default;
    MLC_INLINE Iter(Iter &&) = default;
    MLC_INLINE Iter &operator=(const Iter &) = default;
    MLC_INLINE Iter &operator=(Iter &&) = default;
    /* clang-format off */
    MLC_INLINE Iter &operator++() { this->i = this->i.Add(); return *this; }
    MLC_INLINE Iter &operator--() { this->i = this->i.Sub(); return *this; }
    MLC_INLINE Iter operator++(int) { State ret = this->i; this->i = this->i.Add(); return ret; }
    MLC_INLINE Iter operator--(int) { State ret = this->i; this->i = this->i.Sub(); return ret; }
    /* clang-format on */
    MLC_INLINE bool operator==(const Iter &other) const { return i.i == other.i.i; }
    MLC_INLINE bool operator!=(const Iter &other) const { return i.i != other.i.i; }
    inline reference operator*() const {
      const auto &kv = i.At();
      K first = kv.first;
      V second = kv.second;
      return std::pair<const K, V>{first, second};
    }
    MLC_INLINE pointer operator->() const = delete;
  };
  using iterator = Iter<false>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_iterator = Iter<true>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  MLC_INLINE DictObj() : UDictObj() {}
  MLC_INLINE DictObj(std::initializer_list<std::pair<K, V>> init) : UDictObj(init.size() * 2) {
    for (const auto &pair : init) {
      UDictObj::operator[](pair.first) = pair.second;
    }
  }
  template <typename Iter> MLC_INLINE DictObj(Iter begin, Iter end) : UDictObj(begin, end) {
    using IterD = typename std::iterator_traits<Iter>::value_type;
    static_assert(std::is_convertible_v<typename std::tuple_element_t<0, IterD>, K>);
    static_assert(std::is_convertible_v<typename std::tuple_element_t<1, IterD>, V>);
  }
  MLC_INLINE const V at(const K &key) const { return UDictObj::at(key); }
  MLC_INLINE const V operator[](const K &key) const { return UDictObj::operator[](key); }
  MLC_INLINE int64_t count(const K &key) const { return UDictObj::count(key); }
  MLC_INLINE void Set(K key, V value) { UDictObj::operator[](key) = value; }
  // clang-format off
  MLC_INLINE iterator find(const K &key) { return iterator(IterStateMut{this, Acc::Find(this, AnyView(key))}); }
  MLC_INLINE const_iterator find(const K &key) const { return const_iterator(IterStateConst{this, Acc::Find(this, AnyView(key))}); }
  MLC_INLINE void erase(const K &key) { UDictObj::erase(key); }
  MLC_INLINE void erase(const iterator &it) { UDictObj::erase(UDictObj::IterStateMut(it.i)); }
  MLC_INLINE void erase(const const_iterator &it) { UDictObj::erase(UDictObj::IterStateConst(it.i)); }
  // clang-format on
  MLC_INLINE iterator begin() { return ::mlc::core::DictBase::Begin(); }
  MLC_INLINE iterator end() { return ::mlc::core::DictBase::End(); }
  MLC_INLINE reverse_iterator rbegin() { return reverse_iterator(end()); }
  MLC_INLINE reverse_iterator rend() { return reverse_iterator(begin()); }
  MLC_INLINE const_iterator begin() const { return ::mlc::core::DictBase::Begin(); }
  MLC_INLINE const_iterator end() const { return ::mlc::core::DictBase::End(); }
  MLC_INLINE const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  MLC_INLINE const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
};

template <typename K, typename V> struct Dict : protected UDict {
  static_assert(::mlc::base::IsContainerElement<K>);
  static_assert(::mlc::base::IsContainerElement<V>);
  friend std::ostream &operator<<(std::ostream &os, const Dict &self) {
    os << AnyView(self);
    return os;
  }
  using TSelfDict = DictObj<K, V>;
  using TKey = K;
  using TValue = V;
  using iterator = typename DictObj<K, V>::iterator;
  using reverse_iterator = typename DictObj<K, V>::reverse_iterator;
  using const_iterator = typename DictObj<K, V>::const_iterator;
  using const_reverse_iterator = typename DictObj<K, V>::const_reverse_iterator;

  MLC_INLINE Dict() : Dict(::mlc::base::AllocatorOf<TSelfDict>::New()) {}
  MLC_INLINE Dict(DictObj<K, V> *p) : UDict(p) {}
  MLC_INLINE Dict(std::initializer_list<std::pair<K, V>> init)
      : Dict(::mlc::base::AllocatorOf<TSelfDict>::New(std::move(init))) {}
  template <typename Iter>
  MLC_INLINE Dict(Iter begin, Iter end) : Dict(::mlc::base::AllocatorOf<TSelfDict>::New(begin, end)) {}
  MLC_INLINE int64_t size() const { return get()->size(); }
  MLC_INLINE bool empty() const { return get()->empty(); }
  MLC_INLINE void clear() { get()->clear(); }
  MLC_INLINE int64_t capacity() const { return get()->capacity(); }
  MLC_INLINE const V at(const K &key) const { return get()->at(key); }
  MLC_INLINE const V operator[](const K &key) const { return get()->operator[](key); }
  MLC_INLINE int64_t count(const K &key) const { return get()->count(key); }
  MLC_INLINE void Set(K key, V value) { get()->Set(key, value); }

  MLC_INLINE iterator find(const K &key) { return get()->find(key); }
  MLC_INLINE const_iterator find(const K &key) const { return get()->find(key); }
  MLC_INLINE void erase(const K &key) { get()->erase(key); }
  MLC_INLINE void erase(const iterator &it) { get()->erase(it); }
  MLC_INLINE void erase(const const_iterator &it) { get()->erase(it); }
  MLC_INLINE iterator begin() { return get()->begin(); }
  MLC_INLINE iterator end() { return get()->end(); }
  MLC_INLINE reverse_iterator rbegin() { return get()->rbegin(); }
  MLC_INLINE reverse_iterator rend() { return get()->rend(); }
  MLC_INLINE const_iterator begin() const { return get()->begin(); }
  MLC_INLINE const_iterator end() const { return get()->end(); }
  MLC_INLINE const_reverse_iterator rbegin() const { return get()->rbegin(); }
  MLC_INLINE const_reverse_iterator rend() const { return get()->rend(); }

  MLC_DEF_OBJ_REF(Dict, TSelfDict, UDict);
};

template <typename K, typename V> MLC_INLINE_NO_MSVC Dict<K, V> UDict::AsTyped() const {
  return Dict<K, V>(get()->AsTyped<K, V>());
}

template <typename K, typename V> MLC_INLINE_NO_MSVC DictObj<K, V> *UDictObj::AsTyped() const {
  UDictObj *self = const_cast<UDictObj *>(this);
  try {
    AnyView view(self);
    core::NestedTypeCheck<Dict<K, V>>::Run(view);
  } catch (core::NestedTypeError &e) {
    std::ostringstream os;
    e.Format(os, base::Type2Str<Dict<K, V>>::Run());
    MLC_THROW(NestedTypeError) << os.str();
  }
  return reinterpret_cast<DictObj<K, V> *>(self);
}
} // namespace mlc

#endif // MLC_CORE_DICT_H_
