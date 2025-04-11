#ifndef MLC_CORE_TYPING_H_
#define MLC_CORE_TYPING_H_

#include "./object.h"
#include "./str.h"
#include <mlc/base/all.h>
#include <sstream>
#include <string_view>
#include <type_traits>

namespace mlc {
namespace core {
namespace typing {

struct TypeObj : public Object {
protected:
  TypeObj() = default;
  MLC_DEF_STATIC_TYPE(MLC_EXPORTS, TypeObj, Object, MLCTypeIndex::kMLCTyping, "mlc.core.typing.Type");
};

struct Type : public ObjectRef {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, Type, TypeObj, ObjectRef);
};

struct AnyTypeObj : protected MLCTypingAny {
  AnyTypeObj() = default;
  ::mlc::Str __str__() const { return "Any"; }
  ::mlc::Str __cxx_str__() const { return "::mlc::Any"; }
  MLC_DEF_STATIC_TYPE(MLC_EXPORTS, AnyTypeObj, TypeObj, MLCTypeIndex::kMLCTypingAny, "mlc.core.typing.AnyType");
};

struct AnyType : public Type {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, AnyType, AnyTypeObj, Type)
      .StaticFn("__init__", InitOf<AnyTypeObj>)
      .MemFn("__str__", &AnyTypeObj::__str__)
      .MemFn("__cxx_str__", &AnyTypeObj::__cxx_str__);
  explicit AnyType() : AnyType(AnyType::New()) {}
};

struct AtomicTypeObj : protected MLCTypingAtomic {
  explicit AtomicTypeObj(int32_t type_index) : MLCTypingAtomic{} { this->type_index = type_index; }
  ::mlc::Str __str__() const {
    if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCNone)) {
      return "None";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCBool)) {
      return "bool";
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
    return ::mlc::Lib::GetTypeKey(this->type_index);
  }
  ::mlc::Str __cxx_str__() const {
    if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCNone)) {
      return "std::nullptr_t";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCBool)) {
      return "bool";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCInt)) {
      return "int64_t";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCFloat)) {
      return "double";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCPtr)) {
      return "void*";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCDataType)) {
      return "DLDataType";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCDevice)) {
      return "DLDevice";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCRawStr)) {
      return "char*";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCObject)) {
      return "::mlc::ObjectRef";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCList)) {
      return "::mlc::UList";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCDict)) {
      return "::mlc::UDict";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCFunc)) {
      return "::mlc::Func";
    } else if (type_index == static_cast<int32_t>(MLCTypeIndex::kMLCStr)) {
      return "::mlc::Str";
    }
    const char *type_key = ::mlc::Lib::GetTypeKey(this->type_index);
    std::ostringstream oss;
    for (size_t i = 0, j = 0;; ++i) {
      char c = type_key[i];
      if (c == '.' || c == '\0') {
        // [j, i) is the current segment
        std::string_view segment(type_key + j, i - j);
        oss << "::" << segment;
        j = i + 1;
        if (c == '\0') {
          break;
        }
      }
    }
    return oss.str();
  }
  using MLCTypingAtomic::type_index;
  MLC_DEF_STATIC_TYPE(MLC_EXPORTS, AtomicTypeObj, TypeObj, MLCTypeIndex::kMLCTypingAtomic,
                      "mlc.core.typing.AtomicType");
};

struct AtomicType : public Type {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, AtomicType, AtomicTypeObj, Type)
      .Field("type_index", &MLCTypingAtomic::type_index, /*frozen=*/true)
      .StaticFn("__init__", InitOf<AtomicTypeObj, int32_t>)
      .MemFn("__str__", &AtomicTypeObj::__str__)
      .MemFn("__cxx_str__", &AtomicTypeObj::__cxx_str__);
  explicit AtomicType(int32_t type_index) : AtomicType(AtomicType::New(type_index)) {}
};

struct PtrTypeObj : protected MLCTypingPtr {
  explicit PtrTypeObj(Type ty) : MLCTypingPtr{} { this->TyMut() = ty; }
  ~PtrTypeObj() { this->TyMut().~Type(); }
  ::mlc::Str __str__() const {
    std::ostringstream os;
    os << "Ptr[" << this->Ty() << "]";
    return os.str();
  }
  ::mlc::Str __cxx_str__() const {
    ::mlc::Str ty_str = ::mlc::Lib::CxxStr(this->Ty());
    std::ostringstream os;
    os << ty_str->data() << "Obj *";
    return os.str();
  }
  MLC_DEF_STATIC_TYPE(MLC_EXPORTS, PtrTypeObj, TypeObj, MLCTypeIndex::kMLCTypingPtr, "mlc.core.typing.PtrType");

private:
  Type &TyMut() { return reinterpret_cast<Type &>(this->MLCTypingPtr::ty); }
  Type Ty() const { return Type(reinterpret_cast<const Ref<TypeObj> &>(this->MLCTypingPtr::ty)); }
};

struct PtrType : public Type {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, PtrType, PtrTypeObj, Type)
      .StaticFn("__init__", InitOf<PtrTypeObj, Type>)
      ._Field("ty", offsetof(MLCTypingPtr, ty), sizeof(MLCTypingPtr::ty), false, ParseType<Type>())
      .MemFn("__str__", &PtrTypeObj::__str__)
      .MemFn("__cxx_str__", &PtrTypeObj::__cxx_str__);
  explicit PtrType(Type ty) : PtrType(PtrType::New(ty)) {}
};

struct OptionalObj : protected MLCTypingOptional {
  explicit OptionalObj(Type ty) : MLCTypingOptional{} { this->TyMutable() = ty; }
  ~OptionalObj() { this->TyMutable().~Type(); }
  ::mlc::Str __str__() const {
    std::ostringstream os;
    os << this->Ty() << " | None";
    return os.str();
  }
  ::mlc::Str __cxx_str__() const {
    ::mlc::Str ty_str = ::mlc::Lib::CxxStr(this->Ty());
    std::ostringstream os;
    os << "::mlc::Optional<" << ty_str->data() << ">";
    return os.str();
  }
  MLC_DEF_STATIC_TYPE(MLC_EXPORTS, OptionalObj, TypeObj, MLCTypeIndex::kMLCTypingOptional, "mlc.core.typing.Optional");

private:
  Type &TyMutable() { return reinterpret_cast<Type &>(this->MLCTypingOptional::ty); }
  Type Ty() const { return Type(reinterpret_cast<const Ref<TypeObj> &>(this->MLCTypingOptional::ty)); }
};

struct Optional : public Type {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, Optional, OptionalObj, Type)
      .StaticFn("__init__", InitOf<OptionalObj, Type>)
      ._Field("ty", offsetof(MLCTypingOptional, ty), sizeof(MLCTypingOptional::ty), false, ParseType<Type>())
      .MemFn("__str__", &OptionalObj::__str__)
      .MemFn("__cxx_str__", &OptionalObj::__cxx_str__);
  explicit Optional(Type ty) : Optional(Optional::New(ty)) {}
};

struct ListObj : protected MLCTypingList {
  explicit ListObj(Type ty) : MLCTypingList{} { this->TyMutable() = ty; }
  ~ListObj() { this->TyMutable().~Type(); }
  ::mlc::Str __str__() const {
    std::ostringstream os;
    os << "list[" << this->Ty() << "]";
    return os.str();
  }
  ::mlc::Str __cxx_str__() const {
    ::mlc::Str ty_str = ::mlc::Lib::CxxStr(this->Ty());
    std::ostringstream os;
    os << "::mlc::List<" << ty_str->data() << ">";
    return os.str();
  }
  MLC_DEF_STATIC_TYPE(MLC_EXPORTS, ListObj, TypeObj, MLCTypeIndex::kMLCTypingList, "mlc.core.typing.List");

protected:
  Type &TyMutable() { return reinterpret_cast<Type &>(this->MLCTypingList::ty); }
  Type Ty() const { return Type(reinterpret_cast<const Ref<TypeObj> &>(this->MLCTypingList::ty)); }
};

struct List : public Type {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, List, ListObj, Type)
      .StaticFn("__init__", InitOf<ListObj, Type>)
      ._Field("ty", offsetof(MLCTypingList, ty), sizeof(MLCTypingList::ty), false, ParseType<Type>())
      .MemFn("__str__", &ListObj::__str__)
      .MemFn("__cxx_str__", &ListObj::__cxx_str__);
  explicit List(Type ty) : List(List::New(ty)) {}
};

struct DictObj : protected MLCTypingDict {
  explicit DictObj(Type ty_k, Type ty_v) : MLCTypingDict{} {
    this->TyKMut() = ty_k;
    this->TyVMut() = ty_v;
  }
  ~DictObj() {
    this->TyKMut().~Type();
    this->TyVMut().~Type();
  }
  ::mlc::Str __str__() const {
    std::ostringstream os;
    os << "dict[" << this->TyK() << ", " << this->TyV() << "]";
    return os.str();
  }
  ::mlc::Str __cxx_str__() const {
    ::mlc::Str k_str = ::mlc::Lib::CxxStr(this->TyK());
    ::mlc::Str v_str = ::mlc::Lib::CxxStr(this->TyV());
    std::ostringstream os;
    os << "::mlc::Dict<" << k_str->data() << ", " << v_str->data() << ">";
    return os.str();
  }
  MLC_DEF_STATIC_TYPE(MLC_EXPORTS, DictObj, TypeObj, MLCTypeIndex::kMLCTypingDict, "mlc.core.typing.Dict");

protected:
  Type &TyKMut() { return reinterpret_cast<Type &>(this->ty_k); }
  Type &TyVMut() { return reinterpret_cast<Type &>(this->ty_v); }
  Type TyK() const { return Type(reinterpret_cast<const Ref<TypeObj> &>(this->ty_k)); }
  Type TyV() const { return Type(reinterpret_cast<const Ref<TypeObj> &>(this->ty_v)); }
};

struct Dict : public Type {
  MLC_DEF_OBJ_REF(MLC_EXPORTS, Dict, DictObj, Type)
      .StaticFn("__init__", InitOf<DictObj, Type, Type>)
      ._Field("ty_k", offsetof(MLCTypingDict, ty_k), sizeof(MLCTypingDict::ty_k), false, ParseType<Type>())
      ._Field("ty_v", offsetof(MLCTypingDict, ty_v), sizeof(MLCTypingDict::ty_v), false, ParseType<Type>())
      .MemFn("__str__", &DictObj::__str__)
      .MemFn("__cxx_str__", &DictObj::__cxx_str__);
  explicit Dict(Type ty_k, Type ty_v) : Dict(Dict::New(ty_k, ty_v)) {}
};

} // namespace typing

template <typename T> struct TypeAnnParser {
  static typing::Type Run() {
    using namespace mlc::base;
    if constexpr (std::is_same_v<T, Any> || std::is_same_v<T, AnyView>) {
      return typing::AnyType();
    } else if constexpr (IsPOD<T>) {
      return typing::AtomicType(TypeTraits<T>::type_index);
    } else if constexpr (IsObjPtr<T>) {
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
