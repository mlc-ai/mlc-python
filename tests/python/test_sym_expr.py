import itertools
from collections.abc import Callable

import pytest
from mlc import sym as S


def test_op() -> None:
    op = S.Op.get("mlc.S.if_then_else")
    assert str(op) == 'S.Op("mlc.S.if_then_else")'


@pytest.mark.parametrize(
    "x, expected",
    [
        [S.Var("x", "int64"), 'S.int64("x")'],
        [S.Var("y", "int32"), 'S.int32("y")'],
    ],
    ids=itertools.count(),
)
def test_var(x: S.Var, expected: str) -> None:
    assert str(x) == expected


@pytest.mark.parametrize(
    "x, expected",
    [
        [S.ShapeVar("x", "int64"), 'S.int64("x", size_var=True)'],
        [S.ShapeVar("y", "int32"), 'S.int32("y", size_var=True)'],
    ],
    ids=itertools.count(),
)
def test_size_var(x: S.Var, expected: str) -> None:
    assert str(x) == expected


@pytest.mark.parametrize(
    "x, expected",
    [
        [S.IntImm(-1, "int64"), "-1"],
        [S.IntImm(1, "int64"), "1"],
        [S.IntImm(65536, "int32"), "65536"],  # TODO: detect overflow
    ],
    ids=itertools.count(),
)
def test_int_imm(x: S.IntImm, expected: str) -> None:
    assert str(x) == expected


@pytest.mark.parametrize(
    "x, expected",
    [
        [S.BoolImm(True), "True"],
        [S.BoolImm(False), "False"],
    ],
    ids=itertools.count(),
)
def test_bool_imm(x: S.IntImm, expected: str) -> None:
    assert str(x) == expected


@pytest.mark.parametrize(
    "x, expected",
    [
        [S.FloatImm(0, "float64"), "0.0"],
        [S.FloatImm(1, "float64"), "1.0"],
        [S.FloatImm(-1, "float64"), "-1.0"],
    ],
    ids=itertools.count(),
)
def test_float_imm(x: S.FloatImm, expected: str) -> None:
    assert str(x) == expected


def test_cast() -> None:
    x = S.Var("x", "int64")
    y = x.cast("int32")
    assert str(y) == 'x.cast("int32")'


@pytest.mark.parametrize(
    "op, expected",
    [
        [lambda x, y: x + y, "x + y"],
        [lambda x, y: x - y, "x - y"],
        [lambda x, y: x * y, "x * y"],
        [lambda x, y: S.truncdiv(x, y), "S.truncdiv(x, y)"],
        [lambda x, y: S.truncmod(x, y), "S.truncmod(x, y)"],
        [lambda x, y: x // y, "x // y"],
        [lambda x, y: x % y, "x % y"],
        [lambda x, y: S.min(x, y), "S.min(x, y)"],
        [lambda x, y: S.max(x, y), "S.max(x, y)"],
    ],
    ids=itertools.count(),
)
def test_arith_binary(op: Callable[[S.Expr, S.Expr], S.Expr], expected: str) -> None:
    x = S.Var("x", "int64")
    y = S.Var("y", "int64")
    z = op(x, y)
    assert str(z) == expected


@pytest.mark.parametrize(
    "op, expected",
    [
        [lambda x, y: x == y, "x == y"],
        [lambda x, y: x != y, "x != y"],
        [lambda x, y: x >= y, "x >= y"],
        [lambda x, y: x <= y, "x <= y"],
        [lambda x, y: x > y, "x > y"],
        [lambda x, y: x < y, "x < y"],
    ],
    ids=itertools.count(),
)
def test_cmp(op: Callable[[S.Expr, S.Expr], S.Expr], expected: str) -> None:
    x = S.Var("x", "int64")
    y = S.Var("y", "int64")
    z = op(x, y)
    assert z.dtype == "bool"
    assert str(z) == expected


def test_logical_and() -> None:
    x = S.Var("x", "bool")
    y = S.Var("y", "bool")
    z = S.logical_and(x, y)
    assert str(z) == "x and y"


def test_logical_or() -> None:
    x = S.Var("x", "bool")
    y = S.Var("y", "bool")
    z = S.logical_or(x, y)
    assert str(z) == "x or y"


def test_logical_not() -> None:
    x = S.Var("x", "bool")
    y = S.logical_not(x)
    assert str(y) == "not x"


def test_select() -> None:
    cond = S.Var("cond", "bool")
    true_value = S.Var("true_value", "int64")
    false_value = S.Var("false_value", "int64")
    z = S.select(cond, true_value, false_value)
    assert str(z) == "S.select(cond, true_value, false_value)"


def test_let() -> None:
    x = S.Var("x", "int64")
    y = S.Var("y", "int64")
    z = S.let(
        var=y,
        value=x + 1,
        body=x + y,
    )
    assert (
        str(z).strip()
        == """
y = x + 1
x + y
""".strip()
    )


def test_ramp() -> None:
    tx = S.Var("tx", "int64")
    ty = S.Var("ty", "int64")
    tz = S.Var("tz", "int64")
    x = S.ramp(
        ty * 512 + tz * 256 + tx * 8 + 2048,
        stride=S.IntImm(1, "int64"),
        lanes=8,
    )
    assert str(x) == "S.ramp(ty * 512 + tz * 256 + tx * 8 + 2048, 1, 8)"
    assert x.dtype == "int64x8"


def test_broadcast() -> None:
    value = S.Var("value", "int64")
    x = S.broadcast(value, lanes=8)
    assert str(x) == "S.broadcast(value, 8)"
    assert x.dtype == "int64x8"


def test_range() -> None:
    min = S.Var("min", "int64")
    extent = S.Var("extent", "int64")
    x = S.Range(min, extent)
    assert str(x) == "S.Range(min, extent)"
