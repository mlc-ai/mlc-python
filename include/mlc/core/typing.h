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
      return "mlc.Ptr";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCDataType)) {
      return "mlc.DataType";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCDevice)) {
      return "mlc.Device";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCRawStr)) {
      return "char*";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCObject)) {
      return "mlc.Object";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCList)) {
      return "list[Any]";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCDict)) {
      return "dict[Any, Any]";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCFunc)) {
      return "mlc.Func";
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

struct PtrTypeObj : protected MLCTypingPtr {
  explicit PtrTypeObj(Type ty) : MLCTypingPtr{} { this->TyMutable() = ty; }
  Type Ty() const { return Type(reinterpret_cast<const Ref<TypeObj> &>(this->MLCTypingPtr::ty)); }
  std::string __str__() const {
    std::ostringstream os;
    os << "Ptr[" << this->Ty() << "]";
    return os.str();
  }
  MLC_DEF_STATIC_TYPE(PtrTypeObj, TypeObj, MLCTypeIndex::kMLCTypingPtr, "mlc.core.typing.PtrType");

private:
  Type &TyMutable() { return reinterpret_cast<Type &>(this->MLCTypingPtr::ty); }
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
    os << this->Ty() << " | None";
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
    } else if constexpr (std::is_base_of_v<UList, T> || std::is_base_of_v<UListObj, T>) {
      using E = typename T::TElem;
      return typing::List(ParseType<E>());
    } else if constexpr (std::is_base_of_v<UDict, T> || std::is_base_of_v<UDictObj, T>) {
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

template <typename T> typing::Type ParseType() { return ::mlc::core::TypeAnnParser<::mlc::base::RemoveCR<T>>::Run(); }

} // namespace core
} // namespace mlc

#endif // MLC_CORE_TYPING_H_
