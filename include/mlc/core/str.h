#ifndef MLC_STR_H_
#define MLC_STR_H_
#include "./object.h"
#include <cstring>
#include <sstream>

namespace mlc {
namespace base {
template <> struct ObjPtrTraits<StrObj, void> {
  MLC_INLINE static StrObj *AnyToUnownedPtr(const MLCAny *v) { return ObjPtrTraitsDefault<StrObj>::AnyToUnownedPtr(v); }
  MLC_INLINE static void PtrToAnyView(const StrObj *v, MLCAny *ret) {
    return ObjPtrTraitsDefault<StrObj>::PtrToAnyView(v, ret);
  }
  MLC_INLINE static StrObj *AnyToOwnedPtr(const MLCAny *v) {
    if (v->type_index == static_cast<int32_t>(MLCTypeIndex::kMLCRawStr)) {
      return StrCopyFromCharArray(v->v_str, std::strlen(v->v_str));
    }
    return AnyToUnownedPtr(v);
  }
  MLC_INLINE static StrObj *AnyToOwnedPtrWithStorage(const MLCAny *v, Any *storage) {
    if (v->type_index == static_cast<int32_t>(MLCTypeIndex::kMLCRawStr)) {
      StrObj *ret = StrCopyFromCharArray(v->v_str, std::strlen(v->v_str));
      *storage = reinterpret_cast<Object *>(ret);
      return ret;
    }
    return AnyToUnownedPtr(v);
  }
};
} // namespace base
} // namespace mlc

namespace mlc {
struct StrObj : public MLCStr {
  struct Allocator;
  std::string __str__() const {
    std::ostringstream os;
    os << '"' << this->data() << '"';
    return os.str();
  }
  MLC_INLINE StrObj() : MLCStr() {}
  MLC_INLINE const char *c_str() const { return this->MLCStr::data; }
  MLC_INLINE const char *data() const { return this->MLCStr::data; }
  MLC_INLINE int64_t length() const { return this->MLCStr::length; }
  MLC_INLINE int64_t size() const { return this->MLCStr::length; }
  MLC_INLINE uint64_t Hash() const;
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
  MLC_DEF_STATIC_TYPE(StrObj, Object, MLCTypeIndex::kMLCStr, "object.Str")
      .FieldReadOnly("length", &MLCStr::length)
      .FieldReadOnly("data", &MLCStr::data)
      .Method("__str__", &StrObj::__str__);
};
} // namespace mlc

namespace mlc {
namespace core {
struct StrStd : public StrObj {
  using Allocator = ::mlc::base::DefaultObjectAllocator<StrStd, void>;
  template <typename, typename> friend struct ::mlc::base::DefaultObjectAllocator;

  MLC_INLINE StrStd(std::string &&str) : StrObj(), container(std::move(str)) {
    this->MLCStr::length = static_cast<int64_t>(container.length());
    this->MLCStr::data = container.data();
  }

  std::string container;
};

struct StrPad : public StrObj {
  using Allocator = ::mlc::base::DefaultObjectAllocator<StrPad, void>;
  template <typename, typename> friend struct ::mlc::base::DefaultObjectAllocator;

  MLC_INLINE StrPad(const char *str, size_t N) : StrObj() {
    char *str_copy = reinterpret_cast<char *>(this) + sizeof(StrObj);
    std::memcpy(str_copy, str, N);
    str_copy[N - 1] = '\0';
    this->MLCStr::length = static_cast<int64_t>(N) - 1;
    this->MLCStr::data = str_copy;
  }
};

inline void PrintAnyToStream(std::ostream &os, const MLCAny *v) {
  Any attr;
  MLCVTableGet(nullptr, v->type_index, "__str__", &attr);
  if (::mlc::base::IsTypeIndexNone(attr.type_index)) {
    MLC_THROW(InternalError) << "Method `__str__` is not defined for type "
                             << ::mlc::base::TypeIndex2TypeKey(v->type_index);
  }
  Any ret;
  ::mlc::base::FuncCall(attr.v_obj, 1, v, &ret);
  os << ret.operator const char *();
}

} // namespace core
} // namespace mlc

namespace mlc {
struct StrObj::Allocator {
  MLC_INLINE static StrObj *New(std::string &&str) { return core::StrStd::Allocator::New(std::move(str)); }
  MLC_INLINE static StrObj *New(const std::string &str) {
    int64_t N = static_cast<int64_t>(str.length()) + 1;
    return core::StrPad::Allocator::NewWithPad<char>(N, str.data(), N);
  }
  MLC_INLINE static StrObj *New(std::nullptr_t) = delete;
  MLC_INLINE static StrObj *New(const char *str) {
    if (str == nullptr) {
      MLC_THROW(ValueError) << "Cannot create StrObj from nullptr";
    }
    int64_t N = static_cast<int64_t>(std::strlen(str)) + 1;
    return core::StrPad::Allocator::NewWithPad<char>(N, str, N);
  }
  template <size_t N> MLC_INLINE static StrObj *New(const ::mlc::base::CharArray<N> &str) {
    return core::StrPad::Allocator::NewWithPad<char>(N, str, N);
  }
  MLC_INLINE static StrObj *New(const char *str, size_t N) {
    return core::StrPad::Allocator::NewWithPad<char>(N, str, N);
  }
};

struct Str : public ObjectRef {
  MLC_INLINE Str(const char *str) : ObjectRef(StrObj::Allocator::New(str)) {}
  template <size_t N>
  MLC_INLINE Str(const ::mlc::base::CharArray<N> &str) : ObjectRef(StrObj::Allocator::New<N>(str)) {}
  MLC_INLINE const char *c_str() const { return this->get()->c_str(); }
  MLC_INLINE const char *data() const { return this->get()->data(); }
  MLC_INLINE int64_t length() const { return this->get()->length(); }
  MLC_INLINE int64_t size() const { return this->get()->size(); }
  MLC_INLINE uint64_t Hash() const { return this->get()->Hash(); }
  MLC_DEF_OBJ_REF(Str, StrObj, ObjectRef);
};
// Overload << operator
inline std::ostream &operator<<(std::ostream &out, const Str &src) {
  out.write(src->data(), src->size());
  return out;
}
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
} // namespace mlc

namespace mlc {
inline Str AnyView::str() const {
  std::ostringstream os;
  os << *this;
  return Str(os.str());
}

inline Str Any::str() const {
  std::ostringstream os;
  os << *this;
  return Str(os.str());
}

inline Str Object::str() const {
  std::ostringstream os;
  os << *this;
  return Str(os.str());
}

template <typename ObjectType> MLC_INLINE Str Ref<ObjectType>::str() const {
  std::ostringstream os;
  os << *this;
  return Str(os.str());
}

inline std::ostream &operator<<(std::ostream &os, const AnyView &src) {
  ::mlc::core::PrintAnyToStream(os, &src);
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const Any &src) {
  ::mlc::core::PrintAnyToStream(os, &src);
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const ::mlc::base::ObjPtrBase &src) {
  MLCAny v{};
  if (src.ptr != nullptr) {
    v.type_index = src.ptr->type_index;
    v.v_obj = src.ptr;
  }
  ::mlc::core::PrintAnyToStream(os, &v);
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const Object &src) {
  MLCAny v{};
  v.type_index = src._mlc_header.type_index;
  v.v_obj = const_cast<MLCAny *>(reinterpret_cast<const MLCAny *>(&src));
  ::mlc::core::PrintAnyToStream(os, &v);
  return os;
}
} // namespace mlc

namespace mlc {
namespace core {

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
} // namespace core
namespace base {
MLC_INLINE StrObj *StrCopyFromCharArray(const char *source, size_t length) {
  return StrObj::Allocator::New(source, length + 1);
}
} // namespace base
MLC_INLINE uint64_t StrObj::Hash() const { return ::mlc::core::StrHash(reinterpret_cast<const MLCStr *>(this)); }
} // namespace mlc

#endif // MLC_STR_H_
