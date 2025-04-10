#ifndef MLC_CORE_ALL_H_
#define MLC_CORE_ALL_H_
#include "./dict.h"         // IWYU pragma: export
#include "./error.h"        // IWYU pragma: export
#include "./func.h"         // IWYU pragma: export
#include "./func_details.h" // IWYU pragma: export
#include "./list.h"         // IWYU pragma: export
#include "./object.h"       // IWYU pragma: export
#include "./object_path.h"  // IWYU pragma: export
#include "./opaque.h"       // IWYU pragma: export
#include "./reflection.h"   // IWYU pragma: export
#include "./str.h"          // IWYU pragma: export
#include "./tensor.h"       // IWYU pragma: export
#include "./typing.h"       // IWYU pragma: export
#include "./utils.h"        // IWYU pragma: export
#include "./visitor.h"      // IWYU pragma: export

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
  int32_t type_index = objptr->GetTypeIndex();
  MLCTypeInfo *info = Lib::GetTypeInfo(type_index);
  if (info) {
    struct ExternObjDeleter {
      MLC_INLINE void operator()(MLCTypeField *, Any *any) { any->Reset(); }
      MLC_INLINE void operator()(MLCTypeField *, ObjectRef *obj) { obj->Reset(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<ObjectRef> *opt) { opt->Reset(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<bool> *opt) { opt->Reset(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<int64_t> *opt) { opt->Reset(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<double> *opt) { opt->Reset(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<DLDevice> *opt) { opt->Reset(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<DLDataType> *opt) { opt->Reset(); }
      MLC_INLINE void operator()(MLCTypeField *, Optional<void *> *opt) { opt->Reset(); }
      MLC_INLINE void operator()(MLCTypeField *, bool *) {}
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

namespace mlc {
inline ::mlc::Str Lib::CxxStr(AnyView obj) {
  FuncObj *func = VTableGetFunc(cxx_str, obj.type_index, "__cxx_str__");
  Any ret;
  ::mlc::base::FuncCall(func, 1, &obj, &ret);
  return ret;
}
inline ::mlc::Str Lib::Str(AnyView obj) {
  FuncObj *func = VTableGetFunc(str, obj.type_index, "__str__");
  Any ret;
  ::mlc::base::FuncCall(func, 1, &obj, &ret);
  return ret;
}
inline int64_t Lib::StructuralHash(AnyView obj) {
  static FuncObj *func_hash_s = ::mlc::Lib::FuncGetGlobal("mlc.core.StructuralHash");
  Any ret;
  ::mlc::base::FuncCall(func_hash_s, 1, &obj, &ret);
  return ret;
}
inline bool Lib::StructuralEqual(AnyView a, AnyView b, bool bind_free_vars, bool assert_mode) {
  static FuncObj *func_eq_s = ::mlc::Lib::FuncGetGlobal("mlc.core.StructuralEqual");
  Any ret;
  ::mlc::base::FuncCall(func_eq_s, 4, std::array<AnyView, 4>{a, b, bind_free_vars, assert_mode}.data(), &ret);
  return ret;
}
inline Any Lib::IRPrint(AnyView obj, AnyView printer, AnyView path) {
  FuncObj *func = Lib::VTableGetFunc(ir_print, obj.GetTypeIndex(), "__ir_print__");
  Any ret;
  ::mlc::base::FuncCall(func, 3, std::array<AnyView, 3>{obj, printer, path}.data(), &ret);
  return ret;
}
inline int32_t Lib::FuncSetGlobal(const char *name, FuncObj *func, bool allow_override) {
  MLC_CHECK_ERR(::MLCFuncSetGlobal(_lib, name, Any(func), allow_override));
  return 0;
}
inline FuncObj *Lib::FuncGetGlobal(const char *name, bool allow_missing) {
  Any ret;
  MLC_CHECK_ERR(::MLCFuncGetGlobal(_lib, name, &ret));
  if (!ret.defined() && !allow_missing) {
    MLC_THROW(KeyError) << "Missing global function: " << name;
  }
  return ret;
}
inline const char *Lib::DeviceTypeToStr(int32_t device_type) {
  static FuncObj *func_device_type2str = ::mlc::Lib::FuncGetGlobal("mlc.base.DeviceTypeToStr");
  AnyView arg(device_type);
  Any ret;
  ::mlc::base::FuncCall(func_device_type2str, 1, &arg, &ret);
  void *ptr = ret;
  return static_cast<const char *>(ptr);
}
inline int32_t Lib::DeviceTypeFromStr(const char *source) {
  static FuncObj *func_device_type2str = ::mlc::Lib::FuncGetGlobal("mlc.base.DeviceTypeFromStr");
  AnyView arg(source);
  Any ret;
  ::mlc::base::FuncCall(func_device_type2str, 1, &arg, &ret);
  return ret;
}
inline void Lib::DeviceTypeRegister(const char *name) {
  static FuncObj *func_device_type_register = ::mlc::Lib::FuncGetGlobal("mlc.base.DeviceTypeRegister");
  AnyView arg(name);
  Any ret;
  ::mlc::base::FuncCall(func_device_type_register, 1, &arg, &ret);
}
inline const char *Lib::DataTypeCodeToStr(int32_t dtype_code) {
  static FuncObj *func_dtype_code2str = ::mlc::Lib::FuncGetGlobal("mlc.base.DataTypeCodeToStr");
  AnyView arg(dtype_code);
  Any ret;
  ::mlc::base::FuncCall(func_dtype_code2str, 1, &arg, &ret);
  void *ptr = ret;
  return static_cast<const char *>(ptr);
}
inline DLDataType Lib::DataTypeFromStr(const char *source) {
  static FuncObj *func_dtype_from_str = ::mlc::Lib::FuncGetGlobal("mlc.base.DataTypeFromStr");
  AnyView arg(source);
  Any ret;
  ::mlc::base::FuncCall(func_dtype_from_str, 1, &arg, &ret);
  return ret;
}
inline void Lib::DataTypeRegister(const char *name, int32_t dtype_bits) {
  static FuncObj *func_dtype_register = ::mlc::Lib::FuncGetGlobal("mlc.base.DataTypeRegister");
  AnyView arg[2]{name, dtype_bits};
  Any ret;
  ::mlc::base::FuncCall(func_dtype_register, 2, arg, &ret);
}
template <typename R, typename... Args> inline R VTable::operator()(Args... args) const {
  constexpr size_t N = sizeof...(Args);
  AnyViewArray<N> stack_args;
  Any ret;
  stack_args.Fill(std::forward<Args>(args)...);
  MLC_CHECK_ERR(::MLCVTableCall(self, N, stack_args.v, &ret));
  if constexpr (std::is_same_v<R, void>) {
    return;
  } else {
    return ret;
  }
}
template <typename Obj> inline VTable &VTable::Set(Func func) {
  constexpr bool override_mode = false;
  MLC_CHECK_ERR(::MLCVTableSetFunc(this->self, Obj::_type_index, func.get(), override_mode));
  return *this;
}

} // namespace mlc

#endif // MLC_CORE_ALL_H_
