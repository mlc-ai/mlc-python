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
    UListObj *list = reinterpret_cast<UListObj *>(any.v_obj);
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
    DictBase *dict = reinterpret_cast<DictBase *>(any.v_obj);
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

} // namespace core
} // namespace mlc

#endif // MLC_CORE_ALL_H_
