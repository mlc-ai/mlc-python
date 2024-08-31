#ifndef MLC_ERROR_H_
#define MLC_ERROR_H_

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

  MLC_DEF_STATIC_TYPE(ErrorObj, Object, MLCTypeIndex::kMLCError, "object.Error")
      .FieldReadOnly("kind", &MLCError::kind)
      .Method("__str__", &ErrorObj::__str__);
};

struct ErrorObj::Allocator {
  MLC_INLINE static ErrorObj *New(const char *kind, MLCByteArray message, MLCByteArray traceback) {
    return ::mlc::base::DefaultObjectAllocator<ErrorObj>::NewWithPad<char>(message.num_bytes + traceback.num_bytes + 2,
                                                                           kind, message, traceback);
  }
  MLC_INLINE static ErrorObj *New(const char *kind, int64_t num_bytes, const char *bytes) {
    return ::mlc::base::DefaultObjectAllocator<ErrorObj>::NewWithPad<char>(num_bytes + 1, kind, num_bytes, bytes);
  }
};

struct Error : public ObjectRef {
  MLC_DEF_OBJ_REF(Error, ErrorObj, ObjectRef);
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

struct Exception : public std::exception {
  Ref<ErrorObj> data_;

  Exception(Ref<ErrorObj> data) : data_(data) {}
  Exception(const Exception &other) : data_(other.data_) {}
  Exception(Exception &&other) : data_(std::move(other.data_)) {}
  Exception &operator=(const Exception &other) {
    this->data_ = other.data_;
    return *this;
  }
  Exception &operator=(Exception &&other) {
    this->data_ = std::move(other.data_);
    return *this;
  }

  const char *what() const noexcept(true) override {
    if (data_.get() == nullptr) {
      return "mlc::ffi::Exception: Unspecified";
    }
    return data_->ByteArray();
  }

  void MoveToAny(Any *v) { *v = std::move(this->data_); }
};
} // namespace mlc

namespace mlc {
namespace core {
template <typename T> MLC_INLINE_NO_MSVC void NestedTypeCheck<T>::Run(const MLCAny &any) {
  try {
    static_cast<const AnyView &>(any).Cast<T>();
  } catch (const Exception &e) {
    throw NestedTypeError(e.what()).NewFrame(base::Type2Str<T>::Run());
  }
}
} // namespace core
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

#endif // MLC_ERROR_H_
