#ifndef MLC_ULIST_BASE_H_
#define MLC_ULIST_BASE_H_

#include <mlc/ffi/core/core.h>
#include <vector>

namespace mlc {
namespace ffi {

struct ListBase : public MLCList {
  MLC_INLINE ListBase() : MLCList() {
    this->MLCList::capacity = 0;
    this->MLCList::size = 0;
    this->MLCList::data = nullptr;
  }
  MLC_INLINE ListBase(std::initializer_list<Any> init) : ListBase() {
    std::vector<Any> elems(init.begin(), init.end());
    int64_t numel = static_cast<int64_t>(elems.size());
    this->MLCList::data = details::PODArrayCreate<MLCAny>(numel).release();
    this->MLCList::capacity = numel;
    this->MLCList::size = 0;
    this->Replace(0, 0, numel, elems.data());
  }
  template <typename Iter> MLC_INLINE ListBase(Iter first, Iter last) : ListBase() {
    std::vector<Any> elems(first, last);
    int64_t numel = static_cast<int64_t>(elems.size());
    this->MLCList::data = details::PODArrayCreate<MLCAny>(numel).release();
    this->MLCList::capacity = numel;
    this->MLCList::size = 0;
    this->Replace(0, 0, numel, elems.data());
  }
  template <typename SubObject> struct ffi {
    MLC_INLINE static void New(int32_t num_args, const AnyView *args, Any *ret) {
      Ref<SubObject> x = Ref<SubObject>::New();
      x->EnsureCapacity(num_args);
      x->insert(0, args, args + num_args);
      *ret = std::move(x);
    }
    MLC_INLINE static Any At(SubObject *self, int64_t i) { return self->operator[](i); }
  };
  template <typename Iter> MLC_INLINE void insert(int64_t i, Iter first, Iter last) {
    std::vector<Any> elems(first, last);
    int64_t numel = static_cast<int64_t>(elems.size());
    this->EnsureCapacity(this->MLCList::size + numel);
    this->Replace(i, i, numel, elems.data());
  }
  MLC_INLINE ~ListBase() {
    details::PODArrayFinally finally{this->MLCList::data};
    clear();
  }
  MLC_INLINE void insert(int64_t i, Any data) {
    this->EnsureCapacity(this->MLCList::size + 1);
    Replace(i, i, 1, &data);
  }
  MLC_INLINE void reserve(int64_t cap) {
    if (cap > this->MLCList::capacity) {
      this->EnsureCapacity(cap);
    }
  }
  MLC_INLINE void clear() {
    DecRef(this->data(), 0, this->MLCList::size);
    this->MLCList::size = 0;
  }
  MLC_INLINE void resize(int64_t new_size) {
    int64_t &cur_size = this->MLCList::size;
    if (new_size > cur_size) {
      this->EnsureCapacity(new_size);
      std::memset(this->data() + cur_size, 0, (new_size - cur_size) * sizeof(MLCAny));
    } else {
      ListBase::DecRef(data(), new_size, cur_size);
    }
    cur_size = new_size;
  }
  MLC_INLINE void Append(Any data) {
    this->EnsureCapacity(this->MLCList::size + 1);
    Replace(this->MLCList::size, this->MLCList::size, 1, &data);
  }
  MLC_INLINE MLCAny *data() { return reinterpret_cast<MLCAny *>(MLCList::data); }
  MLC_INLINE const MLCAny *data() const { return reinterpret_cast<const MLCAny *>(MLCList::data); }

  static void DecRef(MLCAny *base, int64_t begin, int64_t end) {
    for (int64_t i = begin; i < end; ++i) {
      MLCAny &data = base[i];
      if (!details::IsTypeIndexPOD(data.type_index)) { // TODO(@junrushao): handle throw-in-deleter
        details::DecRef(data.v_obj);
      }
    }
  }

  static void ListRangeCheck(int64_t begin, int64_t end, int64_t length) {
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

  void Replace(int64_t begin, int64_t end, int64_t numel, Any *first) {
    int64_t cur_size = this->MLCList::size;
    int64_t delta = numel - (end - begin);
    ListRangeCheck(begin, end, cur_size);
    MLCAny *base = this->data();
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

  ListBase *EnsureCapacity(int64_t new_capacity) {
    new_capacity = details::BitCeil(new_capacity);
    if (new_capacity > this->MLCList::capacity) {
      details::PODArray new_data = details::PODArrayCreate<MLCAny>(new_capacity);
      std::memcpy(new_data.get(), this->data(), this->MLCList::size * sizeof(MLCAny));
      details::PODArraySwapOut(&new_data, &this->MLCList::data);
      this->MLCList::capacity = new_capacity;
    }
    return this;
  }
};

template <typename SubCls> struct ListBaseIterator {
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = Any;
  MLC_INLINE ListBaseIterator() = default;
  MLC_INLINE ListBaseIterator(const ListBaseIterator &) = default;
  MLC_INLINE ListBaseIterator(ListBaseIterator &&) = default;
  MLC_INLINE ListBaseIterator &operator=(const ListBaseIterator &) = default;
  MLC_INLINE ListBaseIterator &operator=(ListBaseIterator &&) = default;
  /* clang-format off */
  MLC_INLINE SubCls &operator++() { ++i; return Self(); }
  MLC_INLINE SubCls &operator--() { --i; return Self(); }
  MLC_INLINE SubCls operator++(int) { SubCls ret = Self(); ++i; return ret; }
  MLC_INLINE SubCls operator--(int) { SubCls ret = Self(); --i; return ret; }
  MLC_INLINE SubCls operator+(difference_type n) const { SubCls ret = Self(); ret.i += n; return ret; }
  MLC_INLINE SubCls operator-(difference_type n) const { SubCls ret = Self(); ret.i -= n; return ret; }
  MLC_INLINE SubCls &operator+=(difference_type n) { i += n; return Self(); }
  MLC_INLINE SubCls &operator-=(difference_type n) { i -= n; return Self(); }
  /* clang-format on */
  MLC_INLINE difference_type operator-(const ListBaseIterator &other) const { return i - other.i; }
  MLC_INLINE bool operator==(const ListBaseIterator &other) const { return i == other.i; }
  MLC_INLINE bool operator!=(const ListBaseIterator &other) const { return i != other.i; }
  ListBaseIterator(ListBase *self, int64_t i) : self(self), i(i) {}
  ListBase *self;
  int64_t i;

private:
  SubCls &Self() { return *static_cast<SubCls *>(this); }
  const SubCls &Self() const { return *static_cast<const SubCls *>(this); }
};

} // namespace ffi
} // namespace mlc

#endif // MLC_ULIST_BASE_H_
