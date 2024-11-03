from __future__ import annotations

import ctypes
import typing

from mlc._cython import DLDataType, DLDevice, MLCAny, MLCObjPtr, Ptr, type_cast
from mlc.dataclasses import c_class

from .object import Object


@c_class("mlc.core.typing.Type")
class Type(Object):
    def args(self) -> tuple[Type, ...]:
        raise NotImplementedError

    def cast(self, obj: typing.Any) -> typing.Any:
        return type_cast(self, obj)

    def _ctype(self) -> typing.Any:
        raise NotImplementedError


@c_class("mlc.core.typing.AnyType")
class AnyType(Type):
    def __init__(self) -> None:
        self._mlc_init("__init__")

    def args(self) -> tuple:
        return ()

    def _ctype(self) -> typing.Any:
        return MLCAny


@c_class("mlc.core.typing.AtomicType")
class AtomicType(Type):
    type_index: int

    def __init__(self, type_index: int) -> None:
        self._mlc_init("__init__", type_index)

    def args(self) -> tuple:
        return ()

    def _ctype(self) -> typing.Any:
        type_index = self.type_index
        if type_index >= _kMLCStaticObjectBegin:
            return Ptr
        if (ret := _TYPE_INDEX_TO_CTYPES.get(type_index)) is not None:
            return ret
        raise ValueError(f"Unsupported type index: {type_index}")


@c_class("mlc.core.typing.PtrType")
class PtrType(Type):
    def __init__(self) -> None:
        self._mlc_init("__init__")

    @property
    def ty(self) -> Type:
        return self._C("_ty", self)

    def args(self) -> tuple:
        return (self.ty,)

    def _ctype(self) -> typing.Any:
        return MLCObjPtr


@c_class("mlc.core.typing.Optional")
class Optional(Type):
    def __init__(self, ty: Type) -> None:
        self._mlc_init("__init__", ty)

    @property
    def ty(self) -> Type:
        return self._C("_ty", self)

    def args(self) -> tuple:
        return (self.ty,)

    def _ctype(self) -> typing.Any:
        return MLCObjPtr


@c_class("mlc.core.typing.List")
class List(Type):
    def __init__(self, ty: Type) -> None:
        self._mlc_init("__init__", ty)

    @property
    def ty(self) -> Type:
        return self._C("_ty", self)

    def args(self) -> tuple:
        return (self.ty,)

    def _ctype(self) -> typing.Any:
        return MLCObjPtr


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

    def _ctype(self) -> typing.Any:
        return MLCObjPtr


def from_py(ann: type) -> Type:
    if (ty := _PY_TYPE_TO_MLC_TYPE.get(ann)) is not None:
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


_kMLCInt = 1
_kMLCFloat = 2
_kMLCPtr = 3
_kMLCDataType = 4
_kMLCDevice = 5
_kMLCRawStr = 6
_kMLCStaticObjectBegin = 1000
_kMLCStr = 1005
_Any = AnyType()
_INT = AtomicType(_kMLCInt)
_FLOAT = AtomicType(_kMLCFloat)
_PTR = AtomicType(_kMLCPtr)
_STR = AtomicType(_kMLCStr)
_UList = List(_Any)
_UDict = Dict(_Any, _Any)
_TYPE_INDEX_TO_CTYPES = {
    _kMLCInt: ctypes.c_int64,
    _kMLCFloat: ctypes.c_double,
    _kMLCPtr: Ptr,
    _kMLCDataType: DLDataType,
    _kMLCDevice: DLDevice,
    _kMLCRawStr: Ptr,
}
_PY_TYPE_TO_MLC_TYPE: dict[typing.Any, Type] = {
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
}