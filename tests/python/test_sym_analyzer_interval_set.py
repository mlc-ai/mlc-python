from __future__ import annotations

import dataclasses

import pytest
from mlc import sym as S
from mlc.sym import floordiv as fld
from mlc.sym import floormod as flm
from mlc.sym import truncdiv as tdiv
from mlc.sym import truncmod as tmod
from mlc.sym._internal import (
    ConstIntBound,
    IntervalSet,
    const_int_bound_update,
    interval_set,
)


def pytest_generate_tests(metafunc: pytest.Metafunc) -> None:
    if "param" in metafunc.fixturenames:
        if test_cases := getattr(metafunc.cls, "param", None):
            metafunc.parametrize("param", test_cases)


@dataclasses.dataclass
class Param:
    expr: S.Expr
    dom: dict[S.Var, IntervalSet]
    expected: tuple[S.Expr | int, S.Expr | int]
    bounds: dict[S.Var, tuple[int, int]] = dataclasses.field(default_factory=dict)
    analyzer_bounds: dict[S.Var, tuple[int, int]] = dataclasses.field(default_factory=dict)

    @property
    def __name__(self) -> str:
        return str(self.expr).replace("\n", "; ")


class _Test:
    def test_body(self, param: Param) -> None:
        analyzer = S.Analyzer()
        for var, bound in param.bounds.items():
            const_int_bound_update(analyzer, var, ConstIntBound(*bound))
        for var, bound in param.analyzer_bounds.items():
            assert var.dtype == "int32"
            min, extent = bound
            assert min == 0
            analyzer.bind(var, S.Range(_i32(min), _i32(extent)))
        int_set: IntervalSet = interval_set(
            analyzer=analyzer,
            expr=param.expr,
            dom_map=param.dom,
        )
        expected_min, expected_max = param.expected
        correct_min = analyzer.can_prove_equal(int_set.min_value, expected_min)
        correct_max = analyzer.can_prove_equal(int_set.max_value, expected_max)
        if not correct_min or not correct_max:
            raise AssertionError(
                f"Expected: [{expected_min}, {expected_max}]\n"
                f"Got: [{int_set.min_value}, {int_set.max_value}]\n"
                f"Expr: {param.expr}\n"
                f"Domain: {param.dom}"
            )


def _i32(value: int) -> S.IntImm:
    return S.IntImm(value, dtype="int32")


def test_constructor() -> None:
    s = IntervalSet(2, 3)
    assert s.min_value == 2
    assert s.max_value == 3

    s = IntervalSet(2, 2)
    assert s.min_value == 2
    assert s.max_value == 2


class TestAddSub(_Test):
    x = S.Var("x", "int32")
    y = S.Var("y", "int32")

    param = (
        Param(
            x + y,
            dom={x: IntervalSet(0, 10)},
            expected=(y, y + 10),
        ),
        Param(
            x + y,
            dom={x: IntervalSet(0, 10), y: IntervalSet(1, 11)},
            expected=(1, 21),
        ),
        Param(
            x - y,
            dom={x: IntervalSet(0, 10), y: IntervalSet(1, 11)},
            expected=(-11, 9),
        ),
    )


class TestMulDiv(_Test):
    x = S.Var("x", "int32")
    y = S.Var("y", "int32")

    param = (
        Param(x * 2, dom={x: IntervalSet(1, 10)}, expected=(2, 20)),
        Param(x * -2, dom={x: IntervalSet(1, 10)}, expected=(-20, -2)),
        Param(tdiv(x, 2), dom={x: IntervalSet(1, 10)}, expected=(0, 5)),
        Param(fld(x, 2), dom={x: IntervalSet(-1, 10)}, expected=(-1, 5)),
        Param(
            x * y,
            dom={x: IntervalSet(0, 10)},
            expected=(0, 10 * y),
            bounds={y: (1, 100)},
        ),
        Param(
            tdiv(x, y),
            dom={x: IntervalSet(0, 10)},
            expected=(0, tdiv(10, y)),
            bounds={y: (1, 100)},
        ),
        Param(
            fld(x, y),
            dom={x: IntervalSet(0, 10)},
            expected=(0, fld(10, y)),
            bounds={y: (1, 100)},
        ),
    )


class TestMod(_Test):
    x = S.Var("x", "int32")
    y = S.Var("y", "int32")
    z = S.Var("z", "int32")

    param = (
        Param(tmod(x, y), {x: IntervalSet(0, 10)}, (0, y - 1), bounds={y: (1, 100)}),
        Param(tmod(x, 10), {x: IntervalSet(1, 10)}, (0, 9)),
        Param(flm(x, 10), {x: IntervalSet(-10, 10)}, (0, 9)),
        Param(flm(x, 10), {x: IntervalSet(3, 5)}, (3, 5)),
        Param(flm(x, 10), {x: IntervalSet(13, 15)}, (3, 5)),
        Param(flm(x, 10), {x: IntervalSet(3, 15)}, (0, 9)),
        Param(flm(x, 10), {x: IntervalSet(3, 11)}, (0, 9)),
        Param(flm(x, 10), {x: IntervalSet(1, 21)}, (0, 9)),
        Param(
            flm(y, 8),
            dom={y: IntervalSet(z * 8 + x * 4, z * 8 + x * 4 + 3)},
            expected=(
                z * 8 + x * 4 - 8 * fld(z * 8 + x * 4, 8),
                z * 8 + x * 4 + 3 - 8 * fld(z * 8 + x * 4, 8),
            ),
        ),
        Param(
            flm(y, 8),
            {y: IntervalSet(z * 8 + x * 4, z * 8 + x * 4 + 3)},
            (x * 4, x * 4 + 3),
            analyzer_bounds={x: (0, 2)},
        ),
    )


class TestMaxMin(_Test):
    x = S.Var("x", "int32")
    y = S.Var("y", "int32")

    param = (
        Param(S.max(x, x + 1), {x: IntervalSet(0, 10)}, (1, 11)),
        Param(S.min(x - 1, x + 1), {x: IntervalSet(0, 10)}, (-1, 9)),
        Param(S.min(x, y), {}, (S.min(x, y), S.min(x, y))),
        Param(S.max(x, y), {}, (S.max(x, y), S.max(x, y))),
    )


class TestSelect(_Test):
    x = S.Var("x", "int32")

    param = (
        Param(
            expr=S.select(x > 0, x - 1, x + 1),
            dom={x: IntervalSet(0, 10)},
            expected=(-1, 11),
        ),
    )


# TODO: tests for region lower/upper bound estimates
