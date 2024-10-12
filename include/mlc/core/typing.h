#ifndef MLC_CORE_TYPING_H_
#define MLC_CORE_TYPING_H_

#include "./func.h"
#include "./object.h"
#include <mlc/base/all.h>
#include <sstream>
#include <type_traits>

namespace mlc {
namespace core {
namespace typing {

struct TypeObj : public Object {
protected:
  TypeObj() = default;
  MLC_DEF_STATIC_TYPE(TypeObj, Object, MLCTypeIndex::kMLCTyping, "mlc.core.typing.Type");
};

struct Type : public ObjectRef {
  MLC_DEF_OBJ_REF(Type, TypeObj, ObjectRef);
};

struct AnyTypeObj : protected MLCTypingAny {
  AnyTypeObj() = default;
  std::string __str__() const { return "Any"; }
  MLC_DEF_STATIC_TYPE(AnyTypeObj, TypeObj, MLCTypeIndex::kMLCTypingAny, "mlc.core.typing.AnyType");
};

struct AnyType : public Type {
  MLC_DEF_OBJ_REF(AnyType, AnyTypeObj, Type)
      .StaticFn("__init__", InitOf<AnyTypeObj>)
      .MemFn("__str__", &AnyTypeObj::__str__);
};

struct NoneTypeObj : protected MLCTypingNone {
  NoneTypeObj() = default;
  std::string __str__() const { return "None"; }
  MLC_DEF_STATIC_TYPE(NoneTypeObj, TypeObj, MLCTypeIndex::kMLCTypingNone, "mlc.core.typing.NoneType");
};

struct NoneType : public Type {
  MLC_DEF_OBJ_REF(NoneType, NoneTypeObj, Type)
      .StaticFn("__init__", InitOf<NoneTypeObj>)
      .MemFn("__str__", &NoneTypeObj::__str__);
};

struct AtomicTypeObj : protected MLCTypingAtomic {
  explicit AtomicTypeObj(int32_t type_index) : MLCTypingAtomic{} { this->type_index = type_index; }
  std::string __str__() const {
    if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCNone)) {
      return "None";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCInt)) {
      return "int";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCFloat)) {
      return "float";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCPtr)) {
      return "Ptr";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCDataType)) {
      return "dtype";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCDevice)) {
      return "Device";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCRawStr)) {
      return "char*";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCObject)) {
      return "object";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCList)) {
      return "list";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCDict)) {
      return "dict";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCFunc)) {
      return "callable";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCStr)) {
      return "str";
    }
    return ::mlc::base::TypeIndex2TypeInfo(this->type_index)->type_key;
  }
  using MLCTypingAtomic::type_index;
  MLC_DEF_STATIC_TYPE(AtomicTypeObj, TypeObj, MLCTypeIndex::kMLCTypingAtomic, "mlc.core.typing.AtomicType");
};

struct AtomicType : public Type {
  MLC_DEF_OBJ_REF(AtomicType, AtomicTypeObj, Type)
      .FieldReadOnly("type_index", &MLCTypingAtomic::type_index)
      .StaticFn("__init__", InitOf<AtomicTypeObj, int32_t>)
      .MemFn("__str__", &AtomicTypeObj::__str__);
};

struct PtrTypeObj : protected MLCTypingRawPtr {
  explicit PtrTypeObj(Type ty) : MLCTypingRawPtr{} { this->TyMutable() = ty; }
  Type Ty() const { return Type(reinterpret_cast<const Ref<TypeObj> &>(this->MLCTypingRawPtr::ty)); }
  std::string __str__() const {
    std::ostringstream os;
    os << "Ptr[" << this->Ty() << "]";
    return os.str();
  }
  MLC_DEF_STATIC_TYPE(PtrTypeObj, TypeObj, MLCTypeIndex::kMLCTypingPtr, "mlc.core.typing.PtrType");

private:
  Type &TyMutable() { return reinterpret_cast<Type &>(this->MLCTypingRawPtr::ty); }
};

struct PtrType : public Type {
  MLC_DEF_OBJ_REF(PtrType, PtrTypeObj, Type)
      .StaticFn("__init__", InitOf<PtrTypeObj, Type>)
      .MemFn("_ty", &PtrTypeObj::Ty)
      .MemFn("__str__", &PtrTypeObj::__str__);
};

struct OptionalObj : protected MLCTypingOptional {
  explicit OptionalObj(Type ty) : MLCTypingOptional{} { this->TyMutable() = ty; }
  Type Ty() const { return Type(reinterpret_cast<const Ref<TypeObj> &>(this->MLCTypingOptional::ty)); }
  std::string __str__() const {
    std::ostringstream os;
    os << "Optional[" << this->Ty() << "]";
    return os.str();
  }
  MLC_DEF_STATIC_TYPE(OptionalObj, TypeObj, MLCTypeIndex::kMLCTypingOptional, "mlc.core.typing.Optional");

private:
  Type &TyMutable() { return reinterpret_cast<Type &>(this->MLCTypingOptional::ty); }
};

struct Optional : public Type {
  MLC_DEF_OBJ_REF(Optional, OptionalObj, Type)
      .StaticFn("__init__", InitOf<OptionalObj, Type>)
      .MemFn("_ty", &OptionalObj::Ty)
      .MemFn("__str__", &OptionalObj::__str__);
};

struct UnionObj : protected MLCTypingUnion {
  friend struct Union;
  using MLCTypingUnion::num_types;
  explicit UnionObj(int32_t num_types) : MLCTypingUnion{} { this->num_types = num_types; }

  Type Ty(int32_t i) const {
    const Ref<TypeObj> *types = reinterpret_cast<const Ref<TypeObj> *>(this + 1);
    return Type(types[i]);
  }
  std::string __str__() const {
    if (this->num_types == 0) {
      return "...";
    }
    std::ostringstream os;
    os << this->Ty(0);
    for (int32_t i = 1; i < this->num_types; ++i) {
      os << " | " << this->Ty(i);
    }
    return os.str();
  }
  MLC_DEF_STATIC_TYPE(UnionObj, TypeObj, MLCTypeIndex::kMLCTypingUnion, "mlc.core.typing.Union");

protected:
  Type &TyMutable(int32_t i) { return reinterpret_cast<Type *>(this + 1)[i]; }
};

struct Union : public Type {
  MLC_DEF_OBJ_REF(Union, UnionObj, Type)
      .StaticFn("__init__",
                [](int32_t num_args, const ::mlc::AnyView *args, ::mlc::Any *ret) {
                  Ref<UnionObj> obj = Ref<UnionObj>::New(num_args);
                  for (int32_t i = 0; i < num_args; ++i) {
                    obj->TyMutable(i) = Type(args[i]);
                  }
                  *ret = obj;
                })
      .MemFn("_ty", &UnionObj::Ty)
      .MemFn("__str__", &UnionObj::__str__);
};

struct ListObj : protected MLCTypingList {
  explicit ListObj(Type ty) : MLCTypingList{} { this->TyMutable() = ty; }
  Type Ty() const { return Type(reinterpret_cast<const Ref<TypeObj> &>(this->MLCTypingList::ty)); }
  std::string __str__() const {
    std::ostringstream os;
    os << "list[" << this->Ty() << "]";
    return os.str();
  }
  MLC_DEF_STATIC_TYPE(ListObj, TypeObj, MLCTypeIndex::kMLCTypingList, "mlc.core.typing.List");

protected:
  Type &TyMutable() { return reinterpret_cast<Type &>(this->MLCTypingList::ty); }
};

struct List : public Type {
  MLC_DEF_OBJ_REF(List, ListObj, Type)
      .StaticFn("__init__", InitOf<ListObj, Type>)
      .MemFn("_ty", &ListObj::Ty)
      .MemFn("__str__", &ListObj::__str__);
};

struct DictObj : protected MLCTypingDict {
  explicit DictObj(Type ty_k, Type ty_v) : MLCTypingDict{} {
    this->TyMutableK() = ty_k;
    this->TyMutableV() = ty_v;
  }
  Type key() const { return Type(reinterpret_cast<const Ref<TypeObj> &>(this->ty_k)); }
  Type value() const { return Type(reinterpret_cast<const Ref<TypeObj> &>(this->ty_v)); }
  std::string __str__() const {
    std::ostringstream os;
    os << "dict[" << this->key() << ", " << this->value() << "]";
    return os.str();
  }
  MLC_DEF_STATIC_TYPE(DictObj, TypeObj, MLCTypeIndex::kMLCTypingDict, "mlc.core.typing.Dict");

protected:
  Type &TyMutableK() { return reinterpret_cast<Type &>(this->ty_k); }
  Type &TyMutableV() { return reinterpret_cast<Type &>(this->ty_v); }
};

struct Dict : public Type {
  MLC_DEF_OBJ_REF(Dict, DictObj, Type)
      .StaticFn("__init__", InitOf<DictObj, Type, Type>)
      .MemFn("_key", &DictObj::key)
      .MemFn("_value", &DictObj::value)
      .MemFn("__str__", &DictObj::__str__);
};

} // namespace typing

namespace details {
template <typename T> struct FakeObjectRef : public ObjectRef {
  using TObj = T;
};

template <typename T> struct TypeAnnParser {
  static typing::Type Run() {
    using namespace mlc::base;
    if constexpr (std::is_same_v<T, Any> || std::is_same_v<T, AnyView>) {
      return typing::AnyType();
    } else if constexpr (IsPOD<T>) {
      return typing::AtomicType(TypeTraits<T>::type_index);
    } else if constexpr (IsRawObjPtr<T>) {
      using U = std::remove_pointer_t<T>;
      return typing::PtrType(ParseType<U>());
    } else if constexpr (IsRef<T> || IsOptional<T>) {
      using U = typename T::TObj;
      return typing::Optional(ParseType<U>());
    } else if constexpr (std::is_base_of_v<UListObj, T>) {
      using E = typename T::TElem;
      return typing::List(ParseType<E>());
    } else if constexpr (std::is_base_of_v<UDictObj, T>) {
      using K = typename T::TKey;
      using V = typename T::TValue;
      return typing::Dict(ParseType<K>(), ParseType<V>());
    } else if constexpr (IsObjRef<T>) {
      using U = typename T::TObj;
      return typing::AtomicType(U::_type_index);
    } else if constexpr (IsObj<T>) {
      return typing::AtomicType(T::_type_index);
    }
    return typing::Type(::mlc::Null);
  }
};

} // namespace details

template <typename T> typing::Type ParseType() {
  return ::mlc::core::details::TypeAnnParser<::mlc::base::RemoveCR<T>>::Run();
}

} // namespace core
} // namespace mlc

#endif // MLC_CORE_TYPING_H_
