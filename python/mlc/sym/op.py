from __future__ import annotations

from typing import TYPE_CHECKING

from mlc.core import DataType, Func

if TYPE_CHECKING:
    from .expr import Broadcast, Expr, Let, Ramp, Select, Var


def _binary_op_args(a: Expr | int | float, b: Expr | int | float) -> tuple[Expr, Expr]:
    from .expr import Expr, const

    a_is_expr = isinstance(a, Expr)
    b_is_expr = isinstance(b, Expr)
    if not a_is_expr and not b_is_expr:
        raise TypeError(f"Can't infer dtype from `{type(a)=}` and `{type(b)=}")
    if a_is_expr and (not b_is_expr):
        assert isinstance(a, Expr)
        b = const(a.dtype, b)
    elif (not a_is_expr) and b_is_expr:
        assert isinstance(b, Expr)
        a = const(b.dtype, a)
    assert isinstance(a, Expr)
    assert isinstance(b, Expr)
    return a, b


def cast(dtype: DataType | str, expr: Expr) -> Expr:
    return _cast(dtype, expr)


def add(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _add(a, b)


def sub(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _sub(a, b)


def mul(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _mul(a, b)


def neg(a: Expr) -> Expr:
    return _neg(a)


def truncdiv(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _truncdiv(a, b)


def truncmod(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _truncmod(a, b)


def floordiv(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _floordiv(a, b)


def floormod(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _floormod(a, b)


def min(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _min(a, b)


def max(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _max(a, b)


def min_value(dtype: DataType | str) -> Expr:
    return _min_value(dtype)


def max_value(dtype: DataType | str) -> Expr:
    return _max_value(dtype)


def if_then_else(
    cond: Expr,
    true_value: Expr | int | float,
    false_value: Expr | int | float,
) -> Expr:
    true_value, false_value = _binary_op_args(true_value, false_value)
    return _if_then_else(cond, true_value, false_value)


def select(
    cond: Expr | bool,
    true_value: Expr | int | float,
    false_value: Expr | int | float,
) -> Select:
    assert not isinstance(cond, bool), "`cond` should be an Expr"
    true_value, false_value = _binary_op_args(true_value, false_value)
    return _select(cond, true_value, false_value)


def let(var: Var, value: Expr | int | float, body: Expr) -> Let:
    from .expr import Let, const

    if isinstance(value, (int, float)):
        value = const(var.dtype, value)
    return Let(body.dtype, var, value, body)


def ramp(
    base: Expr,
    stride: int | Expr,
    lanes: int,
    *,
    dtype: DataType | str | None = None,
) -> Ramp:
    from .expr import IntImm, Ramp

    if isinstance(stride, int):
        stride = IntImm(stride, base.dtype)
    if dtype is None:
        dtype = f"{base.dtype}x{lanes}"
    return Ramp(dtype, base, stride, lanes)


def broadcast(
    value: Expr,
    lanes: int,
    *,
    dtype: DataType | str | None = None,
) -> Broadcast:
    from .expr import Broadcast

    if dtype is None:
        dtype = f"{value.dtype}x{lanes}"
    return Broadcast(dtype, value, lanes)


def greater(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _greater(a, b)


def greater_equal(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _greater_equal(a, b)


def less(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _less(a, b)


def less_equal(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _less_equal(a, b)


def equal(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _equal(a, b)


def not_equal(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _not_equal(a, b)


def logical_and(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _logical_and(a, b)


def logical_or(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _logical_or(a, b)


def logical_not(a: Expr) -> Expr:
    return _logical_not(a)


def right_shift(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _right_shift(a, b)


def left_shift(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _left_shift(a, b)


def bitwise_and(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _bitwise_and(a, b)


def bitwise_or(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _bitwise_or(a, b)


def bitwise_xor(a: Expr | int | float, b: Expr | int | float) -> Expr:
    a, b = _binary_op_args(a, b)
    return _bitwise_xor(a, b)


def bitwise_neg(a: Expr) -> Expr:
    return _bitwise_neg(a)


def abs(a: Expr) -> Expr:
    return _abs(a)


_cast = Func.get("mlc.sym.op.cast")
_add = Func.get("mlc.sym.op.add")
_sub = Func.get("mlc.sym.op.sub")
_mul = Func.get("mlc.sym.op.mul")
_neg = Func.get("mlc.sym.op.neg")
_truncdiv = Func.get("mlc.sym.op.truncdiv")
_truncmod = Func.get("mlc.sym.op.truncmod")
_floordiv = Func.get("mlc.sym.op.floordiv")
_floormod = Func.get("mlc.sym.op.floormod")
_min = Func.get("mlc.sym.op.min")
_max = Func.get("mlc.sym.op.max")
_max_value = Func.get("mlc.sym.op.max_value")
_min_value = Func.get("mlc.sym.op.min_value")
_if_then_else = Func.get("mlc.sym.op.if_then_else")
_select = Func.get("mlc.sym.op.select")
_greater = Func.get("mlc.sym.op.greater")
_greater_equal = Func.get("mlc.sym.op.greater_equal")
_less = Func.get("mlc.sym.op.less")
_less_equal = Func.get("mlc.sym.op.less_equal")
_equal = Func.get("mlc.sym.op.equal")
_not_equal = Func.get("mlc.sym.op.not_equal")
_logical_and = Func.get("mlc.sym.op.logical_and")
_logical_or = Func.get("mlc.sym.op.logical_or")
_logical_not = Func.get("mlc.sym.op.logical_not")
_right_shift = Func.get("mlc.sym.op.right_shift")
_left_shift = Func.get("mlc.sym.op.left_shift")
_bitwise_and = Func.get("mlc.sym.op.bitwise_and")
_bitwise_or = Func.get("mlc.sym.op.bitwise_or")
_bitwise_xor = Func.get("mlc.sym.op.bitwise_xor")
_bitwise_neg = Func.get("mlc.sym.op.bitwise_neg")
_abs = Func.get("mlc.sym.op.abs")
