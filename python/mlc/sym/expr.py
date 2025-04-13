from __future__ import annotations

from typing import Any

import mlc
import mlc.dataclasses as mlcd
from mlc._cython import DataTypeCode
from mlc.core import DataType, Object

from . import op


@mlcd.c_class("mlc.sym.Expr")
class Expr(Object):
    dtype: mlc.DataType

    def __add__(self, other: Expr | int | float) -> Expr:
        return op.add(self, other)

    def __radd__(self, other: Expr | int | float) -> Expr:
        return op.add(other, self)

    def __sub__(self, other: Expr | int | float) -> Expr:
        return op.sub(self, other)

    def __rsub__(self, other: Expr | int | float) -> Expr:
        return op.sub(other, self)

    def __mul__(self, other: Expr | int | float) -> Expr:
        return op.mul(self, other)

    def __rmul__(self, other: Expr | int | float) -> Expr:
        return op.mul(other, self)

    def __floordiv__(self, other: Expr | int | float) -> Expr:
        return op.floordiv(self, other)

    def __rfloordiv__(self, other: Expr | int | float) -> Expr:
        return op.floordiv(other, self)

    def __mod__(self, other: Expr | int | float) -> Expr:
        return op.floormod(self, other)

    def __rmod__(self, other: Expr | int | float) -> Expr:
        return op.floormod(other, self)

    def __neg__(self) -> Expr:
        return op.neg(self)

    def __lshift__(self, other: Expr | int | float) -> Expr:
        return op.left_shift(self, other)

    def __rlshift__(self, other: Expr | int | float) -> Expr:
        return op.left_shift(other, self)

    def __rshift__(self, other: Expr | int | float) -> Expr:
        return op.right_shift(self, other)

    def __rrshift__(self, other: Expr | int | float) -> Expr:
        return op.right_shift(other, self)

    def __and__(self, other: Expr | int | float) -> Expr:
        return op.bitwise_and(self, other)

    def __rand__(self, other: Expr | int | float) -> Expr:
        return op.bitwise_and(other, self)

    def __or__(self, other: Expr | int | float) -> Expr:
        return op.bitwise_or(self, other)

    def __ror__(self, other: Expr | int | float) -> Expr:
        return op.bitwise_or(other, self)

    def __xor__(self, other: Expr | int | float) -> Expr:
        return op.bitwise_xor(self, other)

    def __rxor__(self, other: Expr | int | float) -> Expr:
        return op.bitwise_xor(other, self)

    def __invert__(self) -> Expr:
        return op.bitwise_neg(self)

    def __eq__(self, other: object) -> Expr | bool:
        if isinstance(other, (Expr, int, float)):
            return op.equal(self, other)  # type: ignore[arg-type]
        return Object.eq_ptr(self, other)  # type: ignore[arg-type]

    def __ne__(self, other: object) -> Expr | bool:
        if isinstance(other, (Expr, int, float)):
            return op.not_equal(self, other)  # type: ignore[arg-type]
        return Object.eq_ptr(self, other)  # type: ignore[arg-type]

    def __lt__(self, other: Expr | int | float) -> Expr:
        return op.less(self, other)

    def __le__(self, other: Expr | int | float) -> Expr:
        return op.less_equal(self, other)

    def __gt__(self, other: Expr | int | float) -> Expr:
        return op.greater(self, other)

    def __ge__(self, other: Expr | int | float) -> Expr:
        return op.greater_equal(self, other)

    def __nonzero__(self) -> Expr:
        raise ValueError(
            "Cannot use `and` / `or` / `not` operator to S.Expr. Please instead use S.logical_{and|or|not}"
        )

    def __bool__(self) -> bool:
        return self.__nonzero__()

    def __hash__(self) -> int:
        return Object.__hash__(self)

    def equal(self, other: Expr | int | float) -> Expr:
        return op.equal(self, other)

    def cast(self, dtype: DataType | str) -> Expr:
        return op.cast(dtype, self)

    def astype(self, dtype: DataType | str) -> Expr:
        return self.cast(dtype)


@mlcd.c_class("mlc.sym.Var", init=False)
class Var(Expr):
    name: str

    def __init__(
        self,
        name: str,
        dtype: mlc.DataType | str = "int64",
    ) -> None:
        self._mlc_init(dtype, name)


@mlcd.c_class("mlc.sym.ShapeVar", init=False)
class ShapeVar(Var):
    def __init__(self, name: str, dtype: mlc.DataType | str = "int64") -> None:
        self._mlc_init(dtype, name)


@mlcd.c_class("mlc.sym.IntImm", init=False)
class IntImm(Expr):
    value: int

    def __init__(self, value: int, dtype: mlc.DataType | str = "int64") -> None:
        self._mlc_init(dtype, value)

    def __int__(self) -> int:
        return self.value

    def __float__(self) -> float:
        return float(self.value)


@mlcd.c_class("mlc.sym.BoolImm", init=False)
class BoolImm(IntImm):
    def __init__(self, value: bool) -> None:
        assert isinstance(value, bool)
        self._mlc_init("bool", int(value))

    def __int__(self) -> int:
        return self.value

    def __bool__(self) -> bool:
        return bool(self.value)

    def __float__(self) -> float:
        return float(self.value)


@mlcd.c_class("mlc.sym.FloatImm", init=False)
class FloatImm(Expr):
    value: float

    def __init__(self, value: int | float, dtype: mlc.DataType | str = "float64") -> None:
        if isinstance(value, int):
            value = float(value)
        assert isinstance(value, float)
        self._mlc_init(dtype, value)

    def __float__(self) -> float:
        return self.value


@mlcd.c_class("mlc.sym.Cast")
class Cast(Expr):
    value: Expr


@mlcd.c_class("mlc.sym.Add")
class Add(Expr):
    a: Expr
    b: Expr


@mlcd.c_class("mlc.sym.Sub")
class Sub(Expr):
    a: Expr
    b: Expr


@mlcd.c_class("mlc.sym.Mul")
class Mul(Expr):
    a: Expr
    b: Expr


@mlcd.c_class("mlc.sym.Div")
class Div(Expr):  # truncdiv
    a: Expr
    b: Expr


@mlcd.c_class("mlc.sym.Mod")
class Mod(Expr):
    a: Expr
    b: Expr


@mlcd.c_class("mlc.sym.FloorDiv")
class FloorDiv(Expr):
    a: Expr
    b: Expr


@mlcd.c_class("mlc.sym.FloorMod")
class FloorMod(Expr):
    a: Expr
    b: Expr


@mlcd.c_class("mlc.sym.Min")
class Min(Expr):
    a: Expr
    b: Expr


@mlcd.c_class("mlc.sym.Max")
class Max(Expr):
    a: Expr
    b: Expr


@mlcd.c_class("mlc.sym.EQ")
class EQ(Expr):
    a: Expr
    b: Expr


@mlcd.c_class("mlc.sym.NE")
class NE(Expr):
    a: Expr
    b: Expr


@mlcd.c_class("mlc.sym.LT")
class LT(Expr):
    a: Expr
    b: Expr


@mlcd.c_class("mlc.sym.LE")
class LE(Expr):
    a: Expr
    b: Expr


@mlcd.c_class("mlc.sym.GT")
class GT(Expr):
    a: Expr
    b: Expr


@mlcd.c_class("mlc.sym.GE")
class GE(Expr):
    a: Expr
    b: Expr


@mlcd.c_class("mlc.sym.And")
class And(Expr):
    a: Expr
    b: Expr


@mlcd.c_class("mlc.sym.Or")
class Or(Expr):
    a: Expr
    b: Expr


@mlcd.c_class("mlc.sym.Not")
class Not(Expr):
    a: Expr


@mlcd.c_class("mlc.sym.Select")
class Select(Expr):
    cond: Expr
    true_value: Expr
    false_value: Expr


@mlcd.c_class("mlc.sym.Ramp")
class Ramp(Expr):
    base: Expr
    stride: Expr
    lanes: int


@mlcd.c_class("mlc.sym.Broadcast")
class Broadcast(Expr):
    value: Expr
    lanes: int


@mlcd.c_class("mlc.sym.Shuffle")
class Shuffle(Expr):
    vectors: list[Expr]
    indices: list[Expr]


@mlcd.c_class("mlc.sym.Let")
class Let(Expr):
    var: Var
    value: Expr
    body: Expr


@mlcd.c_class("mlc.sym.Call")
class Call(Expr):
    op: Any
    args: list[Expr]


@mlcd.c_class("mlc.sym.Op")
class Op(mlcd.PyClass):
    name: str

    @staticmethod
    def get(name: str) -> Op:
        return Op._C(b"get", name)


@mlcd.c_class("mlc.sym.Range", init=False)
class Range(mlcd.PyClass):
    min: Expr
    extent: Expr

    def __init__(self, min: Expr | int, extent: Expr | int) -> None:
        if isinstance(min, int) and isinstance(extent, int):
            raise TypeError("Unable to infer dtype from integer values")
        if isinstance(min, int):
            assert isinstance(extent, Expr)
            min = IntImm(min, dtype=extent.dtype)
        if isinstance(extent, int):
            assert isinstance(min, Expr)
            extent = IntImm(extent, dtype=min.dtype)
        self._mlc_init(min, extent)

    @staticmethod
    def from_const(dtype: DataType | str, min: int, extent: int) -> Range:
        return Range(
            min=const(dtype, min),
            extent=const(dtype, extent),
        )


def const(dtype: DataType | str, a: Expr | int | float) -> Expr:
    from .expr import BoolImm, FloatImm, IntImm

    if isinstance(dtype, str):
        dtype = DataType(dtype)
    dtype = DataType.from_triple(dtype.code, dtype.bits, lanes=1)
    if isinstance(a, bool):
        if not (dtype.code == DataTypeCode.uint and dtype.bits == 1):
            raise TypeError(f"Expected `dtype=bool`, but got: {dtype}")
        return BoolImm(value=a)
    if dtype.code in [DataTypeCode.float, DataTypeCode.bfloat]:
        if not isinstance(a, (int, float)):
            raise TypeError(f"Expected int/float to be converted to dtype `{dtype}`, but got: {a}")
        return FloatImm(dtype=dtype, value=float(a))
    if dtype.code in [DataTypeCode.int, DataTypeCode.uint]:
        if not isinstance(a, int):
            raise TypeError(f"Expected int to be converted to dtype `{dtype}`, but got: {a}")
        return IntImm(dtype=dtype, value=a)
    raise TypeError(f"Unsupport conversion from `{type(a)}` to `{dtype}`")
