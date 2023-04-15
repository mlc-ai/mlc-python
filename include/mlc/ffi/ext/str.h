#ifndef MLC_STR_H_
#define MLC_STR_H_
#include <cstring>
#include <mlc/ffi/core/core.h>
#include <sstream>

namespace mlc {
namespace ffi {

struct StrObj : public MLCStr {
  struct Allocator;
  std::string __str__() const {
    std::ostringstream os;
    os << '"' << this->data() << '"';
    return os.str();
  }

  MLC_DEF_STATIC_TYPE(StrObj, Object, MLCTypeIndex::kMLCStr, "object.Str")
      .FieldReadOnly("length", &MLCStr::length)
      .FieldReadOnly("data", &MLCStr::data)
      .Method("__str__", &StrObj::__str__);

  MLC_INLINE const char *c_str() const { return this->MLCStr::data; }
  MLC_INLINE const char *data() const { return this->MLCStr::data; }
  MLC_INLINE int64_t length() const { return this->MLCStr::length; }
  MLC_INLINE int64_t size() const { return this->MLCStr::length; }
  MLC_INLINE uint64_t Hash() const { return details::StrHash(reinterpret_cast<const MLCStr *>(this)); }
  MLC_INLINE bool StartsWith(const std::string &prefix) {
    int64_t N = static_cast<int64_t>(prefix.length());
    return N <= MLCStr::length && strncmp(MLCStr::data, prefix.data(), prefix.length()) == 0;
  }
  MLC_INLINE bool EndsWidth(const std::string &suffix) {
    int64_t N = static_cast<int64_t>(suffix.length());
    return N <= MLCStr::length && strncmp(MLCStr::data + MLCStr::length - N, suffix.data(), N) == 0;
  }
  MLC_INLINE int Compare(const StrObj *other) const { return std::strncmp(c_str(), other->c_str(), this->size() + 1); }
  MLC_INLINE int Compare(const std::string &other) const { return std::strncmp(c_str(), other.c_str(), size() + 1); }
  MLC_INLINE int Compare(const char *other) const { return std::strncmp(this->c_str(), other, this->size() + 1); }

protected:
  MLC_INLINE StrObj() : MLCStr() {}
};

namespace details {

struct StrStd : public StrObj {
  using Allocator = DefaultObjectAllocator<StrStd, void>;
  template <typename, typename> friend struct DefaultObjectAllocator;

  MLC_INLINE StrStd(std::string &&str) : StrObj(), container(std::move(str)) {
    this->MLCStr::length = static_cast<int64_t>(container.length());
    this->MLCStr::data = container.data();
  }

  std::string container;
};

struct StrPad : public StrObj {
  using Allocator = DefaultObjectAllocator<StrPad, void>;
  template <typename, typename> friend struct DefaultObjectAllocator;

  MLC_INLINE StrPad(const char *str, size_t N) : StrObj() {
    char *str_copy = reinterpret_cast<char *>(this) + sizeof(StrObj);
    std::memcpy(str_copy, str, N);
    str_copy[N - 1] = '\0';

    this->MLCStr::length = static_cast<int64_t>(N) - 1;
    this->MLCStr::data = str_copy;
  }
};
} // namespace details

struct StrObj::Allocator {
  MLC_INLINE static StrObj *New(std::string &&str) { return details::StrStd::Allocator::New(std::move(str)); }
  MLC_INLINE static StrObj *New(const std::string &str) {
    int64_t N = static_cast<int64_t>(str.length()) + 1;
    return details::StrPad::Allocator::NewWithPad<char>(N, str.data(), N);
  }
  MLC_INLINE static StrObj *New(std::nullptr_t) = delete;
  MLC_INLINE static StrObj *New(const char *str) {
    if (str == nullptr) {
      MLC_THROW(ValueError) << "Cannot create StrObj from nullptr";
    }
    int64_t N = static_cast<int64_t>(std::strlen(str)) + 1;
    return details::StrPad::Allocator::NewWithPad<char>(N, str, N);
  }
  template <size_t N> MLC_INLINE static StrObj *New(const CharArray<N> &str) {
    return details::StrPad::Allocator::NewWithPad<char>(N, str, N);
  }
  MLC_INLINE static StrObj *New(const char *str, size_t N) {
    return details::StrPad::Allocator::NewWithPad<char>(N, str, N);
  }
};

struct Str : public ObjectRef {
  MLC_INLINE Str(const char *str) : ObjectRef(StrObj::Allocator::New(str)) {}
  template <size_t N> MLC_INLINE Str(const CharArray<N> &str) : ObjectRef(StrObj::Allocator::New<N>(str)) {}
  MLC_INLINE const char *c_str() const { return this->get()->c_str(); }
  MLC_INLINE const char *data() const { return this->get()->data(); }
  MLC_INLINE int64_t length() const { return this->get()->length(); }
  MLC_INLINE int64_t size() const { return this->get()->size(); }
  MLC_INLINE uint64_t Hash() const { return this->get()->Hash(); }
  MLC_DEF_OBJ_REF(Str, StrObj, ObjectRef);
};

// Overload < operator
MLC_INLINE bool operator<(const Str &lhs, const std::string &rhs) { return lhs->Compare(rhs) < 0; }
MLC_INLINE bool operator<(const std::string &lhs, const Str &rhs) { return rhs->Compare(lhs) > 0; }
MLC_INLINE bool operator<(const Str &lhs, const Str &rhs) { return lhs->Compare(rhs.get()) < 0; }
MLC_INLINE bool operator<(const Str &lhs, const char *rhs) { return lhs->Compare(rhs) < 0; }
MLC_INLINE bool operator<(const char *lhs, const Str &rhs) { return rhs->Compare(lhs) > 0; }
// Overload > operator
MLC_INLINE bool operator>(const Str &lhs, const std::string &rhs) { return lhs->Compare(rhs) > 0; }
MLC_INLINE bool operator>(const std::string &lhs, const Str &rhs) { return rhs->Compare(lhs) < 0; }
MLC_INLINE bool operator>(const Str &lhs, const Str &rhs) { return lhs->Compare(rhs.get()) > 0; }
MLC_INLINE bool operator>(const Str &lhs, const char *rhs) { return lhs->Compare(rhs) > 0; }
MLC_INLINE bool operator>(const char *lhs, const Str &rhs) { return rhs->Compare(lhs) < 0; }
// Overload <= operator
MLC_INLINE bool operator<=(const Str &lhs, const std::string &rhs) { return lhs->Compare(rhs) <= 0; }
MLC_INLINE bool operator<=(const std::string &lhs, const Str &rhs) { return rhs->Compare(lhs) >= 0; }
MLC_INLINE bool operator<=(const Str &lhs, const Str &rhs) { return lhs->Compare(rhs.get()) <= 0; }
MLC_INLINE bool operator<=(const Str &lhs, const char *rhs) { return lhs->Compare(rhs) <= 0; }
MLC_INLINE bool operator<=(const char *lhs, const Str &rhs) { return rhs->Compare(lhs) >= 0; }
// Overload >= operator
MLC_INLINE bool operator>=(const Str &lhs, const std::string &rhs) { return lhs->Compare(rhs) >= 0; }
MLC_INLINE bool operator>=(const std::string &lhs, const Str &rhs) { return rhs->Compare(lhs) <= 0; }
MLC_INLINE bool operator>=(const Str &lhs, const Str &rhs) { return lhs->Compare(rhs.get()) >= 0; }
MLC_INLINE bool operator>=(const Str &lhs, const char *rhs) { return lhs->Compare(rhs) >= 0; }
MLC_INLINE bool operator>=(const char *lhs, const Str &rhs) { return rhs->Compare(lhs) <= 0; }
// Overload == operator
MLC_INLINE bool operator==(const Str &lhs, const std::string &rhs) { return lhs->Compare(rhs) == 0; }
MLC_INLINE bool operator==(const std::string &lhs, const Str &rhs) { return rhs->Compare(lhs) == 0; }
MLC_INLINE bool operator==(const Str &lhs, const Str &rhs) { return lhs->Compare(rhs.get()) == 0; }
MLC_INLINE bool operator==(const Str &lhs, const char *rhs) { return lhs->Compare(rhs) == 0; }
MLC_INLINE bool operator==(const char *lhs, const Str &rhs) { return rhs->Compare(lhs) == 0; }
// Overload != operator
MLC_INLINE bool operator!=(const Str &lhs, const std::string &rhs) { return lhs->Compare(rhs) != 0; }
MLC_INLINE bool operator!=(const std::string &lhs, const Str &rhs) { return rhs->Compare(lhs) != 0; }
MLC_INLINE bool operator!=(const Str &lhs, const Str &rhs) { return lhs->Compare(rhs.get()) != 0; }
MLC_INLINE bool operator!=(const Str &lhs, const char *rhs) { return lhs->Compare(rhs) != 0; }
MLC_INLINE bool operator!=(const char *lhs, const Str &rhs) { return rhs->Compare(lhs) != 0; }
// Overload << operator
inline std::ostream &operator<<(std::ostream &out, const Str &src) {
  out.write(src->data(), src->size());
  return out;
}

MLC_INLINE Ref<StrObj> Object::str() const {
  std::ostringstream os;
  os << *this;
  return Ref<StrObj>::New(os.str());
}

template <typename ObjectType> MLC_INLINE Ref<StrObj> Ref<ObjectType>::str() const {
  std::ostringstream os;
  os << *this;
  return Ref<StrObj>::New(os.str());
}

MLC_INLINE Ref<StrObj> AnyView::str() const {
  std::ostringstream os;
  os << *this;
  return Ref<StrObj>::New(os.str());
}

MLC_INLINE Ref<StrObj> Any::str() const {
  std::ostringstream os;
  os << *this;
  return Ref<StrObj>::New(os.str());
}

/********** Object Pointer Traits *********/

template <> struct ObjPtrTraits<StrObj, void> {
  MLC_INLINE static StrObj *AnyToUnownedPtr(const MLCAny *v) { return ObjPtrTraitsDefault<StrObj>::AnyToUnownedPtr(v); }
  MLC_INLINE static void PtrToAnyView(const StrObj *v, MLCAny *ret) {
    return ObjPtrTraitsDefault<StrObj>::PtrToAnyView(v, ret);
  }
  MLC_INLINE static StrObj *AnyToOwnedPtr(const MLCAny *v) {
    if (v->type_index == static_cast<int32_t>(MLCTypeIndex::kMLCRawStr)) {
      return StrObj::Allocator::New(v->v_str, std::strlen(v->v_str) + 1);
    }
    return AnyToUnownedPtr(v);
  }
  MLC_INLINE static StrObj *AnyToOwnedPtrWithStorage(const MLCAny *v, Any *storage) {
    if (v->type_index == static_cast<int32_t>(MLCTypeIndex::kMLCRawStr)) {
      StrObj *ret = StrObj::Allocator::New(v->v_str, std::strlen(v->v_str) + 1);
      *storage = Ref<StrObj>(ret);
      return ret;
    }
    return AnyToUnownedPtr(v);
  }
};

namespace details {

MLC_INLINE int32_t StrCompare(const MLCStr *a, const MLCStr *b) {
  if (a->length != b->length) {
    return static_cast<int32_t>(a->length - b->length);
  }
  return std::strncmp(a->data, b->data, a->length);
}

MLC_INLINE uint64_t StrHash(const MLCStr *str) {
  const constexpr uint64_t kMultiplier = 1099511628211ULL;
  const constexpr uint64_t kMod = 2147483647ULL;
  const char *it = str->data;
  const char *end = it + str->length;
  uint64_t result = 0;
  for (; it + 8 <= end; it += 8) {
    uint64_t b = (static_cast<uint64_t>(it[0]) << 56) | (static_cast<uint64_t>(it[1]) << 48) |
                 (static_cast<uint64_t>(it[2]) << 40) | (static_cast<uint64_t>(it[3]) << 32) |
                 (static_cast<uint64_t>(it[4]) << 24) | (static_cast<uint64_t>(it[5]) << 16) |
                 (static_cast<uint64_t>(it[6]) << 8) | static_cast<uint64_t>(it[7]);
    result = (result * kMultiplier + b) % kMod;
  }
  if (it < end) {
    uint64_t b = 0;
    if (it + 4 <= end) {
      b = (static_cast<uint64_t>(it[0]) << 24) | (static_cast<uint64_t>(it[1]) << 16) |
          (static_cast<uint64_t>(it[2]) << 8) | static_cast<uint64_t>(it[3]);
      it += 4;
    }
    if (it + 2 <= end) {
      b = (b << 16) | (static_cast<uint64_t>(it[0]) << 8) | static_cast<uint64_t>(it[1]);
      it += 2;
    }
    if (it + 1 <= end) {
      b = (b << 8) | static_cast<uint64_t>(it[0]);
      it += 1;
    }
    result = (result * kMultiplier + b) % kMod;
  }
  return result;
}

MLC_INLINE void AnyView2Str(std::ostream &os, const MLCAny *v) {
  AnyView attr = VTableGet(v->type_index, "__str__");
  if (details::IsTypeIndexNone(attr.type_index)) {
    MLC_THROW(InternalError) << "Method `__str__` is not defined for type " << TypeIndex2TypeKey(v->type_index);
  }
  Any ret;
  FuncCall(attr.v_obj, 1, v, &ret);
  os << ret.operator const char *();
}

MLC_INLINE MLCObject *StrMoveFromStdString(std::string &&source) {
  return reinterpret_cast<MLCObject *>(StrObj::Allocator::New(std::move(source)));
}

MLC_INLINE MLCObject *StrCopyFromCharArray(const char *source, size_t length) {
  return reinterpret_cast<MLCObject *>(StrObj::Allocator::New(source, length + 1));
}

} // namespace details

} // namespace ffi
} // namespace mlc

#endif // MLC_STR_H_
