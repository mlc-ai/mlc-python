#ifndef MLC_CORE_DICT_H_
#define MLC_CORE_DICT_H_
#include "./error.h"
#include "./udict.h"
#include <initializer_list>
#include <tuple>
#include <type_traits>

namespace mlc {
namespace base {
template <typename K, typename V> struct TypeTraits<DictObj<K, V> *> {
  using T = DictObj<K, V>;
  MLC_INLINE static void TypeToAny(T *src, MLCAny *ret) { ObjPtrTraitsDefault<UDictObj>::TypeToAny(src, ret); }
  MLC_INLINE static T *AnyToTypeOwned(const MLCAny *v) { return AnyToTypeUnowned(v); }
  MLC_INLINE static T *AnyToTypeUnowned(const MLCAny *v) {
    return ObjPtrTraitsDefault<UDictObj>::AnyToTypeUnowned(v)->AsTyped<K, V>();
  }
};
} // namespace base
} // namespace mlc

namespace mlc {
template <typename K, typename V> struct DictObj : protected UDictObj {
  static_assert(::mlc::base::IsContainerElement<K>);
  static_assert(::mlc::base::IsContainerElement<V>);
  using TKey = K;
  using TValue = V;
  using UDictObj::_mlc_header;
  using UDictObj::_type_ancestors;
  using UDictObj::_type_depth;
  using UDictObj::_type_index;
  using UDictObj::_type_info;
  using UDictObj::_type_key;
  using UDictObj::_type_parent;
  using UDictObj::capacity;
  using UDictObj::clear;
  using UDictObj::empty;
  using UDictObj::size;

  struct Iterator;
  struct ReverseIterator;

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
  MLC_INLINE Iterator find(const K &key) { return UDictObj::find(key); }
  MLC_INLINE const V at(const K &key) { return UDictObj::at(key).template Cast<V>(); }
  MLC_INLINE const V operator[](const K &key) { return UDictObj::operator[](key).template Cast<V>(); }
  MLC_INLINE int64_t count(const K &key) const { return UDictObj::count(key); }
  MLC_INLINE void Set(K key, V value) { UDictObj::operator[](key) = value; }
  MLC_INLINE Iterator begin();
  MLC_INLINE Iterator end();
  MLC_INLINE ReverseIterator rbegin();
  MLC_INLINE ReverseIterator rend();
  MLC_INLINE void erase(const K &key) { DictBase::Erase<Hash, Equal>(key); }
};

template <typename K, typename V>
struct DictObj<K, V>::Iterator : public core::DictBaseIterator<DictObj<K, V>::Iterator> {
protected:
  using TParent = core::DictBaseIterator<DictObj<K, V>::Iterator>;
  using TParent::index;
  using TParent::self;
  using TParent::TParent;

public:
  using pointer = void;
  using reference = const std::pair<K, V>;
  using difference_type = typename TParent::difference_type;
  Iterator(UDictObj::Iterator src) {
    TParent *parent = reinterpret_cast<TParent *>(&src);
    this->self = parent->self;
    this->index = parent->index;
  }

  MLC_INLINE reference operator*() const {
    std::pair<MLCAny, MLCAny> &pair = core::DictBlockIter::FromIndex(self, index).Data();
    return std::pair<K, V>(static_cast<Any &>(pair.first).template Cast<K>(),
                           static_cast<Any &>(pair.second).template Cast<V>());
  }
  pointer operator->() const = delete; // Use `operator*` instead
};

template <typename K, typename V>
struct DictObj<K, V>::ReverseIterator : public std::reverse_iterator<DictObj<K, V>::Iterator> {
  using std::reverse_iterator<DictObj<K, V>::Iterator>::reverse_iterator;
};

/* clang-format off */
template <typename K, typename V> MLC_INLINE auto DictObj<K, V>::begin() -> DictObj<K, V>::Iterator { return Iterator(-1, this).operator++(); }
template <typename K, typename V> MLC_INLINE auto DictObj<K, V>::end() -> DictObj<K, V>::Iterator { return Iterator(this->Cap(), this); }
template <typename K, typename V> MLC_INLINE auto DictObj<K, V>::rbegin() -> DictObj<K, V>::ReverseIterator { return ReverseIterator(end()); }
template <typename K, typename V> MLC_INLINE auto DictObj<K, V>::rend() -> DictObj<K, V>::ReverseIterator { return ReverseIterator(begin()); }
/* clang-format on */

template <typename K, typename V> struct Dict : protected UDict {
  static_assert(::mlc::base::IsContainerElement<K>);
  static_assert(::mlc::base::IsContainerElement<V>);
  using TSelfDict = DictObj<K, V>;
  using TKey = K;
  using TValue = V;
  using Iterator = typename DictObj<K, V>::Iterator;
  using ReverseIterator = typename DictObj<K, V>::ReverseIterator;

  MLC_INLINE Dict() : Dict(::mlc::base::AllocatorOf<TSelfDict>::New()) {}
  MLC_INLINE Dict(std::initializer_list<std::pair<K, V>> init)
      : Dict(::mlc::base::AllocatorOf<TSelfDict>::New(std::move(init))) {}
  template <typename Iter>
  MLC_INLINE Dict(Iter begin, Iter end) : Dict(::mlc::base::AllocatorOf<TSelfDict>::New(begin, end)) {}
  MLC_INLINE int64_t size() const { return get()->size(); }
  MLC_INLINE bool empty() const { return get()->empty(); }
  MLC_INLINE void clear() { get()->clear(); }
  MLC_INLINE int64_t capacity() const { return get()->capacity(); }
  MLC_INLINE Iterator find(const K &key) { return get()->find(key); }
  MLC_INLINE const V at(const K &key) { return get()->at(key); }
  MLC_INLINE const V operator[](const K &key) { return get()->operator[](key); }
  MLC_INLINE int64_t count(const K &key) const { return get()->count(key); }
  MLC_INLINE void Set(K key, V value) { get()->Set(key, value); }
  MLC_INLINE Iterator begin() { return get()->begin(); }
  MLC_INLINE Iterator end() { return get()->end(); }
  MLC_INLINE ReverseIterator rbegin() { return get()->rbegin(); }
  MLC_INLINE ReverseIterator rend() { return get()->rend(); }
  MLC_INLINE void erase(const K &key) { get()->erase(key); }
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
