#ifndef MLC_CORE_DICT_BASE_H_
#define MLC_CORE_DICT_BASE_H_
#include <iterator>
#include <mlc/base/all.h>
#include <type_traits>

namespace mlc {
namespace core {

struct DictBase : public MLCDict {
  using KVPair = std::pair<MLCAny, MLCAny>;
  using ProxyKVPair = std::pair<const Any, Any>;
  struct BlockIter;
  struct Block;
  inline DictBase() : MLCDict() {}
  inline explicit DictBase(int64_t capacity);
  inline ~DictBase() {
    ::mlc::base::PODArrayFinally finally{this->MLCDict::data};
    this->Clear();
  }
  inline void Clear();
  inline BlockIter Probe(BlockIter cur, uint8_t *result) const;
  template <typename Pred> inline void IterateAll(Pred pred) const;
  MLC_INLINE BlockIter Head(uint64_t hash) const;
  MLC_INLINE uint64_t Cap() const { return static_cast<uint64_t>(this->MLCDict::capacity); }
  MLC_INLINE uint64_t Size() const { return this->MLCDict::size; }
  MLC_INLINE Block *Blocks() const { return static_cast<Block *>(this->data); }
  MLC_INLINE void Swap(DictBase *other) {
    MLCDict tmp = *static_cast<MLCDict *>(other);
    *static_cast<MLCDict *>(other) = *static_cast<MLCDict *>(this);
    *static_cast<MLCDict *>(this) = tmp;
  }

  template <typename TDictObj> struct Accessor {
    using TSelf = Accessor<TDictObj>;
    MLC_INLINE static uint64_t Hash(const MLCAny &a) { return TDictObj::Hash(a); }
    MLC_INLINE static bool Equal(const MLCAny &a, const MLCAny &b) { return TDictObj::Equal(a, b); }
    template <typename Iter> inline static void InsertRange(TDictObj *self, Iter begin, Iter end);
    inline static void WithCapacity(TDictObj *self, int64_t new_cap);
    inline static KVPair *InsertOrLookup(TDictObj *self, Any key);
    inline static KVPair *TryInsertOrLookup(TDictObj *self, MLCAny *key);
    inline static void Erase(TDictObj *self, const Any &key);
    inline static void Erase(TDictObj *self, int64_t index);
    inline static Any &At(TDictObj *self, const Any &key);
    inline static const Any &At(const TDictObj *self, const Any &key);
    inline static Any &Bracket(TDictObj *self, const Any &key) {
      return static_cast<Any &>(TSelf::InsertOrLookup(self, key)->second);
    }
    inline static const Any &Bracket(const TDictObj *self, const Any &key) { return At(self, key); }
    inline static BlockIter Lookup(const TDictObj *self, const MLCAny &key);
    inline static int64_t Find(const TDictObj *self, const MLCAny &key);
    inline static BlockIter Prev(const TDictObj *self, BlockIter iter);
    inline static void New(int32_t num_args, const AnyView *args, Any *any_ret);
    inline static Any GetItem(TDictObj *self, Any key) { return self->at(key); }
    inline static Any GetKey(TDictObj *self, int64_t i) { return IterStateMut{self, i}.At().first; }
    inline static Any GetValue(TDictObj *self, int64_t i) { return IterStateMut{self, i}.At().second; }
    inline static int64_t Advance(TDictObj *self, int64_t i) { return IterStateMut{self, i}.Add().i; }
  };

  template <bool is_const> struct IterState {
    using TSelf = IterState<is_const>;
    using TDictPtr = std::conditional_t<is_const, const DictBase *, DictBase *>;
    using TValue = std::conditional_t<is_const, const ProxyKVPair &, ProxyKVPair &>;
    using TValuePtr = std::conditional_t<is_const, const KVPair *, KVPair *>;
    TDictPtr self;
    int64_t i;
    MLC_INLINE TSelf Add() const;
    MLC_INLINE TSelf Sub() const;
    MLC_INLINE TValue At() const;
    MLC_INLINE TValuePtr Ptr() const;
  };
  using IterStateMut = IterState<false>;
  using IterStateConst = IterState<true>;
  // clang-format off
  MLC_INLINE IterStateConst Begin() const { IterStateConst state{this, -1}; return state.Add(); }
  MLC_INLINE IterStateConst End() const { return IterStateConst{this, this->MLCDict::capacity}; }
  MLC_INLINE IterStateMut Begin() {  IterStateMut state{this, -1}; return state.Add(); }
  MLC_INLINE IterStateMut End() { return IterStateMut{this, this->MLCDict::capacity}; }
  // clang-format on

  static constexpr int32_t kBlockCapacity = 16;
  static constexpr uint8_t kEmptySlot = uint8_t(0b11111111);
  static constexpr uint8_t kProtectedSlot = uint8_t(0b11111110);
  static constexpr uint8_t kNewHead = 0b00000000;
  static constexpr uint8_t kNewTail = 0b10000000;
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
  struct Block {
    uint8_t meta[kBlockCapacity];
    KVPair data[kBlockCapacity];
  };
};

struct DictBase::BlockIter {
  /* clang-format off */
  static inline const constexpr uint64_t kNextProbeLocation[] = {
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
      /* triangle numbers for quadratic probing */ 21, 28, 36, 45, 55, 66, 78, 91, 105, 120, 136, 153, 171, 190, 210, 231, 253, 276, 300, 325, 351, 378, 406, 435, 465, 496, 528, 561, 595, 630, 666, 703, 741, 780, 820, 861, 903, 946, 990, 1035, 1081, 1128, 1176, 1225, 1275, 1326, 1378, 1431, 1485, 1540, 1596, 1653, 1711, 1770, 1830, 1891, 1953, 2016, 2080, 2145, 2211, 2278, 2346, 2415, 2485, 2556, 2628,
      /* larger triangle numbers */ 8515, 19110, 42778, 96141, 216153, 486591, 1092981, 2458653, 5532801, 12442566, 27993903, 62983476, 141717030, 318844378, 717352503, 1614057336, 3631522476, 8170957530, 18384510628, 41364789378, 93070452520, 209408356380, 471168559170, 1060128894105, 2385289465695, 5366898840628, 12075518705635, 27169915244790, 61132312065111, 137547689707000, 309482283181501, 696335127828753, 1566753995631385, 3525196511162271, 7931691992677701, 17846306936293605, 40154190677507445, 90346928918121501, 203280589587557251, 457381325854679626, 1029107982097042876, 2315492959180353330, 5209859154120846435,
  };
  /* clang-format on */

  MLC_INLINE static BlockIter None() { return BlockIter(0, nullptr); }
  MLC_INLINE static BlockIter FromIndex(const MLCDict *self, uint64_t i) {
    return BlockIter(i, static_cast<Block *>(self->data) + (i / DictBase::kBlockCapacity));
  }
  MLC_INLINE static BlockIter FromHash(const MLCDict *self, uint64_t h) {
    return BlockIter::FromIndex(self,
                                (11400714819323198485ull * h) >> (::mlc::base::CountLeadingZeros(self->capacity) + 1));
  }
  MLC_INLINE auto &Data() const { return cur->data[i % DictBase::kBlockCapacity]; }
  MLC_INLINE uint8_t &Meta() const { return cur->meta[i % DictBase::kBlockCapacity]; }
  MLC_INLINE uint64_t Offset() const { return kNextProbeLocation[Meta() & 0b01111111]; }
  MLC_INLINE bool IsHead() const { return (Meta() & 0b10000000) == 0b00000000; }
  MLC_INLINE void SetNext(uint8_t jump) const { (Meta() &= 0b10000000) |= jump; }
  MLC_INLINE void Advance(const MLCDict *self) { *this = WithOffset(self, Offset()); }
  MLC_INLINE bool IsNone() const { return cur == nullptr; }
  MLC_INLINE BlockIter WithOffset(const MLCDict *self, uint64_t offset) const {
    return offset == 0 ? BlockIter::None() : BlockIter::FromIndex(self, (i + offset) & (self->capacity - 1));
  }
  MLC_INLINE BlockIter() = default;
  MLC_INLINE explicit BlockIter(uint64_t i, Block *cur) : i(i), cur(cur) {}

  uint64_t i;
  Block *cur;
};

inline DictBase::DictBase(int64_t capacity) : MLCDict() {
  if (capacity == 0) {
    return;
  }
  capacity = (capacity + DictBase::kBlockCapacity - 1) & ~(DictBase::kBlockCapacity - 1);
  capacity = std::max<int64_t>(capacity, DictBase::kBlockCapacity);
  capacity = ::mlc::base::BitCeil(capacity);
  if ((capacity & (capacity - 1)) != 0 || capacity % DictBase::kBlockCapacity != 0) {
    MLC_THROW(InternalError) << "Invalid capacity: " << capacity;
  }
  int64_t num_blocks = capacity / DictBase::kBlockCapacity;
  this->MLCDict::capacity = capacity;
  this->MLCDict::size = 0;
  this->MLCDict::data = ::mlc::base::PODArrayCreate<Block>(num_blocks).release();
  Block *blocks = this->Blocks();
  for (int64_t i = 0; i < num_blocks; ++i) {
    std::memset(blocks[i].meta, DictBase::kEmptySlot, sizeof(blocks[i].meta));
  }
}

MLC_INLINE DictBase::BlockIter DictBase::Head(uint64_t hash) const {
  BlockIter iter = BlockIter::FromHash(this, hash);
  return iter.IsHead() ? iter : BlockIter::None();
}

inline DictBase::BlockIter DictBase::Probe(BlockIter cur, uint8_t *result) const {
  static constexpr uint8_t kNumProbe = std::size(BlockIter::kNextProbeLocation);
  static_assert(kNumProbe == 126, "Check assumption");
  for (uint8_t i = 1; i < kNumProbe && BlockIter::kNextProbeLocation[i] < this->Size(); ++i) {
    BlockIter next = cur.WithOffset(this, BlockIter::kNextProbeLocation[i]);
    if (next.Meta() == DictBase::kEmptySlot) {
      *result = i;
      return next;
    }
  }
  *result = 0;
  return BlockIter::None();
}

template <typename Pred> //
inline void DictBase::IterateAll(Pred pred) const {
  Block *blocks_ = this->Blocks();
  int64_t num_blocks = this->MLCDict::capacity / DictBase::kBlockCapacity;
  for (int64_t i = 0; i < num_blocks; ++i) {
    for (int j = 0; j < DictBase::kBlockCapacity; ++j) {
      uint8_t &meta = blocks_[i].meta[j];
      auto &kv = blocks_[i].data[j];
      if (meta != DictBase::kEmptySlot && meta != DictBase::kProtectedSlot) {
        pred(&meta, &kv.first, &kv.second);
      }
    }
  }
}

inline void DictBase::Clear() {
  this->IterateAll([](uint8_t *meta, MLCAny *key, MLCAny *value) {
    static_cast<Any *>(key)->Reset();
    static_cast<Any *>(value)->Reset();
    *meta = DictBase::kEmptySlot;
  });
  this->MLCDict::size = 0;
}

template <typename TDictObj>
template <typename Iter>
inline void DictBase::Accessor<TDictObj>::InsertRange(TDictObj *self, Iter begin, Iter end) {
  for (; begin != end; ++begin) {
    Any key((*begin).first);
    Any value((*begin).second);
    static_cast<Any &>(InsertOrLookup(self, key)->second) = value;
  }
}

template <typename TDictObj> //
inline void DictBase::Accessor<TDictObj>::WithCapacity(TDictObj *self, int64_t new_cap) {
  Ref<TDictObj> dict = Ref<TDictObj>::New(new_cap);
  self->IterateAll([dict_ptr = dict.get()](uint8_t *, MLCAny *key, MLCAny *value) {
    KVPair *ret = TSelf::InsertOrLookup(dict_ptr, *static_cast<Any *>(key));
    static_cast<Any &>(ret->second) = *static_cast<Any *>(value);
  });
  self->Swap(dict.get());
}

template <typename TDictObj> //
inline DictBase::KVPair *DictBase::Accessor<TDictObj>::InsertOrLookup(TDictObj *self, Any key) {
  KVPair *ret;
  while (!(ret = TSelf::TryInsertOrLookup(self, &key))) {
    int64_t new_cap = self->MLCDict::capacity == 0 ? DictBase::kBlockCapacity : self->MLCDict::capacity * 2;
    WithCapacity(self, new_cap);
  }
  return ret;
}

template <typename TDictObj> //
inline DictBase::KVPair *DictBase::Accessor<TDictObj>::TryInsertOrLookup(TDictObj *self, MLCAny *key) {
  DictBase *self_base = static_cast<DictBase *>(self);
  if (self_base->Cap() == self_base->Size() || self_base->Size() + 1 > self_base->Cap() * 0.99) {
    return nullptr;
  }
  // `iter` starts from the head of the linked list
  BlockIter iter = BlockIter::FromHash(self_base, Hash(*key));
  uint8_t new_meta = DictBase::kNewHead;
  // There are three cases over all:
  // 1) available - `iter` points to an empty slot that we could directly write in;
  // 2) hit - `iter` points to the head of the linked list which we want to iterate through
  // 3) relocate - `iter` points to the body of a different linked list, and in this case, we
  // will have to relocate the elements to make space for the new element
  if (iter.Meta() == DictBase::kEmptySlot) { // (Case 1) available
    // Do nothing
  } else if (iter.IsHead()) { // (Case 2) hit
    // Point `iter` to the last element of the linked list
    for (BlockIter prev = iter;; prev = iter) {
      if (Equal(*key, iter.Data().first)) {
        return &iter.Data();
      }
      iter.Advance(self_base);
      if (iter.IsNone()) {
        iter = prev;
        break;
      }
    }
    // Prob at the end of the linked list for the next empty slot
    BlockIter prev = iter;
    uint8_t jump;
    iter = self_base->Probe(iter, &jump);
    if (jump == 0) {
      return nullptr;
    }
    prev.SetNext(jump);
    new_meta = DictBase::kNewTail;
  } else {
    // (Case 3) relocate: Chop the list starting from `iter`, and move it to other locations.
    BlockIter next = iter, prev = TSelf::Prev(self, iter);
    // Loop invariant:
    // - `next` points to the first element of the list we are going to relocate.
    // - `prev` points to the last element of the list that we are relocating to.
    for (uint8_t next_meta = DictBase::kProtectedSlot; !next.IsNone(); next_meta = DictBase::kEmptySlot) {
      // Step 1. Prob for the next empty slot `new_next` after `prev`
      uint8_t jump;
      BlockIter new_next = self_base->Probe(prev, &jump);
      if (jump == 0) {
        return nullptr;
      }
      // Step 2. Relocate `next` to `new_next`
      new_next.Meta() = DictBase::kNewTail;
      new_next.Data() = {next.Data().first, next.Data().second};
      std::swap(next_meta, next.Meta());
      prev.SetNext(jump);
      // Step 3. Update `prev` -> `new_next`, `next` -> `Advance(next)`
      prev = new_next;
      next = next.WithOffset(self_base, BlockIter::kNextProbeLocation[next_meta & 0b01111111]);
    }
  }
  self_base->MLCDict::size += 1;
  iter.Meta() = new_meta;
  auto &kv = iter.Data() = {*key, MLCAny()};
  key->type_index = 0;
  key->v.v_int64 = 0;
  return &kv;
}

template <typename TDictObj> //
inline void DictBase::Accessor<TDictObj>::Erase(TDictObj *self, const Any &key) {
  BlockIter iter = TSelf::Lookup(self, key);
  if (!iter.IsNone()) {
    TSelf::Erase(self, iter.i);
  } else {
    MLC_THROW(KeyError) << key;
  }
}

template <typename TDictObj> //
inline void DictBase::Accessor<TDictObj>::Erase(TDictObj *self, int64_t index) {
  DictBase *self_base = static_cast<DictBase *>(self);
  BlockIter iter = BlockIter::FromIndex(self_base, index);
  if (uint64_t offset = iter.Offset(); offset != 0) {
    BlockIter prev = iter;
    BlockIter next = iter.WithOffset(self_base, offset);
    while ((offset = next.Offset()) != 0) {
      prev = next;
      next = next.WithOffset(self_base, offset);
    }
    auto &kv = iter.Data();
    static_cast<Any &>(kv.first).Reset();
    static_cast<Any &>(kv.second).Reset();
    kv = next.Data();
    next.Meta() = DictBase::kEmptySlot;
    prev.SetNext(0);
  } else {
    if (!iter.IsHead()) {
      TSelf::Prev(self, iter).SetNext(0);
    }
    iter.Meta() = DictBase::kEmptySlot;
    auto &kv = iter.Data();
    static_cast<Any &>(kv.first).Reset();
    static_cast<Any &>(kv.second).Reset();
  }
  self_base->MLCDict::size -= 1;
}

template <typename TDictObj> //
inline Any &DictBase::Accessor<TDictObj>::At(TDictObj *self, const Any &key) {
  BlockIter iter = TSelf::Lookup(self, key);
  if (iter.IsNone()) {
    MLC_THROW(KeyError) << key;
  }
  return static_cast<Any &>(iter.Data().second);
}

template <typename TDictObj> //
inline const Any &DictBase::Accessor<TDictObj>::At(const TDictObj *self, const Any &key) {
  BlockIter iter = TSelf::Lookup(self, key);
  if (iter.IsNone()) {
    MLC_THROW(KeyError) << key;
  }
  return static_cast<const Any &>(iter.Data().second);
}

template <typename TDictObj>
inline DictBase::BlockIter DictBase::Accessor<TDictObj>::Lookup(const TDictObj *self, const MLCAny &key) {
  const DictBase *self_base = static_cast<const DictBase *>(self);
  if (self_base->Cap() == 0) { // TODO: add a unittest for this case
    return BlockIter::None();
  }
  uint64_t hash = Hash(key);
  for (BlockIter iter = self_base->Head(hash); !iter.IsNone(); iter.Advance(self_base)) {
    if (Equal(key, iter.Data().first)) {
      return iter;
    }
  }
  return BlockIter::None();
}

template <typename TDictObj>
inline int64_t DictBase::Accessor<TDictObj>::Find(const TDictObj *self, const MLCAny &key) {
  const DictBase *self_base = static_cast<const DictBase *>(self);
  BlockIter iter = Lookup(self, key);
  return iter.IsNone() ? self_base->MLCDict::capacity : static_cast<int64_t>(iter.i);
}

template <typename TDictObj>
inline DictBase::BlockIter DictBase::Accessor<TDictObj>::Prev(const TDictObj *self, BlockIter iter) {
  const DictBase *self_base = static_cast<const DictBase *>(self);
  BlockIter prev = self_base->Head(Hash(iter.Data().first));
  BlockIter next = prev;
  for (next.Advance(self_base); next.i != iter.i; prev = next, next.Advance(self_base)) {
  }
  return prev;
}

template <typename TDictObj>
inline void DictBase::Accessor<TDictObj>::New(int32_t num_args, const AnyView *args, Any *any_ret) {
  using TSelf = DictBase::Accessor<TDictObj>;
  Ref<TDictObj> ret = Ref<TDictObj>::New(num_args * 2);
  TDictObj *dict_ptr = ret.get();
  for (int32_t i = 0; i < num_args; i += 2) {
    const AnyView *k = args + i;
    const AnyView *v = args + i + 1;
    DictBase::KVPair *kv = TSelf::InsertOrLookup(dict_ptr, Any(*k));
    static_cast<Any &>(kv->second) = Any(*v);
  }
  *any_ret = std::move(ret);
}

template <bool is_const> //
inline DictBase::IterState<is_const> DictBase::IterState<is_const>::Add() const {
  int64_t cap = self->Cap();
  int64_t new_i = this->i;
  while (++new_i < cap) {
    if (DictBase::BlockIter::FromIndex(self, new_i).Meta() != DictBase::kEmptySlot) {
      return TSelf{self, new_i};
    }
  }
  return TSelf{self, cap};
}

template <bool is_const> //
inline DictBase::IterState<is_const> DictBase::IterState<is_const>::Sub() const {
  int64_t new_i = this->i;
  while (--new_i >= 0) {
    if (DictBase::BlockIter::FromIndex(self, new_i).Meta() != DictBase::kEmptySlot) {
      return TSelf{self, new_i};
    }
  }
  return TSelf{self, -1};
}

template <bool is_const> //
inline typename DictBase::IterState<is_const>::TValue DictBase::IterState<is_const>::At() const {
  return reinterpret_cast<TValue>(DictBase::BlockIter::FromIndex(self, this->i).Data());
}

template <bool is_const> //
inline typename DictBase::IterState<is_const>::TValuePtr DictBase::IterState<is_const>::Ptr() const {
  auto &ret = DictBase::BlockIter::FromIndex(self, this->i).Data();
  return reinterpret_cast<TValuePtr>(&ret);
}

static_assert(sizeof(DictBase::Block) == DictBase::kBlockCapacity * (1 + sizeof(MLCAny) * 2), "ABI check");
static_assert(std::is_aggregate_v<DictBase::Block>, "ABI check");
static_assert(std::is_trivially_destructible_v<DictBase::Block>, "ABI check");
static_assert(std::is_standard_layout_v<DictBase::Block>, "ABI check");

} // namespace core
} // namespace mlc
#endif // MLC_CORE_DICT_BASE_H_
