#ifndef MLC_CORE_OBJECT_PATH_H_
#define MLC_CORE_OBJECT_PATH_H_

#include "./object.h"
#include "./str.h"

namespace mlc {
namespace core {

struct ObjectPath;

struct ObjectPathObj : public Object {
  /*!
   * kind = -1 ==> {root}
   * kind =  0 ==> field_name (const char *)
   * kind =  1 ==> list_index (int64_t)
   * kind =  2 ==> dict_key   (Any)
   */
  int32_t kind;
  Any key;
  Optional<ObjectRef> prev;
  int64_t length;

  explicit ObjectPathObj(int32_t kind, Any key, Optional<ObjectRef> prev, int64_t length)
      : kind(kind), key(key), prev(prev), length(length) {}
  explicit ObjectPathObj(int32_t kind, Any key, const ObjectPathObj *prev)
      : kind(kind), key(key), prev(prev), length(prev->length + 1) {}

  ObjectPath WithField(const char *field_name) const;
  ObjectPath WithListIndex(int64_t list_index) const;
  ObjectPath WithDictKey(Any dict_key) const;
  ::mlc::Str __str__() const;
  bool Equal(const ObjectPathObj *other) const;
  ObjectPathObj *GetPrefix(int64_t prefix_length) const;
  bool IsPrefixOf(const ObjectPathObj *other) const;

  MLC_DEF_DYN_TYPE(ObjectPathObj, Object, "mlc.core.ObjectPath");
};

struct ObjectPath : public ObjectRef {
  static ObjectPath Root() { return ObjectPath(-1, Any(), Optional<ObjectRef>(nullptr), 1); }

  MLC_DEF_OBJ_REF(ObjectPath, ObjectPathObj, ObjectRef)
      .Field("kind", &ObjectPathObj::kind)
      .Field("key", &ObjectPathObj::key)
      .Field("prev", &ObjectPathObj::prev)
      .Field("length", &ObjectPathObj::length)
      .StaticFn("__init__", ::mlc::InitOf<ObjectPathObj, int32_t, Any, Optional<ObjectRef>, int64_t>)
      .StaticFn("root", &ObjectPath::Root)
      .MemFn("__str__", &ObjectPathObj::__str__)
      .MemFn("with_field", &ObjectPathObj::WithField)
      .MemFn("with_list_index", &ObjectPathObj::WithListIndex)
      .MemFn("with_dict_key", &ObjectPathObj::WithDictKey)
      .MemFn("equal", &ObjectPathObj::Equal)
      .MemFn("get_prefix", &ObjectPathObj::GetPrefix)
      .MemFn("is_prefix_of", &ObjectPathObj::IsPrefixOf);
};

inline ::mlc::Str ObjectPathObj::__str__() const {
  std::ostringstream os;
  std::vector<const ObjectPathObj *> items;
  for (const ObjectPathObj *p = this; p; p = p->prev.Cast<ObjectPathObj>()) {
    items.push_back(p);
  }
  for (auto it = items.rbegin(); it != items.rend(); ++it) {
    const ObjectPathObj *p = *it;
    int32_t type_index = p->key.type_index;
    if (p->kind == -1) {
      os << "{root}";
    } else if (p->kind == 0) {
      os << "." << p->key.operator const char *();
    } else if (p->kind == 1) {
      os << "[" << p->key.operator int64_t() << "]";
    } else if (::mlc::base::IsTypeIndexPOD(type_index) || type_index == kMLCStr) {
      os << "[" << p->key << "]";
    } else {
      const char *type_key = ::mlc::base::TypeIndex2TypeKey(type_index);
      os << "[" << type_key << "@" << static_cast<const void *>(p->key.v.v_obj) << "]";
    }
  }
  return os.str();
}

inline ObjectPath ObjectPathObj::WithField(const char *field_name) const {
  return ObjectPath(0, Any(field_name), this);
}

inline ObjectPath ObjectPathObj::WithListIndex(int64_t list_index) const {
  return ObjectPath(1, Any(list_index), this);
}

inline ObjectPath ObjectPathObj::WithDictKey(Any dict_key) const { //
  return ObjectPath(2, std::move(dict_key), this);
}

inline bool ObjectPathObj::Equal(const ObjectPathObj *other) const {
#define MLC_OBJ_PATH_CHECK_SEG(LHS, RHS, ObjType)                                                                      \
  if ((LHS).operator ObjType() != (RHS).operator ObjType()) {                                                          \
    return false;                                                                                                      \
  } else {                                                                                                             \
    continue;                                                                                                          \
  }

#define MLC_OBJ_PATH_CHECK_SEG_PRED(LHS, RHS, ObjType, PRED)                                                           \
  if (!PRED((LHS).operator ObjType(), (RHS).operator ObjType())) {                                                     \
    return false;                                                                                                      \
  } else {                                                                                                             \
    continue;                                                                                                          \
  }

  if (kind != other->kind || length != other->length) {
    return false;
  }
  for (const ObjectPathObj *p = this, *q = other; p && q;
       p = p->prev.Cast<ObjectPathObj>(), q = q->prev.Cast<ObjectPathObj>()) {
    if (p->kind != q->kind) {
      return false;
    } else if (p->kind == -1) {
      return true;
    } else if (p->kind == 0) {
      MLC_OBJ_PATH_CHECK_SEG(p->key, q->key, mlc::Str);
    } else if (p->kind == 1) {
      MLC_OBJ_PATH_CHECK_SEG(p->key, q->key, int64_t);
    }
    int32_t type_index = p->key.GetTypeIndex();
    if (type_index != q->key.GetTypeIndex()) {
      return false;
    } else if (type_index >= kMLCStaticObjectBegin) {
      MLC_OBJ_PATH_CHECK_SEG(p->key, q->key, Object *);
    } else if (type_index == kMLCNone) {
      return true;
    } else if (type_index == kMLCInt) {
      MLC_OBJ_PATH_CHECK_SEG(p->key, q->key, int64_t);
    } else if (type_index == kMLCFloat) {
      MLC_OBJ_PATH_CHECK_SEG(p->key, q->key, double);
    } else if (type_index == kMLCPtr) {
      MLC_OBJ_PATH_CHECK_SEG(p->key, q->key, void *);
    } else if (type_index == kMLCDataType) {
      MLC_OBJ_PATH_CHECK_SEG_PRED(p->key, q->key, DLDataType, mlc::base::DataTypeEqual);
    } else if (type_index == kMLCDevice) {
      MLC_OBJ_PATH_CHECK_SEG_PRED(p->key, q->key, DLDevice, mlc::base::DeviceEqual);
    } else {
      MLC_THROW(TypeError) << "Unsupported type index: " << type_index;
    }
  }
  return true;
#undef MLC_OBJ_PATH_CHECK_SEG
#undef MLC_OBJ_PATH_CHECK_SEG_PRED
}

inline ObjectPathObj *ObjectPathObj::GetPrefix(int64_t prefix_length) const {
  if (prefix_length > length) {
    MLC_THROW(ValueError) << "prefix_length" << prefix_length << " > length: " << prefix_length << " vs " << length;
  }
  ObjectPathObj *p = const_cast<ObjectPathObj *>(this);
  for (; p->length > prefix_length; p = p->prev.Cast<ObjectPathObj>()) {
  }
  return p;
}

inline bool ObjectPathObj::IsPrefixOf(const ObjectPathObj *other) const {
  if (length > other->length) {
    return false;
  }
  return this->Equal(other->GetPrefix(length));
}

} // namespace core
} // namespace mlc

#endif // MLC_CORE_OBJECT_PATH_H_
