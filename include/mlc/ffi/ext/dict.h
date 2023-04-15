#ifndef MLC_DICT_H_
#define MLC_DICT_H_
#include "./error.h"
#include "./udict.h"
#include <initializer_list>
#include <tuple>
#include <type_traits>

namespace mlc {
namespace ffi {

template <typename K, typename V> struct DictObj : protected UDictObj {
  static_assert(IsContainerElement<K>);
  static_assert(IsContainerElement<V>);
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

template <typename K, typename V> struct DictObj<K, V>::Iterator : public DictBaseIterator<DictObj<K, V>::Iterator> {
protected:
  using TParent = DictBaseIterator<DictObj<K, V>::Iterator>;
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
    std::pair<MLCAny, MLCAny> &pair = DictBlockIter::FromIndex(self, index).Data();
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
  static_assert(IsContainerElement<K>);
  static_assert(IsContainerElement<V>);
  using TSelfDict = DictObj<K, V>;
  using TKey = K;
  using TValue = V;
  using Iterator = typename DictObj<K, V>::Iterator;
  using ReverseIterator = typename DictObj<K, V>::ReverseIterator;

  MLC_INLINE Dict() : Dict(details::AllocatorOf<TSelfDict>::New()) {}
  MLC_INLINE Dict(std::initializer_list<std::pair<K, V>> init)
      : Dict(details::AllocatorOf<TSelfDict>::New(std::move(init))) {}
  template <typename Iter>
  MLC_INLINE Dict(Iter begin, Iter end) : Dict(details::AllocatorOf<TSelfDict>::New(begin, end)) {}
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

namespace details {
template <typename K, typename V> MLC_INLINE_NO_MSVC void NestedTypeCheck<Dict<K, V>>::Run(const MLCAny &any) {
  try {
    static_cast<const AnyView &>(any).Cast<UDict>();
  } catch (const Exception &e) {
    throw NestedTypeError(e.what()).NewFrame(Type2Str<UDict>::Run());
  }
  if constexpr (!std::is_same_v<K, Any> || !std::is_same_v<V, Any>) {
    DictBase *dict = reinterpret_cast<DictBase *>(any.v_obj);
    dict->IterateAll([](uint8_t *, MLCAny *key, MLCAny *value) {
      if constexpr (!std::is_same_v<K, Any>) {
        try {
          NestedTypeCheck<K>::Run(*key);
        } catch (NestedTypeError &e) {
          throw e.NewFrame(Type2Str<K>::Run());
        }
      }
      if constexpr (!std::is_same_v<V, Any>) {
        try {
          NestedTypeCheck<V>::Run(*value);
        } catch (NestedTypeError &e) {
          throw e.NewIndex(*static_cast<AnyView *>(key));
        }
      }
    });
  }
}
} // namespace details

template <typename K, typename V> MLC_INLINE_NO_MSVC Dict<K, V> UDict::AsTyped() const {
  return Dict<K, V>(get()->AsTyped<K, V>());
}

template <typename K, typename V> MLC_INLINE_NO_MSVC DictObj<K, V> *UDictObj::AsTyped() const {
  UDictObj *self = const_cast<UDictObj *>(this);
  try {
    AnyView view(self);
    details::NestedTypeCheck<Dict<K, V>>::Run(view);
  } catch (details::NestedTypeError &e) {
    std::ostringstream os;
    e.Format(os, Type2Str<Dict<K, V>>::Run());
    MLC_THROW(NestedTypeError) << os.str();
  }
  return reinterpret_cast<DictObj<K, V> *>(self);
}

template <typename K, typename V> struct ObjPtrTraits<DictObj<K, V>> {
  using T = DictObj<K, V>;
  MLC_INLINE static void PtrToAnyView(const T *v, MLCAny *ret) { ObjPtrTraitsDefault<T>::PtrToAnyView(v, ret); }
  MLC_INLINE static T *AnyToOwnedPtr(const MLCAny *v) { return AnyToUnownedPtr(v); }
  MLC_INLINE static T *AnyToUnownedPtr(const MLCAny *v) {
    return ObjPtrTraitsDefault<UDictObj>::AnyToUnownedPtr(v)->AsTyped<K, V>();
  }
};

} // namespace ffi
} // namespace mlc
#endif // MLC_UDICT_H_
