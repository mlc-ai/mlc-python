#ifndef MLC_CORE_ALL_H_
#define MLC_CORE_ALL_H_
#include "./dict.h"          // IWYU pragma: export
#include "./error.h"         // IWYU pragma: export
#include "./field_visitor.h" // IWYU pragma: export
#include "./func.h"          // IWYU pragma: export
#include "./func_details.h"  // IWYU pragma: export
#include "./list.h"          // IWYU pragma: export
#include "./object.h"        // IWYU pragma: export
#include "./object_path.h"   // IWYU pragma: export
#include "./str.h"           // IWYU pragma: export
#include "./typing.h"        // IWYU pragma: export

namespace mlc {
namespace core {
template <typename T> MLC_INLINE_NO_MSVC void NestedTypeCheck<T>::Run(const MLCAny &any) {
  try {
    static_cast<const AnyView &>(any).operator T();
  } catch (const Exception &e) {
    throw NestedTypeError(e.what()).NewFrame(base::Type2Str<T>::Run());
  }
}

template <typename T> MLC_INLINE_NO_MSVC void NestedTypeCheck<List<T>>::Run(const MLCAny &any) {
  try {
    [[maybe_unused]] UList ret(static_cast<const AnyView *>(&any));
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
    [[maybe_unused]] UDict ret(static_cast<const AnyView *>(&any));
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

inline void ReportTypeFieldError(const char *type_key, MLCTypeField *field) {
  ::mlc::Object *field_ty = reinterpret_cast<::mlc::Object *>(field->ty);
  MLC_THROW(InternalError) << "Field `" << type_key << "." << field->name << "` whose size is " << field->num_bytes
                           << " byte(s) is not supported yet, because its type is: " << AnyView(field_ty);
}

MLC_INLINE void DeleteExternObject(Object *objptr) {
  MLCTypeInfo *info = nullptr;
  int32_t type_index = objptr->GetTypeIndex();
  MLCTypeIndex2Info(nullptr, type_index, &info);
  if (info) {
    struct ExternObjDeleter {
      MLC_INLINE void operator()(MLCTypeField *, Any *any) { any->Reset(); }
      MLC_INLINE void operator()(MLCTypeField *, ObjectRef *obj) { obj->Reset(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<ObjectRef> *opt) { opt->Reset(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<int64_t> *opt) { opt->Reset(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<double> *opt) { opt->Reset(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<DLDevice> *opt) { opt->Reset(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<DLDataType> *opt) { opt->Reset(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<void *> *opt) { opt->Reset(); }
      MLC_INLINE void operator()(MLCTypeField *, int8_t *) {}
      MLC_INLINE void operator()(MLCTypeField *, int16_t *) {}
      MLC_INLINE void operator()(MLCTypeField *, int32_t *) {}
      MLC_INLINE void operator()(MLCTypeField *, int64_t *) {}
      MLC_INLINE void operator()(MLCTypeField *, float *) {}
      MLC_INLINE void operator()(MLCTypeField *, double *) {}
      MLC_INLINE void operator()(MLCTypeField *, void **) {}
      MLC_INLINE void operator()(MLCTypeField *, DLDataType *) {}
      MLC_INLINE void operator()(MLCTypeField *, DLDevice *) {}
      MLC_INLINE void operator()(MLCTypeField *, const char **) {}
    };
    VisitFields(objptr, info, ExternObjDeleter{});
    std::free(objptr);
  } else {
    MLC_THROW(InternalError) << "Cannot find type info for type index: " << type_index;
  }
}

} // namespace core
} // namespace mlc

namespace mlc {
namespace base {

template <typename K, typename V>
MLC_INLINE DictObj<K, V> *TypeTraits<DictObj<K, V> *>::AnyToTypeUnowned(const MLCAny *v) {
  return ObjPtrTraitsDefault<UDictObj>::AnyToTypeUnowned(v)->AsTyped<K, V>();
}
template <typename E> MLC_INLINE ListObj<E> *TypeTraits<ListObj<E> *>::AnyToTypeUnowned(const MLCAny *v) {
  return ObjPtrTraitsDefault<UListObj>::AnyToTypeUnowned(v)->AsTyped<E>();
}

} // namespace base
} // namespace mlc

#endif // MLC_CORE_ALL_H_
