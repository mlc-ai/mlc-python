#ifndef MLC_UDICT_H_
#define MLC_UDICT_H_
#include "./udict_base.h"
#include <iterator>
#include <mlc/ffi/core/core.h>

namespace mlc {
namespace ffi {

struct AnyHash {
  uint64_t operator()(const MLCAny &a) const {
    if (a.type_index == static_cast<int32_t>(MLCTypeIndex::kMLCStr)) {
      return details::StrHash(reinterpret_cast<MLCStr *>(a.v_obj));
    }
    union {
      int64_t i64;
      uint64_t u64;
    } cvt;
    cvt.i64 = a.v_int64;
    return cvt.u64;
  }
};

struct AnyEqual {
  bool operator()(const MLCAny &a, const MLCAny &b) const {
    if (a.type_index != b.type_index) {
      return false;
    }
    if (a.type_index == static_cast<int32_t>(MLCTypeIndex::kMLCStr)) {
      return details::StrCompare(reinterpret_cast<MLCStr *>(a.v_obj), reinterpret_cast<MLCStr *>(b.v_obj)) == 0;
    }
    return a.v_int64 == b.v_int64;
  }
};

struct UDictObj : protected DictBase {
  template <typename> friend struct DictBase::ffi;
  friend struct DictBase;
  using Hash = AnyHash;
  using Equal = AnyEqual;
  using MLCDict::_mlc_header;
  struct Iterator;
  struct ReverseIterator;

  MLC_INLINE ~UDictObj() = default;
  MLC_INLINE UDictObj(int64_t capacity) : DictBase(capacity) {}
  MLC_INLINE UDictObj() : DictBase() {}
  template <typename Iter> MLC_INLINE UDictObj(Iter begin, Iter end) : DictBase(std::distance(begin, end) * 2) {
    this->InsertRange<UDictObj>(begin, end);
  }
  MLC_INLINE Iterator find(const Any &key);
  MLC_INLINE Any &at(const Any &key) { return DictBase::at<Hash, Equal>(key); }
  MLC_INLINE const Any &at(const Any &key) const { return DictBase::at<Hash, Equal>(key); }
  MLC_INLINE Any &operator[](const Any &key) {
    return static_cast<Any &>(DictBase::InsertOrLookup<UDictObj>(key)->second);
  }
  MLC_INLINE const Any &operator[](const Any &key) const { return this->at(key); }
  MLC_INLINE int64_t size() const { return this->MLCDict::size; }
  MLC_INLINE bool empty() const { return this->MLCDict::size == 0; }
  MLC_INLINE int64_t count(const Any &key) const { return Lookup<Hash, Equal>(key).IsNone() ? 0 : 1; }
  MLC_INLINE void clear() { this->Clear(); }
  MLC_INLINE Iterator begin();
  MLC_INLINE Iterator end();
  MLC_INLINE ReverseIterator rbegin();
  MLC_INLINE ReverseIterator rend();
  MLC_INLINE void erase(const Any &key) { DictBase::Erase<Hash, Equal>(key); }
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

  MLC_DEF_STATIC_TYPE(UDictObj, Object, MLCTypeIndex::kMLCDict, "object.Dict")
      .FieldReadOnly("capacity", &MLCDict::capacity)
      .FieldReadOnly("size", &MLCDict::size)
      .FieldReadOnly("data", &MLCDict::data)
      .Method("__str__", &UDictObj::__str__)
      .Method("__init__", DictBase::ffi<UDictObj>::New)
      .Method("__getitem__", DictBase::ffi<UDictObj>::GetItem)
      .Method("__iter_get_key__", DictBase::ffi<UDictObj>::GetKey)
      .Method("__iter_get_value__", DictBase::ffi<UDictObj>::GetValue)
      .Method("__iter_advance__", DictBase::ffi<UDictObj>::Advance);
};

struct UDictObj::Iterator : public DictBaseIterator<UDictObj::Iterator> {
public:
  using pointer = std::pair<const Any, Any> *;
  using reference = std::pair<const Any, Any> &;
  MLC_INLINE reference operator*() const { return *(this->operator->()); }
  MLC_INLINE pointer operator->() const {
    return reinterpret_cast<pointer>(&DictBlockIter::FromIndex(self, this->index).Data());
  }

protected:
  template <typename> friend struct DictBase::ffi;
  using DictBaseIterator::DictBaseIterator;
  using DictBaseIterator::index;
  using DictBaseIterator::self;
};

MLC_INLINE UDictObj::Iterator UDictObj::find(const Any &key) {
  DictBlockIter iter = Lookup<Hash, Equal>(key);
  return iter.IsNone() ? end() : Iterator(iter.i, this);
}

struct UDictObj::ReverseIterator : public std::reverse_iterator<UDictObj::Iterator> {
  using std::reverse_iterator<UDictObj::Iterator>::reverse_iterator;
};

MLC_INLINE UDictObj::Iterator UDictObj::begin() { return Iterator(-1, this).operator++(); }
MLC_INLINE UDictObj::Iterator UDictObj::end() { return Iterator(this->Cap(), this); }
MLC_INLINE UDictObj::ReverseIterator UDictObj::rbegin() { return ReverseIterator(end()); }
MLC_INLINE UDictObj::ReverseIterator UDictObj::rend() { return ReverseIterator(begin()); }

struct UDict : public ObjectRef {
  using Iterator = UDictObj::Iterator;
  using ReverseIterator = UDictObj::ReverseIterator;
  using Hash = UDictObj::Hash;
  using Equal = UDictObj::Equal;

  MLC_INLINE UDict() : UDict(details::AllocatorOf<UDictObj>::New()) {}
  MLC_INLINE UDict(std::initializer_list<std::pair<Any, Any>> init)
      : UDict(details::AllocatorOf<UDictObj>::New(init.begin(), init.end())) {}
  template <typename Iter>
  MLC_INLINE UDict(Iter begin, Iter end) : UDict(details::AllocatorOf<UDictObj>::New(begin, end)) {}
  template <typename K, typename V> MLC_INLINE_NO_MSVC Dict<K, V> AsTyped() const;
  MLC_INLINE Iterator find(const Any &key) { return get()->find(key); }
  MLC_INLINE Any &at(const Any &key) { return get()->at(key); }
  MLC_INLINE const Any &at(const Any &key) const { return get()->at(key); }
  MLC_INLINE Any &operator[](const Any &key) { return get()->operator[](key); }
  MLC_INLINE const Any &operator[](const Any &key) const { return get()->operator[](key); }
  MLC_INLINE int64_t size() const { return get()->size(); }
  MLC_INLINE bool empty() const { return get()->empty(); }
  MLC_INLINE int64_t count(const Any &key) const { return get()->count(key); }
  MLC_INLINE void clear() { get()->clear(); }
  MLC_INLINE Iterator begin() { return get()->begin(); }
  MLC_INLINE Iterator end() { return get()->end(); }
  MLC_INLINE ReverseIterator rbegin() { return get()->rbegin(); }
  MLC_INLINE ReverseIterator rend() { return get()->rend(); }
  MLC_INLINE void erase(const Any &key) { get()->erase(key); }
  MLC_DEF_OBJ_REF(UDict, UDictObj, ObjectRef);
};

} // namespace ffi
} // namespace mlc
#endif // MLC_UDICT_H_
