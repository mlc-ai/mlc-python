#ifndef MLC_CORE_ALL_H_
#define MLC_CORE_ALL_H_
#include "./dict.h"         // IWYU pragma: export
#include "./error.h"        // IWYU pragma: export
#include "./func.h"         // IWYU pragma: export
#include "./func_details.h" // IWYU pragma: export
#include "./func_traits.h"  // IWYU pragma: export
#include "./list.h"         // IWYU pragma: export
#include "./object.h"       // IWYU pragma: export
#include "./str.h"          // IWYU pragma: export
#include "./typing.h"       // IWYU pragma: export
#include "./udict.h"        // IWYU pragma: export
#include "./ulist.h"        // IWYU pragma: export

namespace mlc {
namespace core {
template <typename T> MLC_INLINE_NO_MSVC void NestedTypeCheck<T>::Run(const MLCAny &any) {
  try {
    static_cast<const AnyView &>(any).Cast<T>();
  } catch (const Exception &e) {
    throw NestedTypeError(e.what()).NewFrame(base::Type2Str<T>::Run());
  }
}

template <typename T> MLC_INLINE_NO_MSVC void NestedTypeCheck<List<T>>::Run(const MLCAny &any) {
  try {
    UList(static_cast<const AnyView &>(any));
  } catch (const Exception &e) {
    throw NestedTypeError(e.what()).NewFrame(::mlc::base::Type2Str<UList>::Run());
  }
  if constexpr (!std::is_same_v<T, Any>) {
    UListObj *list = reinterpret_cast<UListObj *>(any.v.v_obj);
    int64_t size = list->size();
    for (int32_t i = 0; i < size; ++i) {
      try {
        NestedTypeCheck<T>::Run(list->data()[i]);
      } catch (NestedTypeError &e) {
        throw e.NewIndex(i);
      }
    }
  }
}

template <typename K, typename V> MLC_INLINE_NO_MSVC void NestedTypeCheck<Dict<K, V>>::Run(const MLCAny &any) {
  try {
    UDict(static_cast<const AnyView &>(any));
  } catch (const Exception &e) {
    throw NestedTypeError(e.what()).NewFrame(::mlc::base::Type2Str<UDict>::Run());
  }
  if constexpr (!std::is_same_v<K, Any> || !std::is_same_v<V, Any>) {
    DictBase *dict = reinterpret_cast<DictBase *>(any.v.v_obj);
    dict->IterateAll([](uint8_t *, MLCAny *key, MLCAny *value) {
      if constexpr (!std::is_same_v<K, Any>) {
        try {
          NestedTypeCheck<K>::Run(*key);
        } catch (NestedTypeError &e) {
          throw e.NewFrame(::mlc::base::Type2Str<K>::Run());
        }
      }
      if constexpr (!std::is_same_v<V, Any>) {
        try {
          NestedTypeCheck<V>::Run(*value);
        } catch (NestedTypeError &e) {
          throw e.NewIndex(*static_cast<AnyView *>(key));
        }
      }
    });
  }
}

template <typename Visitor> inline void VisitTypeField(void *obj_addr, MLCTypeInfo *info, Visitor &&visitor) {
  MLCTypeField *fields = info->fields;
  MLCTypeField *field = fields;
  for (; field->name != nullptr; ++field) {
    const char *field_name = field->name;
    void *field_addr = static_cast<char *>(obj_addr) + field->offset;
    int32_t num_bytes = field->num_bytes;
    if (field->ty->type_index == kMLCTypingAny && num_bytes == sizeof(MLCAny)) {
      visitor(reinterpret_cast<Any *>(field_addr), field_name);
    } else if (field->ty->type_index == kMLCTypingAtomic) {
      int32_t type_index = reinterpret_cast<MLCTypingAtomic *>(field->ty)->type_index;
      if (type_index >= MLCTypeIndex::kMLCStaticObjectBegin && num_bytes == sizeof(MLCObjPtr)) {
        visitor(reinterpret_cast<::mlc::Object **>(field_addr), field_name);
      } else if (type_index == kMLCInt && num_bytes == 1) {
        visitor(reinterpret_cast<int8_t *>(field_addr), field_name);
      } else if (type_index == kMLCInt && num_bytes == 2) {
        visitor(reinterpret_cast<int16_t *>(field_addr), field_name);
      } else if (type_index == kMLCInt && num_bytes == 4) {
        visitor(reinterpret_cast<int32_t *>(field_addr), field_name);
      } else if (type_index == kMLCInt && num_bytes == 8) {
        visitor(reinterpret_cast<int64_t *>(field_addr), field_name);
      } else if (type_index == kMLCFloat && num_bytes == 4) {
        visitor(reinterpret_cast<float *>(field_addr), field_name);
      } else if (type_index == kMLCFloat && num_bytes == 8) {
        visitor(reinterpret_cast<double *>(field_addr), field_name);
      } else if (type_index == kMLCPtr && num_bytes == sizeof(void *)) {
        visitor(reinterpret_cast<void **>(field_addr), field_name);
      } else if (type_index == kMLCDataType && num_bytes == sizeof(DLDataType)) {
        visitor(reinterpret_cast<DLDataType *>(field_addr), field_name);
      } else if (type_index == kMLCDevice && num_bytes == sizeof(DLDevice)) {
        visitor(reinterpret_cast<DLDevice *>(field_addr), field_name);
      } else if (type_index == kMLCRawStr) {
        visitor(reinterpret_cast<const char *>(field_addr), field_name);
      } else {
        MLC_THROW(InternalError) << "Unknown supported type: " << base::TypeIndex2TypeKey(type_index)
                                 << " with size (in bytes): " << num_bytes;
      }
    } else if (field->ty->type_index == kMLCTypingPtr) { // TODO: support pointer type
      MLC_THROW(InternalError) << "Pointer type is not supported yet";
    } else if (field->ty->type_index == kMLCTypingOptional && num_bytes == sizeof(MLCObjPtr)) {
      MLCAny *ty = reinterpret_cast<MLCTypingOptional *>(field->ty)->ty.ptr;
      MLCTypingAtomic *ty_atomic = reinterpret_cast<MLCTypingAtomic *>(ty);
      if (ty->type_index == kMLCTypingAtomic && ty_atomic->type_index >= kMLCStaticObjectBegin) {
        visitor(reinterpret_cast<::mlc::Object **>(field_addr), field_name);
      } else {
        visitor(reinterpret_cast<::MLCBoxedPOD **>(field_addr), field_name);
      }
    } else if (field->ty->type_index == kMLCTypingList && num_bytes == sizeof(MLCObjPtr)) {
      visitor(reinterpret_cast<::mlc::Object **>(field_addr), field_name);
    } else if (field->ty->type_index == kMLCTypingDict && num_bytes == sizeof(MLCObjPtr)) {
      visitor(reinterpret_cast<::mlc::Object **>(field_addr), field_name);
    } else {
      MLC_THROW(InternalError) << "Unknown supported type: " << base::TypeIndex2TypeKey(field->ty->type_index);
    }
  }
}

struct ExternObjDeleter {
  void operator()(Any *any, const char *) { any->Reset(); }
  void operator()(Object **obj, const char *) { ::mlc::base::DecRef(reinterpret_cast<MLCAny *>(*obj)); }
  void operator()(::MLCBoxedPOD **pod, const char *) { ::mlc::base::DecRef(reinterpret_cast<MLCAny *>(*pod)); }
  void operator()(int8_t *, const char *) {}
  void operator()(int16_t *, const char *) {}
  void operator()(int32_t *, const char *) {}
  void operator()(int64_t *, const char *) {}
  void operator()(float *, const char *) {}
  void operator()(double *, const char *) {}
  void operator()(void **, const char *) {}
  void operator()(DLDataType *, const char *) {}
  void operator()(DLDevice *, const char *) {}
  void operator()(const char *, const char *) {}
};

} // namespace core
} // namespace mlc

#endif // MLC_CORE_ALL_H_
