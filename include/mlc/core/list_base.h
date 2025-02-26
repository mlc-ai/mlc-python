#ifndef MLC_CORE_LIST_BASE_H_
#define MLC_CORE_LIST_BASE_H_

#include <mlc/base/all.h>
#include <vector>

namespace mlc {
namespace core {

struct ListBase : public MLCList {
  MLC_INLINE ~ListBase();
  // Constructors
  MLC_INLINE ListBase();
  MLC_INLINE ListBase(std::initializer_list<Any> init);
  template <typename Iter> MLC_INLINE ListBase(Iter first, Iter last);
  // Mutations
  template <typename Iter> MLC_INLINE void Insert(int64_t i, Iter first, Iter last);
  MLC_INLINE void Insert(int64_t i, Any value);
  MLC_INLINE void Reserve(int64_t cap);
  MLC_INLINE void Clear();
  MLC_INLINE void Resize(int64_t new_size);
  MLC_INLINE void Append(Any value);
  // Accessors
  MLC_INLINE Any *Data() { return static_cast<Any *>(MLCList::data); }
  MLC_INLINE const Any *Data() const { return static_cast<const Any *>(MLCList::data); }
  // Iterators
  template <typename ListPtr, typename Ref> struct IterState {
    ListPtr self;
    int64_t i;
    MLC_INLINE IterState Add(std::ptrdiff_t n) const { return IterState{self, i + n}; }
    MLC_INLINE Ref At() const { return self->Data()[i]; }
  };
  using IterStateMut = IterState<ListBase *, Any &>;
  using IterStateConst = IterState<const ListBase *, const Any &>;
  MLC_INLINE IterStateConst Begin() const { return IterStateConst{this, 0}; }
  MLC_INLINE IterStateConst End() const { return IterStateConst{this, this->MLCList::size}; }
  MLC_INLINE IterStateMut Begin() { return IterStateMut{this, 0}; }
  MLC_INLINE IterStateMut End() { return IterStateMut{this, this->MLCList::size}; }
  static void DecRef(MLCAny *base, int64_t begin, int64_t end);
  static void ListRangeCheck(int64_t begin, int64_t end, int64_t length);
  void Replace(int64_t begin, int64_t end, int64_t numel, Any *first);
  ListBase *EnsureCapacity(int64_t new_capacity);

  template <typename TListObj> struct Accessor {
    MLC_INLINE static void New(int32_t num_args, const AnyView *args, Any *ret);
    MLC_INLINE static Any At(TListObj *self, int64_t i);
    MLC_INLINE static void SetItem(TListObj *self, int64_t i, Any value);
  };
};

/////////////////// Implementation ///////////////////

MLC_INLINE ListBase::ListBase() : MLCList() {
  this->MLCList::capacity = 0;
  this->MLCList::size = 0;
  this->MLCList::data = nullptr;
}

MLC_INLINE ListBase::ListBase(std::initializer_list<Any> init) : ListBase() {
  std::vector<Any> elems(init.begin(), init.end());
  int64_t numel = static_cast<int64_t>(elems.size());
  this->MLCList::data = ::mlc::base::PODArrayCreate<MLCAny>(numel).release();
  this->MLCList::capacity = numel;
  this->MLCList::size = 0;
  this->Replace(0, 0, numel, elems.data());
}

template <typename Iter> MLC_INLINE ListBase::ListBase(Iter first, Iter last) : ListBase() {
  std::vector<Any> elems(first, last);
  int64_t numel = static_cast<int64_t>(elems.size());
  this->MLCList::data = ::mlc::base::PODArrayCreate<MLCAny>(numel).release();
  this->MLCList::capacity = numel;
  this->MLCList::size = 0;
  this->Replace(0, 0, numel, elems.data());
}

template <typename Iter> MLC_INLINE void ListBase::Insert(int64_t i, Iter first, Iter last) {
  std::vector<Any> elems(first, last);
  int64_t numel = static_cast<int64_t>(elems.size());
  this->EnsureCapacity(this->MLCList::size + numel);
  this->Replace(i, i, numel, elems.data());
}

MLC_INLINE ListBase::~ListBase() {
  ::mlc::base::PODArrayFinally finally{this->MLCList::data};
  Clear();
}

MLC_INLINE void ListBase::Insert(int64_t i, Any value) {
  this->EnsureCapacity(this->MLCList::size + 1);
  Replace(i, i, 1, &value);
}

MLC_INLINE void ListBase::Reserve(int64_t cap) {
  if (cap > this->MLCList::capacity) {
    this->EnsureCapacity(cap);
  }
}
MLC_INLINE void ListBase::Clear() {
  DecRef(this->Data(), 0, this->MLCList::size);
  this->MLCList::size = 0;
}

MLC_INLINE void ListBase::Resize(int64_t new_size) {
  int64_t &cur_size = this->MLCList::size;
  if (new_size > cur_size) {
    this->EnsureCapacity(new_size);
    std::memset(static_cast<MLCAny *>(this->data) + cur_size, 0, (new_size - cur_size) * sizeof(MLCAny));
  } else {
    ListBase::DecRef(Data(), new_size, cur_size);
  }
  cur_size = new_size;
}

MLC_INLINE void ListBase::Append(Any value) {
  this->EnsureCapacity(this->MLCList::size + 1);
  Replace(this->MLCList::size, this->MLCList::size, 1, &value);
}

inline void ListBase::DecRef(MLCAny *base, int64_t begin, int64_t end) {
  for (int64_t i = begin; i < end; ++i) {
    MLCAny &data = base[i];
    if (!::mlc::base::IsTypeIndexPOD(data.type_index)) { // TODO(@junrushao): handle throw-in-deleter
      ::mlc::base::DecRef(data.v.v_obj);
    }
  }
}

inline void ListBase::ListRangeCheck(int64_t begin, int64_t end, int64_t length) {
  if (begin > end) {
    MLC_THROW(IndexError) << "Invalid range [" << begin << ", " << end << ") when indexing a list";
  }
  if (begin < 0 || end > length) {
    if (begin == end || begin + 1 == end) {
      MLC_THROW(IndexError) << "Indexing `" << begin << "` of a list of size " << length;
    } else {
      MLC_THROW(IndexError) << "Indexing [" << begin << ", " << end << ") of a list of size " << length;
    }
  }
}

inline void ListBase::Replace(int64_t begin, int64_t end, int64_t numel, Any *first) {
  int64_t cur_size = this->MLCList::size;
  int64_t delta = numel - (end - begin);
  ListRangeCheck(begin, end, cur_size);
  MLCAny *base = this->Data();
  // Step 1. Deallocate items in [begin, end)
  DecRef(base, begin, end);
  // Step 2. Move [end, size) to [end + delta, size + delta)
  // so there are exaclty `num_elems` free slots in [begin, end + delta)
  std::memmove(&base[end + delta], &base[end], (cur_size - end) * sizeof(MLCAny));
  // Step 3. Copy `num_elems` items to [begin, end + delta)
  for (int64_t i = 0; i < numel; ++i) {
    Any v = std::move(first[i]); // TODO(@junrushao): optimize this
    base[begin + i] = static_cast<MLCAny>(v);
    static_cast<MLCAny &>(v) = MLCAny();
  }
  this->MLCList::size += delta;
}

inline ListBase *ListBase::EnsureCapacity(int64_t new_capacity) {
  new_capacity = ::mlc::base::BitCeil(new_capacity);
  if (new_capacity > this->MLCList::capacity) {
    ::mlc::base::PODArray new_data = ::mlc::base::PODArrayCreate<MLCAny>(new_capacity);
    std::memcpy(new_data.get(), this->Data(), this->MLCList::size * sizeof(MLCAny));
    ::mlc::base::PODArraySwapOut(&new_data, &this->MLCList::data);
    this->MLCList::capacity = new_capacity;
  }
  return this;
}

template <typename TListObj>
MLC_INLINE void ListBase::Accessor<TListObj>::New(int32_t num_args, const AnyView *args, Any *ret) {
  Ref<TListObj> x = Ref<TListObj>::New();
  x->insert(0, args, args + num_args);
  *ret = std::move(x);
}

template <typename TListObj> //
MLC_INLINE Any ListBase::Accessor<TListObj>::At(TListObj *self, int64_t i) {
  return self->operator[](i);
}

template <typename TListObj> //
MLC_INLINE void ListBase::Accessor<TListObj>::SetItem(TListObj *self, int64_t i, Any value) {
  self->operator[](i) = std::move(value);
}

} // namespace core
} // namespace mlc

#endif // MLC_CORE_LIST_BASE_H_
