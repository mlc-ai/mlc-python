from __future__ import annotations

import typing

from mlc._cython import Ptr, c_class

from .object import Object


@c_class("mlc.core.typing.Type")
class Type(Object):
    def args(self) -> tuple[Type, ...]:
        raise NotImplementedError


@c_class("mlc.core.typing.AnyType")
class AnyType(Type):
    def __init__(self) -> None:
        self._mlc_init("__init__")

    def args(self) -> tuple:
        return ()


@c_class("mlc.core.typing.NoneType")
class NoneType(Type):
    def __init__(self) -> None:
        self._mlc_init("__init__")

    def args(self) -> tuple:
        return ()


@c_class("mlc.core.typing.AtomicType")
class AtomicType(Type):
    type_index: int

    def __init__(self, type_index: int) -> None:
        self._mlc_init("__init__", type_index)

    def args(self) -> tuple:
        return ()


@c_class("mlc.core.typing.PtrType")
class PtrType(Type):
    def __init__(self) -> None:
        self._mlc_init("__init__")

    @property
    def ty(self) -> Type:
        return self._C("_ty", self)

    def args(self) -> tuple:
        return (self.ty,)


@c_class("mlc.core.typing.Optional")
class Optional(Type):
    def __init__(self, ty: Type) -> None:
        self._mlc_init("__init__", ty)

    @property
    def ty(self) -> Type:
        return self._C("_ty", self)

    def args(self) -> tuple:
        return (self.ty,)


@c_class("mlc.core.typing.Union")
class Union(Type):
    num_types: int

    def __init__(self, *args: Type) -> None:
        self._mlc_init("__init__", *args)

    def ty(self, index: int) -> Type:
        return self._C("_ty", self, index)

    def args(self) -> tuple[Type, ...]:
        return tuple(self.ty(i) for i in range(self.num_types))


@c_class("mlc.core.typing.List")
class List(Type):
    def __init__(self, ty: Type) -> None:
        self._mlc_init("__init__", ty)

    @property
    def ty(self) -> Type:
        return self._C("_ty", self)

    def args(self) -> tuple:
        return (self.ty,)


@c_class("mlc.core.typing.Dict")
class Dict(Type):
    def __init__(self, key_ty: Type, value_ty: Type) -> None:
        self._mlc_init("__init__", key_ty, value_ty)

    @property
    def key(self) -> Type:
        return self._C("_key", self)

    @property
    def value(self) -> Type:
        return self._C("_value", self)

    def args(self) -> tuple:
        return (self.key, self.value)


_Any = AnyType()
_None = NoneType()
_INT = AtomicType(1)
_FLOAT = AtomicType(2)
_PTR = AtomicType(3)
_STR = AtomicType(32773)
_UList = List(_Any)
_UDict = Dict(_Any, _Any)

BUILTIN_TYPE_DICT: dict[typing.Any, Type] = {
    int: _INT,
    float: _FLOAT,
    Ptr: _PTR,
    str: _STR,
    typing.Any: _Any,
    Ellipsis: _Any,
    list: _UList,
    dict: _UDict,
    typing.List: _UList,  # noqa: UP006
    typing.Dict: _UDict,  # noqa: UP006
    type(None): _None,
    None: _None,
}


def from_py(ann: type) -> Type:
    if (ty := BUILTIN_TYPE_DICT.get(ann)) is not None:
        return ty
    elif (type_info := getattr(ann, "_mlc_type_info", None)) is not None:
        return AtomicType(type_info.type_index)
    elif (origin := typing.get_origin(ann)) is not None:
        from mlc.core import Dict as MLCDict
        from mlc.core import List as MLCList

        args = typing.get_args(ann)
        if (origin is list) or (origin is MLCList):
            if len(args) == 1:
                return List(from_py(args[0]))
            raise ValueError(f"Unsupported type: {ann}")
        elif (origin is dict) or (origin is MLCDict):
            if len(args) == 2:
                return Dict(from_py(args[0]), from_py(args[1]))
            raise ValueError(f"Unsupported type: {ann}")
        elif origin is tuple:
            raise ValueError("Unsupported type: `tuple`. Use `list` instead.")
    raise ValueError(f"Unsupported type: {ann}")
