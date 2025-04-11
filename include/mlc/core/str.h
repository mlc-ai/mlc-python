#ifndef MLC_CORE_STR_H_
#define MLC_CORE_STR_H_
#include "./object.h"
#include <cctype>
#include <cstring>
#include <ostream>
#include <sstream>
#include <string_view>

namespace mlc {
namespace base {
template <> struct TypeTraits<StrObj *> {
  using T = StrObj;
  MLC_INLINE static void TypeToAny(T *src, MLCAny *ret) { ObjPtrTraitsDefault<T>::TypeToAny(src, ret); }
  MLC_INLINE static T *AnyToTypeUnowned(const MLCAny *v) { return ObjPtrTraitsDefault<T>::AnyToTypeUnowned(v); }
  MLC_INLINE static T *AnyToTypeOwned(const MLCAny *v) {
    if (v->type_index == static_cast<int32_t>(MLCTypeIndex::kMLCRawStr)) {
      return StrCopyFromCharArray(v->v.v_str, std::strlen(v->v.v_str));
    }
    return AnyToTypeUnowned(v);
  }
  MLC_INLINE static T *AnyToTypeWithStorage(const MLCAny *v, Any *storage) {
    if (v->type_index == static_cast<int32_t>(MLCTypeIndex::kMLCRawStr)) {
      StrObj *ret = StrCopyFromCharArray(v->v.v_str, std::strlen(v->v.v_str));
      *storage = reinterpret_cast<Object *>(ret);
      return ret;
    }
    return AnyToTypeUnowned(v);
  }
};
} // namespace base
} // namespace mlc

namespace mlc {
struct StrObj : public MLCStr {
  struct Allocator;
  std::string __str__() const {
    std::ostringstream os;
    this->PrintEscape(os);
    return os.str();
  }
  MLC_INLINE StrObj() : MLCStr() {}
  MLC_INLINE const char *c_str() const { return this->MLCStr::data; }
  MLC_INLINE const char *data() const { return this->MLCStr::data; }
  MLC_INLINE char *data() { return this->MLCStr::data; }
  MLC_INLINE int64_t length() const { return this->MLCStr::length; }
  MLC_INLINE int64_t size() const { return this->MLCStr::length; }
  MLC_INLINE bool empty() const { return this->MLCStr::length == 0; }
  MLC_INLINE const char *begin() const { return this->data(); }
  MLC_INLINE const char *end() const { return this->data() + this->length(); }
  MLC_INLINE char *begin() { return this->data(); }
  MLC_INLINE char *end() { return this->data() + this->length(); }
  MLC_INLINE char &back() { return this->data()[this->length() - 1]; }
  MLC_INLINE char &front() { return this->data()[0]; }
  MLC_INLINE const char &back() const { return this->data()[this->length() - 1]; }
  MLC_INLINE const char &front() const { return this->data()[0]; }
  MLC_INLINE void pop_back() {
    if (this->length() > 0) {
      this->back() = '\0';
      this->MLCStr::length -= 1;
    }
  }
  MLC_INLINE bool StartsWith(const std::string &prefix) {
    int64_t N = static_cast<int64_t>(prefix.length());
    return N <= MLCStr::length && strncmp(MLCStr::data, prefix.data(), prefix.length()) == 0;
  }
  MLC_INLINE bool EndsWidth(const std::string &suffix) {
    int64_t N = static_cast<int64_t>(suffix.length());
    return N <= MLCStr::length && strncmp(MLCStr::data + MLCStr::length - N, suffix.data(), N) == 0;
  }
  MLC_INLINE void PrintEscape(std::ostream &os) const;
  MLC_INLINE int32_t Compare(const char *rhs_str, int64_t rhs_len) const {
    return ::mlc::base::StrCompare(this->MLCStr::data, rhs_str, this->MLCStr::length, rhs_len);
  }
  MLC_INLINE int32_t Compare(const StrObj *other) const {
    return this->Compare(other->c_str(), other->MLCStr::length); //
  }
  MLC_INLINE int32_t Compare(const std::string &other) const {
    return this->Compare(other.data(), static_cast<int64_t>(other.length()));
  }
  MLC_INLINE int32_t Compare(const char *other) const {
    return this->Compare(other, static_cast<int64_t>(std::strlen(other)));
  }
  MLC_INLINE uint64_t Hash() const {
    return ::mlc::base::StrHash(this->MLCStr::data, this->MLCStr::length); //
  }
  inline std::vector<std::string_view> Split(char delim) const {
    std::vector<std::string_view> ret;
    const char *start = this->data();
    const char *end = start + this->length();
    for (const char *p = start; p < end; ++p) {
      if (*p == delim) {
        ret.emplace_back(start, p - start);
        start = p + 1;
      }
    }
    ret.emplace_back(start, end - start);
    return ret;
  }
  MLC_DEF_STATIC_TYPE(MLC_EXPORTS, StrObj, Object, MLCTypeIndex::kMLCStr, "object.Str");
};
} // namespace mlc

namespace mlc {
namespace core {
struct StrStd : public StrObj {
  using Allocator = ::mlc::DefaultObjectAllocator<StrStd>;

  MLC_INLINE StrStd(std::string &&str) : StrObj(), container(std::move(str)) {
    this->MLCStr::length = static_cast<int64_t>(container.length());
    this->MLCStr::data = container.data();
  }

  std::string container;
};

struct StrPad : public StrObj {
  using Allocator = ::mlc::DefaultObjectAllocator<StrPad>;

  MLC_INLINE StrPad(int64_t N) : StrObj() {
    this->MLCStr::length = N;
    this->MLCStr::data = reinterpret_cast<char *>(this) + sizeof(StrObj);
  }
  MLC_INLINE StrPad(const char *str, size_t N) : StrObj() {
    char *str_copy = reinterpret_cast<char *>(this) + sizeof(StrObj);
    std::memcpy(str_copy, str, N);
    str_copy[N - 1] = '\0';
    this->MLCStr::length = static_cast<int64_t>(N) - 1;
    this->MLCStr::data = str_copy;
  }
};

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
  MLC_INLINE Str(const char *str) : Str(StrObj::Allocator::New(str)) {}
  MLC_INLINE Str(std::string &&str) : Str(StrObj::Allocator::New(std::move(str))) {}
  MLC_INLINE Str(const std::string &str) : Str(StrObj::Allocator::New(str)) {}
  template <size_t N>
  MLC_INLINE Str(const ::mlc::base::CharArray<N> &str) : ObjectRef(StrObj::Allocator::New<N>(str)) {}
  MLC_INLINE Str FromEscaped(int64_t N, const char *str);
  MLC_INLINE const char *c_str() const { return this->get()->c_str(); }
  MLC_INLINE const char *data() const { return this->get()->data(); }
  MLC_INLINE int64_t length() const { return this->get()->length(); }
  MLC_INLINE int64_t size() const { return this->get()->size(); }
  MLC_INLINE uint64_t Hash() const { return this->get()->Hash(); }
  MLC_INLINE const char *begin() const { return this->get()->data(); }
  MLC_INLINE const char *end() const { return this->get()->data() + this->get()->length(); }
  MLC_INLINE char *begin() { return this->get()->data(); }
  MLC_INLINE char *end() { return this->get()->data() + this->get()->length(); }
  MLC_INLINE char operator[](int64_t i) const { return this->get()->data()[i]; }
  inline std::string ToStdString() const { return std::string(this->get()->data(), this->get()->length()); }
  inline std::string_view ToStdStringView() const {
    return std::string_view(this->get()->data(), this->get()->length());
  }
  MLC_DEF_OBJ_REF(MLC_EXPORTS, Str, StrObj, ObjectRef)
      .Field("length", &MLCStr::length, /*frozen=*/true)
      .Field("data", &MLCStr::data, /*frozen=*/true)
      .MemFn("__str__", &StrObj::__str__);
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

template <typename T> inline Str Ref<T>::str() const {
  AnyView v(this);
  std::ostringstream os;
  os << v;
  return Str(os.str());
}

inline std::ostream &operator<<(std::ostream &os, const AnyView &src) {
  os << Lib::Str(src)->data();
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const Any &src) {
  os << Lib::Str(src).data();
  return os;
}

template <typename T> inline std::ostream &operator<<(std::ostream &os, const Ref<T> &_src) {
  const MLCObjPtr &src = reinterpret_cast<const MLCObjPtr &>(_src);
  MLCAny v{};
  if (src.ptr != nullptr) {
    v.type_index = src.ptr->type_index;
    v.v.v_obj = src.ptr;
  }
  os << ::mlc::Lib::Str(reinterpret_cast<const AnyView &>(v))->data();
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const ObjectRef &_src) {
  const MLCObjPtr &src = reinterpret_cast<const MLCObjPtr &>(_src);
  MLCAny v{};
  if (src.ptr != nullptr) {
    v.type_index = src.ptr->type_index;
    v.v.v_obj = src.ptr;
  }
  os << ::mlc::Lib::Str(reinterpret_cast<const AnyView &>(v))->data();
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const Object &src) {
  MLCAny v{};
  v.type_index = src._mlc_header.type_index;
  v.v.v_obj = const_cast<MLCAny *>(reinterpret_cast<const MLCAny *>(&src));
  os << ::mlc::Lib::Str(reinterpret_cast<const AnyView &>(v))->data();
  return os;
}

inline void StrObj::PrintEscape(std::ostream &oss) const {
  const char *data = this->MLCStr::data;
  int64_t length = this->MLCStr::length;
  oss << '"';
  for (int64_t i = 0; i < length;) {
    unsigned char c = static_cast<unsigned char>(data[i]);
    unsigned char d = static_cast<unsigned char>(data[i + 1]);
    // Detect ANSI escape sequences
    if (c == '\x1b' && d == '[') {
      int64_t j = i + 2; // Start just after "\x1b["
      while (j < length && (std::isdigit(data[j]) || data[j] == ';')) {
        ++j; // Move past digits and semicolons in ANSI sequence
      }
      if (j < length && (data[j] == 'm' || data[j] == 'K')) {
        oss << "\\u001b[";
        for (i += 2; i <= j; ++i) {
          oss << data[i];
        }
        continue;
      }
    }
    // Handle ASCII C escape sequences
    switch (c) {
    case '\n':
      oss << "\\n";
      ++i;
      continue;
    case '\t':
      oss << "\\t";
      ++i;
      continue;
    case '\r':
      oss << "\\r";
      ++i;
      continue;
    case '\\':
      oss << "\\\\";
      ++i;
      continue;
    case '\"':
      oss << "\\\"";
      ++i;
      continue;
    default:
      break;
    }
    // Handle UTF-8 sequences
    if ((c & 0x80) == 0) {
      // 1-byte character (ASCII)
      oss << static_cast<char>(c);
      ++i;
      continue;
    }
    if ((c & 0xE0) == 0xC0 && i + 1 < length) {
      // 2-byte character
      int32_t codepoint = ((c & 0x1F) << 6) | (d & 0x3F);
      oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << codepoint;
      i += 2;
    } else if ((c & 0xF0) == 0xE0 && i + 2 < length) {
      // 3-byte character
      unsigned char e = static_cast<unsigned char>(data[i + 2]);
      int32_t codepoint = ((c & 0x0F) << 12) | ((d & 0x3F) << 6) | (e & 0x3F);
      oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << codepoint;
      i += 3;
    } else {
      // Invalid or unsupported sequence, copy raw byte
      oss << "\\x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
      ++i;
    }
    oss.unsetf(std::ios::adjustfield | std::ios::basefield | std::ios::floatfield);
    oss.fill(' ');
  }
  oss << '"';
}

inline Str Str::FromEscaped(int64_t N, const char *str) {
  std::ostringstream oss;
  if (N < 2 || str[0] != '\"' || str[N - 1] != '\"') {
    MLC_THROW(ValueError) << "Invalid escaped string: " << str;
  }
  N -= 1; // Exclude the trailing quote
  for (int64_t i = 1; i < N;) {
    if (str[i] == '\\' && i + 1 < N) {
      char next = str[i + 1];
      switch (next) {
      case 'n': // Newline
        oss << '\n';
        i += 2;
        continue;
      case 't': // Tab
        oss << '\t';
        i += 2;
        continue;
      case 'r': // Carriage return
        oss << '\r';
        i += 2;
        continue;
      case '\\': // Backslash
        oss << '\\';
        i += 2;
        continue;
      case '\"': // Double quote
        oss << '\"';
        i += 2;
        continue;
      case 'x': // Hexadecimal byte escape (\xHH)
        if (i + 3 < N && std::isxdigit(str[i + 2]) && std::isxdigit(str[i + 3])) {
          int32_t value = std::stoi(std::string(str + i + 2, 2), nullptr, 16);
          oss << static_cast<char>(value);
          i += 4;
          continue;
        } else {
          MLC_THROW(ValueError) << "Invalid hexadecimal escape sequence at position " << i << " in string: " << str;
        }
      case 'u': // Unicode escape (\uXXXX)
        if (i + 5 < N && std::isxdigit(str[i + 2]) && std::isxdigit(str[i + 3]) && std::isxdigit(str[i + 4]) &&
            std::isxdigit(str[i + 5])) {
          int32_t codepoint = std::stoi(std::string(str + i + 2, 4), nullptr, 16);
          if (codepoint <= 0x7F) {
            // 1-byte UTF-8
            oss << static_cast<char>(codepoint);
          } else if (codepoint <= 0x7FF) {
            // 2-byte UTF-8
            oss << static_cast<char>(0xC0 | (codepoint >> 6));
            oss << static_cast<char>(0x80 | (codepoint & 0x3F));
          } else {
            // 3-byte UTF-8
            oss << static_cast<char>(0xE0 | (codepoint >> 12));
            oss << static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            oss << static_cast<char>(0x80 | (codepoint & 0x3F));
          }
          i += 6;
          continue;
        } else {
          MLC_THROW(ValueError) << "Invalid Unicode escape sequence at position " << i << " in string: " << str;
        }
      default:
        // Unrecognized escape sequence
        oss << str[i + 1]; // Interpret literally
        i += 2;
        continue;
      }
    } else {
      // Regular character, copy as-is
      oss << str[i];
      ++i;
    }
  }
  return Str(oss.str());
}
} // namespace mlc

namespace mlc {
namespace base {
MLC_INLINE StrObj *StrCopyFromCharArray(const char *source, size_t length) {
  return StrObj::Allocator::New(source, length + 1);
}
} // namespace base
} // namespace mlc

#endif // MLC_CORE_STR_H_
