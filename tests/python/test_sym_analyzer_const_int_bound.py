from __future__ import annotations

import dataclasses

import pytest
from mlc import sym as S
from mlc.sym import truncdiv as tdiv
from mlc.sym import truncmod as tmod
from mlc.sym._internal import (
    ConstIntBound,
    const_int_bound,
    const_int_bound_update,
    enter_constraint,
)

POS_INF = 2**63 - 1
NEG_INF = -POS_INF


def pytest_generate_tests(metafunc: pytest.Metafunc) -> None:
    if "param" in metafunc.fixturenames:
        if test_cases := getattr(metafunc.cls, "param", None):
            metafunc.parametrize("param", test_cases)


@dataclasses.dataclass
class Param:
    expr: S.Expr
    expected: tuple[int | None, int | None]
    bounds: dict[S.Var, tuple[int, int]] = dataclasses.field(default_factory=dict)
    constraint: S.Expr | None = None

    @property
    def __name__(self) -> str:
        return str(self.expr).replace("\n", "; ")


class _Test:
    def test_body(self, param: Param) -> None:
        analyzer = S.Analyzer()
        for var, bounds in param.bounds.items():
            const_int_bound_update(analyzer, var, ConstIntBound(*bounds))
        with enter_constraint(analyzer, param.constraint):
            bounds = const_int_bound(analyzer, param.expr)
        expected_min_value, expected_max_value = param.expected
        if expected_min_value is not None:
            assert bounds.min_value == expected_min_value  # type: ignore[attr-defined]
        if expected_max_value is not None:
            assert bounds.max_value == expected_max_value  # type: ignore[attr-defined]


class TestDataType(_Test):
    param = (
        Param(S.Var("x", dtype="int64"), (NEG_INF, POS_INF)),
        Param(S.Var("x", dtype="int8"), (-128, 127)),
        Param(S.Var("x", dtype="uint8"), (0, 255)),
        Param(S.ShapeVar("x", dtype="int32"), (0, POS_INF)),
    )


class TestCastBound(_Test):
    x = S.Var("x", dtype="int8")

    param = (
        Param(tmod(x, 3).astype("uint32"), (0, 2)),
        Param(tmod(x, 3).astype("float32").astype("int32"), (-2, 2)),
    )


class TestAddSubBound(_Test):
    x = S.Var("x", "int64")
    y = S.Var("y", "int64")

    param = (
        Param(x + y, (NEG_INF, POS_INF)),
        Param(x + y, (1, 14), bounds={x: (0, 4), y: (1, 10)}),
        Param(x - y, (-10, 3), bounds={x: (0, 4), y: (1, 10)}),
        Param(x - y, (-10, POS_INF), bounds={x: (0, POS_INF), y: (1, 10)}),
        Param(1 - x, (NEG_INF, 1), bounds={x: (0, POS_INF), y: (1, 10)}),
    )


class TestMulBound(_Test):
    x = S.Var("x", "int64")
    y = S.Var("y", "int64")

    param = (
        Param(x * y + 20, (0, 60), {x: (-2, 4), y: (4, 10)}),
        Param(x * y, (-32, 24), {x: (-3, 4), y: (-8, 2)}),
        Param(x * y, (NEG_INF, POS_INF), {x: (NEG_INF, 4), y: (-8, 2)}),
    )


class TestTruncDivBound(_Test):
    x = S.Var("x")
    y = S.Var("y")

    param = (
        Param(tdiv(x, y), (-2, None), {x: (-9, 4), y: (4, 10)}),
        Param(tdiv(x, y), (-4, 9), {x: (-9, 4), y: (-2, 0)}),
        Param(tdiv(x, y), (NEG_INF, POS_INF), {x: (NEG_INF, 4), y: (-2, 1)}),
        Param(tdiv(x, y), (-9, 9), {x: (-9, 4), y: (-4, 12)}),
    )


class TestTruncModBound(_Test):
    x = S.Var("x")
    y = S.Var("y")

    param = (
        Param(tmod(x, y), (-9, 4), {x: (-9, 4), y: (4, 10)}),
        Param(tmod(x, y), (-9, 9), {x: (NEG_INF, POS_INF), y: (4, 10)}),
        Param(tmod(x, y), (0, 9), {x: (1, POS_INF), y: (4, 10)}),
    )


class TestFloorDivBound(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    ux = S.Var("x", dtype="uint32")
    uy = S.Var("y", dtype="uint32")

    param = (
        Param(x // y, (-9 // 4, None), {x: (-9, 4), y: (4, 10)}),
        Param(x // y, (-4, 9), {x: (-9, 4), y: (-2, 0)}),
        Param(x // y, (NEG_INF, POS_INF), {x: (NEG_INF, 4), y: (-2, 1)}),
        Param(x // y, (-9, 9), {x: (-9, 4), y: (-4, 12)}),
        Param(ux // uy, (0, 4), {ux: (1, 4), uy: (0, 12)}),
    )


class TestFloorModBound(_Test):
    x = S.Var("x")
    y = S.Var("y")

    param = (
        Param(x % y, (0, 9), {x: (-9, 4), y: (4, 10)}),
        Param(x % y, (0, 9), {x: (NEG_INF, POS_INF), y: (4, 10)}),
        Param(x % y, (0, 9), {x: (1, POS_INF), y: (4, 10)}),
    )


class TestMinMaxBound(_Test):
    x = S.Var("x")
    y = S.Var("y")

    param = (
        Param(S.min(x, y), (-9, 10), {x: (-9, 11), y: (4, 10)}),
        Param(S.min(x, y), (NEG_INF, 10), {x: (NEG_INF, POS_INF), y: (4, 10)}),
        Param(S.max(x, y), (4, POS_INF), {x: (NEG_INF, POS_INF), y: (4, 10)}),
        Param(S.max(x, y), (4, POS_INF), {x: (1, POS_INF), y: (4, 10)}),
    )


class TestSelectBound(_Test):
    x = S.Var("x")
    y = S.Var("y")

    param = (
        Param(
            S.select(x > 1, (y < 0).astype("int32"), y + 1),
            (0, 11),
            {x: (-9, 11), y: (4, 10)},
        ),
    )


class TestShiftAndBound(_Test):
    x = S.Var("x")
    y = S.Var("y")

    param = (
        Param(x >> y, (-3, 2), {x: (-9, 11), y: (2, 10)}),
        Param(x & y, (0, 10), {x: (-9, 11), y: (2, 10)}),
        Param(x & y, (0, 10), {x: (10, 11), y: (2, 10)}),
    )


class TestMixIndexBound(_Test):
    x = S.Var("x")
    y = S.Var("y")

    param = (
        Param(tmod(x, 8) + tdiv(x, 8) * 8, (0, 24 - 1), {x: (0, 24 - 1), y: (0, 3 - 1)}),
        Param(y + x * 3, (0, 24 * 3 - 1), {x: (0, 24 - 1), y: (0, 3 - 1)}),
        Param(tmod(x, 7) + tdiv(x, 7) * 7, (0, (23 // 7) * 7 + 6), {x: (0, 24 - 1), y: (0, 3 - 1)}),
    )


class TestLetBound(_Test):
    x = S.Var("x")
    param = (Param(S.let(x, 1, x + 1), (2, 2)),)


class TestFloorModNegativeDivisor(_Test):
    a = S.Var("a")
    b = S.Var("b")

    param = (Param(a % b, (-4, 6), {a: (0, 6), b: (-5, 7)}),)


class TestDivModAssumeNoZeroDivisor(_Test):
    a = S.Var("a")
    b = S.Var("b")

    param = (
        Param(a // b, (0, 6), {a: (0, 6), b: (0, POS_INF)}),
        Param(a % b, (0, 6), {a: (0, 6), b: (0, POS_INF)}),
    )


class TestMultipleCondition(_Test):
    a = S.Var("a")

    param = (
        Param(
            a % 58 - 1,
            (0, None),
            bounds={a: (0, 128)},
            constraint=S.logical_and(1 <= a % 58, a % 58 < 57),
        ),
    )


class TestBroadcastBound(_Test):
    a = S.Var("a")

    param = (
        Param(
            S.broadcast(a, 4),
            (0, 128),
            bounds={a: (0, 128)},
        ),
    )


class TestRampBound(_Test):
    a = S.Var("a")

    param = (
        Param(
            S.ramp(a, 2, 4) + 2,
            (2, 128 + 2 * 3 + 2),
            bounds={a: (0, 128)},
        ),
    )
