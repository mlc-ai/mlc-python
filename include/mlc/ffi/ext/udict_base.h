#ifndef MLC_UDICT_BASE_H_
#define MLC_UDICT_BASE_H_
#include <iterator>
#include <mlc/ffi/core/core.h>
#include <type_traits>
#include <unordered_map>

namespace mlc {
namespace ffi {

/**
 * Each block has a 8-byte key (MLCAny), 8-byte value (MLCAny), and a 1-byte metadata.
 *
 * Metadata can be one of the following three cases:
 * - 1) Empty: 0xFF (0b11111111)_2. The slot is available and can be written in.
 * - 2) Protected: 0xFE (0b11111110)_2. The slot is empty but not allowed to be written. It is
 *   only used during insertion when relocating certain elements.
 * - 3) Normal: (0bXYYYYYYY)_2. The highest bit `X` indicates if it is the head of a linked
 *   list, where `0` means head, and `1` means not head. The next 7 bits `YYYYYYY` are used as
 *   the "next pointer" (i.e. pointer to the next element),  where `kNextProbeLocation[YYYYYYY]`
 *   is the offset to the next element. And if `YYYYYYY == 0`, it means the end of the linked
 *   list.
 */
struct DictBlock {
  /* clang-format off */
  static inline const constexpr uint64_t kNextProbeLocation[] = {
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
      /* triangle numbers for quadratic probing */ 21, 28, 36, 45, 55, 66, 78, 91, 105, 120, 136, 153, 171, 190, 210, 231, 253, 276, 300, 325, 351, 378, 406, 435, 465, 496, 528, 561, 595, 630, 666, 703, 741, 780, 820, 861, 903, 946, 990, 1035, 1081, 1128, 1176, 1225, 1275, 1326, 1378, 1431, 1485, 1540, 1596, 1653, 1711, 1770, 1830, 1891, 1953, 2016, 2080, 2145, 2211, 2278, 2346, 2415, 2485, 2556, 2628,
      /* larger triangle numbers */ 8515, 19110, 42778, 96141, 216153, 486591, 1092981, 2458653, 5532801, 12442566, 27993903, 62983476, 141717030, 318844378, 717352503, 1614057336, 3631522476, 8170957530, 18384510628, 41364789378, 93070452520, 209408356380, 471168559170, 1060128894105, 2385289465695, 5366898840628, 12075518705635, 27169915244790, 61132312065111, 137547689707000, 309482283181501, 696335127828753, 1566753995631385, 3525196511162271, 7931691992677701, 17846306936293605, 40154190677507445, 90346928918121501, 203280589587557251, 457381325854679626, 1029107982097042876, 2315492959180353330, 5209859154120846435,
  };
  /* clang-format on */
  static constexpr int32_t kBlockCapacity = 16;
  static constexpr uint8_t kNumProbe = std::size(kNextProbeLocation);
  static constexpr uint8_t kEmptySlot = uint8_t(0b11111111);
  static constexpr uint8_t kProtectedSlot = uint8_t(0b11111110);
  static constexpr uint64_t kMinSize = 7;
  static constexpr uint8_t kNewHead = 0b00000000;
  static constexpr uint8_t kNewTail = 0b10000000;
  using KVPair = std::pair<MLCAny, MLCAny>;

  uint8_t meta[kBlockCapacity];
  KVPair data[kBlockCapacity];
};
static_assert(sizeof(DictBlock) == DictBlock::kBlockCapacity * (1 + sizeof(MLCAny) * 2), "ABI check");
static_assert(std::is_aggregate_v<DictBlock>, "ABI check");
static_assert(std::is_trivially_destructible_v<DictBlock>, "ABI check");
static_assert(std::is_standard_layout_v<DictBlock>, "ABI check");
static_assert(DictBlock::kNumProbe == 126, "Check assumption");

struct DictBlockIter {
  MLC_INLINE static DictBlockIter None() { return DictBlockIter(0, nullptr); }
  MLC_INLINE static DictBlockIter FromIndex(const MLCDict *self, uint64_t i) {
    return DictBlockIter(i, static_cast<DictBlock *>(self->data) + (i / DictBlock::kBlockCapacity));
  }
  MLC_INLINE static DictBlockIter FromHash(const MLCDict *self, uint64_t h) {
    return DictBlockIter::FromIndex(self,
                                    (11400714819323198485ull * h) >> (details::CountLeadingZeros(self->capacity) + 1));
  }
  MLC_INLINE auto &Data() const { return cur->data[i % DictBlock::kBlockCapacity]; }
  MLC_INLINE uint8_t &Meta() const { return cur->meta[i % DictBlock::kBlockCapacity]; }
  MLC_INLINE uint64_t Offset() const { return DictBlock::kNextProbeLocation[Meta() & 0b01111111]; }
  MLC_INLINE bool IsHead() const { return (Meta() & 0b10000000) == 0b00000000; }
  MLC_INLINE void SetNext(uint8_t jump) const { (Meta() &= 0b10000000) |= jump; }
  MLC_INLINE void Advance(const MLCDict *self) { *this = WithOffset(self, Offset()); }
  MLC_INLINE bool IsNone() const { return cur == nullptr; }
  MLC_INLINE DictBlockIter WithOffset(const MLCDict *self, uint64_t offset) const {
    return offset == 0 ? DictBlockIter::None() : DictBlockIter::FromIndex(self, (i + offset) & (self->capacity - 1));
  }
  MLC_INLINE DictBlockIter() = default;
  MLC_INLINE explicit DictBlockIter(uint64_t i, DictBlock *cur) : i(i), cur(cur) {}

  uint64_t i;
  DictBlock *cur;
};

struct DictBase : public MLCDict {
  template <typename SubObject> struct ffi;

  MLC_INLINE DictBase() : MLCDict() {}
  MLC_INLINE ~DictBase() {
    details::PODArrayFinally finally{this->MLCDict::data};
    this->Clear();
  }

  MLC_INLINE explicit DictBase(int64_t capacity) : MLCDict() {
    if (capacity == 0) {
      return;
    }
    capacity = (capacity + DictBlock::kBlockCapacity - 1) & ~(DictBlock::kBlockCapacity - 1);
    capacity = std::max<int64_t>(capacity, DictBlock::kBlockCapacity);
    capacity = details::BitCeil(capacity);
    if ((capacity & (capacity - 1)) != 0 || capacity % DictBlock::kBlockCapacity != 0) {
      MLC_THROW(InternalError) << "Invalid capacity: " << capacity;
    }
    int64_t num_blocks = capacity / DictBlock::kBlockCapacity;
    this->MLCDict::capacity = capacity;
    this->MLCDict::size = 0;
    this->MLCDict::data = details::PODArrayCreate<DictBlock>(num_blocks).release();
    DictBlock *blocks = this->Blocks();
    for (int64_t i = 0; i < num_blocks; ++i) {
      std::memset(blocks[i].meta, DictBlock::kEmptySlot, sizeof(blocks[i].meta));
    }
  }

  template <typename SubObject, typename Iter> void InsertRange(Iter begin, Iter end) {
    for (; begin != end; ++begin) {
      Any key((*begin).first);
      Any value((*begin).second);
      static_cast<Any &>(this->InsertOrLookup<SubObject>(key)->second) = value;
    }
  }

  template <typename SubObject> Ref<SubObject> WithCapacity(int64_t new_cap) {
    Ref<SubObject> dict = Ref<SubObject>::New(new_cap);
    DictBase *dict_ptr = dict.get();
    this->IterateAll([&](uint8_t *, MLCAny *key, MLCAny *value) {
      DictBlock::KVPair *ret = dict_ptr->InsertOrLookup<SubObject>(*static_cast<Any *>(key));
      static_cast<Any &>(ret->second) = *static_cast<Any *>(value);
    });
    return dict;
  }

  template <typename SubObject> DictBlock::KVPair *InsertOrLookup(Any key) {
    using Hash = typename SubObject::Hash;
    using Equal = typename SubObject::Equal;
    DictBlock::KVPair *ret;
    while (!(ret = this->TryInsertOrLookup<Hash, Equal>(&key))) {
      int64_t new_cap = this->MLCDict::capacity == 0 ? DictBlock::kBlockCapacity : this->MLCDict::capacity * 2;
      Ref<SubObject> larger_dict = this->WithCapacity<SubObject>(new_cap);
      this->Swap(larger_dict.get());
    }
    return ret;
  }

  template <typename Hash, typename Equal> MLC_INLINE Any &at(const Any &key) {
    DictBlockIter iter = this->Lookup<Hash, Equal>(key);
    if (iter.IsNone()) {
      MLC_THROW(KeyError) << key;
    }
    return static_cast<Any &>(iter.Data().second);
  }

  template <typename Hash, typename Equal> MLC_INLINE const Any &at(const Any &key) const {
    DictBlockIter iter = this->Lookup<Hash, Equal>(key);
    if (iter.IsNone()) {
      MLC_THROW(KeyError) << key;
    }
    return static_cast<const Any &>(iter.Data().second);
  }

  template <typename Hash, typename Equal> MLC_INLINE void Erase(const Any &key) {
    DictBlockIter iter = Lookup<Hash, Equal>(key);
    if (!iter.IsNone()) {
      Erase<Hash>(iter.i);
    } else {
      MLC_THROW(KeyError) << key;
    }
  }

  MLC_INLINE void Swap(DictBase *other) {
    MLCDict tmp = *static_cast<MLCDict *>(other);
    *static_cast<MLCDict *>(other) = *static_cast<MLCDict *>(this);
    *static_cast<MLCDict *>(this) = tmp;
  }

  MLC_INLINE DictBlockIter Head(uint64_t hash) const {
    DictBlockIter iter = DictBlockIter::FromHash(this, hash);
    return iter.IsHead() ? iter : DictBlockIter::None();
  }

  template <typename Hash> DictBlockIter Prev(DictBlockIter iter) const {
    DictBlockIter prev = Head(Hash()(iter.Data().first));
    DictBlockIter next = prev;
    for (next.Advance(this); next.i != iter.i; prev = next, next.Advance(this)) {
    }
    return prev;
  }

  MLC_INLINE uint64_t Cap() const { return static_cast<uint64_t>(this->MLCDict::capacity); }
  MLC_INLINE uint64_t Size() const { return this->MLCDict::size; }
  MLC_INLINE DictBlock *Blocks() const { return static_cast<DictBlock *>(this->data); }

  DictBlockIter Probe(DictBlockIter cur, uint8_t *result) const {
    for (uint8_t i = 1; i < DictBlock::kNumProbe && DictBlock::kNextProbeLocation[i] < this->Size(); ++i) {
      DictBlockIter next = cur.WithOffset(this, DictBlock::kNextProbeLocation[i]);
      if (next.Meta() == DictBlock::kEmptySlot) {
        *result = i;
        return next;
      }
    }
    *result = 0;
    return DictBlockIter::None();
  }

  template <typename Pred> void IterateAll(Pred pred) const {
    DictBlock *blocks_ = this->Blocks();
    int64_t num_blocks = this->MLCDict::capacity / DictBlock::kBlockCapacity;
    for (int64_t i = 0; i < num_blocks; ++i) {
      for (int j = 0; j < DictBlock::kBlockCapacity; ++j) {
        uint8_t &meta = blocks_[i].meta[j];
        auto &kv = blocks_[i].data[j];
        if (meta != DictBlock::kEmptySlot && meta != DictBlock::kProtectedSlot) {
          pred(&meta, &kv.first, &kv.second);
        }
      }
    }
  }

  void Clear() {
    this->IterateAll([](uint8_t *meta, MLCAny *key, MLCAny *value) {
      static_cast<Any *>(key)->Reset();
      static_cast<Any *>(value)->Reset();
      *meta = DictBlock::kEmptySlot;
    });
    this->MLCDict::size = 0;
  }

  template <typename Hash, typename Equal> DictBlockIter Lookup(const MLCAny &key) const {
    uint64_t hash = Hash()(key);
    for (DictBlockIter iter = Head(hash); !iter.IsNone(); iter.Advance(this)) {
      if (Equal()(key, iter.Data().first)) {
        return iter;
      }
    }
    return DictBlockIter::None();
  }

  template <typename Hash> void Erase(int64_t index) {
    DictBlockIter iter = DictBlockIter::FromIndex(this, index);
    if (uint64_t offset = iter.Offset(); offset != 0) {
      DictBlockIter prev = iter;
      DictBlockIter next = iter.WithOffset(this, offset);
      while ((offset = next.Offset()) != 0) {
        prev = next;
        next = next.WithOffset(this, offset);
      }
      auto &kv = iter.Data();
      static_cast<Any &>(kv.first).Reset();
      static_cast<Any &>(kv.second).Reset();
      kv = next.Data();
      next.Meta() = DictBlock::kEmptySlot;
      prev.SetNext(0);
    } else {
      if (!iter.IsHead()) {
        Prev<Hash>(iter).SetNext(0);
      }
      iter.Meta() = DictBlock::kEmptySlot;
      auto &kv = iter.Data();
      static_cast<Any &>(kv.first).Reset();
      static_cast<Any &>(kv.second).Reset();
    }
    this->MLCDict::size -= 1;
  }

  template <typename Hash, typename Equal> DictBlock::KVPair *TryInsertOrLookup(MLCAny *key) {
    if (Cap() == Size() || Size() + 1 > Cap() * 0.99) {
      return nullptr;
    }
    // `iter` starts from the head of the linked list
    DictBlockIter iter = DictBlockIter::FromHash(this, Hash()(*key));
    uint8_t new_meta = DictBlock::kNewHead;
    // There are three cases over all:
    // 1) available - `iter` points to an empty slot that we could directly write in;
    // 2) hit - `iter` points to the head of the linked list which we want to iterate through
    // 3) relocate - `iter` points to the body of a different linked list, and in this case, we
    // will have to relocate the elements to make space for the new element
    if (iter.Meta() == DictBlock::kEmptySlot) { // (Case 1) available
      // Do nothing
    } else if (iter.IsHead()) { // (Case 2) hit
      // Point `iter` to the last element of the linked list
      for (DictBlockIter prev = iter;; prev = iter) {
        if (Equal()(*key, iter.Data().first)) {
          return &iter.Data();
        }
        iter.Advance(this);
        if (iter.IsNone()) {
          iter = prev;
          break;
        }
      }
      // Prob at the end of the linked list for the next empty slot
      DictBlockIter prev = iter;
      uint8_t jump;
      iter = Probe(iter, &jump);
      if (jump == 0) {
        return nullptr;
      }
      prev.SetNext(jump);
      new_meta = DictBlock::kNewTail;
    } else {
      // (Case 3) relocate: Chop the list starting from `iter`, and move it to other locations.
      DictBlockIter next = iter, prev = Prev<Hash>(iter);
      // Loop invariant:
      // - `next` points to the first element of the list we are going to relocate.
      // - `prev` points to the last element of the list that we are relocating to.
      for (uint8_t next_meta = DictBlock::kProtectedSlot; !next.IsNone(); next_meta = DictBlock::kEmptySlot) {
        // Step 1. Prob for the next empty slot `new_next` after `prev`
        uint8_t jump;
        DictBlockIter new_next = Probe(prev, &jump);
        if (jump == 0) {
          return nullptr;
        }
        // Step 2. Relocate `next` to `new_next`
        new_next.Meta() = DictBlock::kNewTail;
        new_next.Data() = {next.Data().first, next.Data().second};
        std::swap(next_meta, next.Meta());
        prev.SetNext(jump);
        // Step 3. Update `prev` -> `new_next`, `next` -> `Advance(next)`
        prev = new_next;
        next = next.WithOffset(this, DictBlock::kNextProbeLocation[next_meta & 0b01111111]);
      }
    }
    this->MLCDict::size += 1;
    iter.Meta() = new_meta;
    auto &kv = iter.Data() = {*key, MLCAny()};
    key->type_index = 0;
    key->v_int64 = 0;
    return &kv;
  }
};

template <typename SubCls> struct DictBaseIterator {
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = std::pair<const Any, Any>;
  static_assert(sizeof(value_type) == sizeof(DictBlock::KVPair), "ABI check");
  static_assert(offsetof(value_type, first) == offsetof(DictBlock::KVPair, first), "ABI check");
  static_assert(offsetof(value_type, second) == offsetof(DictBlock::KVPair, second), "ABI check");

  MLC_INLINE DictBaseIterator() = default;
  MLC_INLINE DictBaseIterator(const DictBaseIterator &) = default;
  MLC_INLINE DictBaseIterator(DictBaseIterator &&) = default;
  MLC_INLINE DictBaseIterator &operator=(const DictBaseIterator &) = default;
  MLC_INLINE DictBaseIterator &operator=(DictBaseIterator &&) = default;

  SubCls &operator++() {
    int64_t cap = self->Cap();
    while (++index < cap) {
      if (DictBlockIter::FromIndex(self, index).Meta() != DictBlock::kEmptySlot) {
        return Self();
      }
    }
    return Self();
  }
  SubCls &operator--() {
    while (--index >= 0) {
      if (DictBlockIter::FromIndex(self, index).Meta() != DictBlock::kEmptySlot) {
        return Self();
      }
    }
    return Self();
  }
  MLC_INLINE SubCls operator++(int) {
    SubCls copy(*this);
    this->operator++();
    return copy;
  }
  MLC_INLINE SubCls operator--(int) {
    SubCls copy(*this);
    this->operator--();
    return copy;
  }
  MLC_INLINE bool operator==(const DictBaseIterator &other) const { return index == other.index; }
  MLC_INLINE bool operator!=(const DictBaseIterator &other) const { return index != other.index; }
  MLC_INLINE DictBaseIterator(int64_t index, const DictBase *self) : index(index), self(self) {}

  int64_t index;
  const DictBase *self;

private:
  SubCls &Self() { return *static_cast<SubCls *>(this); }
};

template <typename SubObject> struct DictBase::ffi {
  using Iterator = typename SubObject::Iterator;
  static void New(int32_t num_args, const AnyView *args, Any *any_ret) {
    Ref<SubObject> ret = Ref<SubObject>::New(num_args * 2);
    DictBase *dict_ptr = ret.get();
    for (auto it = args; it != args + num_args; it += 2) {
      static_cast<Any &>(dict_ptr->InsertOrLookup<SubObject>(Any(*it))->second) = Any(*(it + 1));
    }
    *any_ret = std::move(ret);
  }
  static Any GetItem(SubObject *self, Any key) { return self->at(key); }
  static Any GetKey(SubObject *self, int64_t i) { return Iterator(i, reinterpret_cast<DictBase *>(self))->first; }
  static Any GetValue(SubObject *self, int64_t i) { return Iterator(i, reinterpret_cast<DictBase *>(self))->second; }
  static int64_t Advance(SubObject *self, int64_t i) {
    Iterator iter(i, reinterpret_cast<DictBase *>(self));
    ++iter;
    return iter.index;
  }
};

} // namespace ffi
} // namespace mlc
#endif // MLC_UDICT_BASE_H_
