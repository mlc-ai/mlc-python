#ifndef MLC_CORE_OPAQUE_H_
#define MLC_CORE_OPAQUE_H_

#include "./object.h"
#include "./typing.h"
#include <cstring>

namespace mlc {

struct OpaqueObj : public MLCOpaque {
  explicit OpaqueObj(void *handle_, void *handle_deleter_, const char *opaque_type_name_) : MLCOpaque{} {
    this->handle = handle_;
    this->handle_deleter = reinterpret_cast<MLCDeleterType>(handle_deleter_);
    this->opaque_type_name = [opaque_type_name_]() {
      char *ret = new char[std::strlen(opaque_type_name_) + 1];
      std::memcpy(ret, opaque_type_name_, std::strlen(opaque_type_name_) + 1);
      return ret;
    }();
  }
  ~OpaqueObj() {
    delete[] this->opaque_type_name;
    this->handle_deleter(this->handle);
  }
  std::string __str__() const { return "<Opaque `" + std::string(this->opaque_type_name) + "`>"; }
  MLC_DEF_STATIC_TYPE(MLC_EXPORTS, OpaqueObj, Object, MLCTypeIndex::kMLCOpaque, "mlc.core.Opaque");
};

struct Opaque : public ObjectRef {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, Opaque, OpaqueObj, ObjectRef)
      .StaticFn("__init__", InitOf<OpaqueObj, void *, void *, const char *>)
      .MemFn("__str__", &OpaqueObj::__str__)
      .Field("handle", &OpaqueObj::handle, /*frozen=*/true)
      ._Field("handle_deleter", offsetof(MLCOpaque, handle_deleter), sizeof(MLCOpaque::handle_deleter), true,
              ::mlc::core::ParseType<void *>())
      .Field("opaque_type_name", &OpaqueObj::opaque_type_name, /*frozen=*/true);
  explicit Opaque(void *handle, void *handle_deleter, const char *opaque_type_name)
      : Opaque(Opaque::New(handle, handle_deleter, opaque_type_name)) {}
};

} // namespace mlc

#endif // MLC_CORE_OPAQUE_H_
