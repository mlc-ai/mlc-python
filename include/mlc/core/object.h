#ifndef MLC_OBJECT_H_
#define MLC_OBJECT_H_

#include <mlc/base/all.h>

namespace mlc {

struct Object {
  MLCAny _mlc_header;

  MLC_INLINE Object() : _mlc_header() {}
  MLC_INLINE Object(const Object &) : _mlc_header() {}
  MLC_INLINE Object(Object &&) {}
  MLC_INLINE Object &operator=(const Object &) { return *this; }
  MLC_INLINE Object &operator=(Object &&) { return *this; }
  Str str() const;
  friend std::ostream &operator<<(std::ostream &os, const Object &src);

  MLC_DEF_STATIC_TYPE(Object, ::mlc::base::ObjectDummyRoot, MLCTypeIndex::kMLCObject, "object.Object");
};

struct ObjectRef : protected ::mlc::base::ObjectRefDummyRoot {
  MLC_DEF_OBJ_REF(ObjectRef, Object, ::mlc::base::ObjectRefDummyRoot).Method("__init__", InitOf<Object>);
};

} // namespace mlc

#endif // MLC_OBJECT_H_
