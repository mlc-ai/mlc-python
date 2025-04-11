#ifndef MLC_CORE_ERROR_H_
#define MLC_CORE_ERROR_H_

#include "./object.h"
#include <string>
#include <vector>

namespace mlc {

struct ErrorObj : public MLCError {
  struct Allocator;
  std::string __str__() const { return this->ByteArray(); }
  const char *ByteArray() const { return reinterpret_cast<const char *>(this + 1); }
  char *ByteArray() { return reinterpret_cast<char *>(this + 1); }
  const char *kind() const { return MLCError::kind; }

  explicit ErrorObj(const char *kind, MLCByteArray message, MLCByteArray traceback) {
    // Assumption:
    // 1) `message` ends with no '\0'
    // 2) `traceback` ends with exactly 1 '\0'
    MLCError::kind = kind;
    char *byte_array = this->ByteArray();
    std::memcpy(byte_array, message.bytes, message.num_bytes);
    byte_array[message.num_bytes] = '\0';
    byte_array += message.num_bytes + 1;
    std::memcpy(byte_array, traceback.bytes, traceback.num_bytes);
    byte_array[traceback.num_bytes] = '\0';
  }

  explicit ErrorObj(const char *kind, int64_t num_bytes, const char *bytes) {
    MLCError::kind = kind;
    char *byte_array = this->ByteArray();
    std::memcpy(byte_array, bytes, num_bytes);
    byte_array[num_bytes] = '\0';
  }

  inline Ref<ErrorObj> AppendWith(MLCByteArray traceback) const;

  void GetInfo(std::vector<const char *> *ret) const {
    ret->clear();
    const char *bytes = this->ByteArray();
    while (*bytes != '\0') {
      ret->push_back(bytes);
      bytes += std::strlen(bytes) + 1;
    }
  }

  void FormatExc(std::ostream &os) const {
    std::vector<const char *> info;
    this->GetInfo(&info);
    os << "Traceback (most recent call last):" << std::endl;
    int frame_id = 0;
    for (size_t i = 1; i < info.size(); i += 3) {
      const char *filename = info[i];
      const char *funcname = info[i + 2];
      os << "  [" << ++frame_id << "] File \"" << filename << "\", line " << info[i + 1] << ", in " << funcname
         << std::endl;
    }
    os << this->kind() << ": " << info[0] << std::endl;
  }

  MLC_DEF_STATIC_TYPE(MLC_EXPORTS, ErrorObj, Object, MLCTypeIndex::kMLCError, "object.Error");
};

struct ErrorObj::Allocator {
  MLC_INLINE static ErrorObj *New(const char *kind, MLCByteArray message, MLCByteArray traceback) {
    return ::mlc::DefaultObjectAllocator<ErrorObj>::NewWithPad<char>(message.num_bytes + traceback.num_bytes + 2, kind,
                                                                     message, traceback);
  }
  MLC_INLINE static ErrorObj *New(const char *kind, int64_t num_bytes, const char *bytes) {
    return ::mlc::DefaultObjectAllocator<ErrorObj>::NewWithPad<char>(num_bytes + 1, kind, num_bytes, bytes);
  }
};

struct Error : public ObjectRef {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, Error, ErrorObj, ObjectRef)
      .Field("kind", &MLCError::kind, /*frozen=*/true)
      .MemFn("__str__", &ErrorObj::__str__);
  explicit Error(const char *kind, MLCByteArray message, MLCByteArray traceback)
      : Error(Error::New(kind, message, traceback)) {}
  explicit Error(const char *kind, int64_t num_bytes, const char *bytes) : Error(Error::New(kind, num_bytes, bytes)) {}
};

inline Ref<ErrorObj> ErrorObj::AppendWith(MLCByteArray traceback) const {
  MLCByteArray self;
  self.bytes = this->ByteArray();
  self.num_bytes = [&]() -> int64_t {
    const char *end_bytes = this->ByteArray();
    while (*end_bytes != '\0') {
      end_bytes += std::strlen(end_bytes) + 1;
    }
    return end_bytes - self.bytes - 1;
  }();
  return Ref<ErrorObj>::New(MLCError::kind, self, traceback);
}

inline const char *Exception::what() const noexcept(true) {
  if (data_.get() == nullptr) {
    return "mlc::ffi::Exception: Unspecified";
  }
  return Obj()->ByteArray();
}

inline Exception::Exception(Ref<ErrorObj> data) : data_(data.get()) {}

inline void Exception::FormatExc(std::ostream &os) const {
  if (data_.get()) {
    Obj()->FormatExc(os);
  } else {
    os << "mlc.Exception: Unspecified";
  }
}
} // namespace mlc

namespace mlc {
namespace base {
[[noreturn]] inline void MLCThrowError(const char *kind, MLCByteArray message, MLCByteArray traceback) noexcept(false) {
  throw Exception(Ref<ErrorObj>::New(kind, message, traceback));
}
inline Any MLCCreateError(const char *kind, const std::string &message, MLCByteArray traceback) {
  return Ref<ErrorObj>::New(kind, MLCByteArray{static_cast<int64_t>(message.size()), message.data()}, traceback);
}
} // namespace base
} // namespace mlc

#endif // MLC_CORE_ERROR_H_
