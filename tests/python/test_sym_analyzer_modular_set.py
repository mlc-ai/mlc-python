from __future__ import annotations

import contextlib
import dataclasses

import pytest
from mlc import sym as S
from mlc.sym import truncdiv as tdiv
from mlc.sym import truncmod as tmod
from mlc.sym._internal import (
    ConstIntBound,
    const_int_bound_update,
    enter_constraint,
    modular_set,
)


def pytest_generate_tests(metafunc: pytest.Metafunc) -> None:
    if "param" in metafunc.fixturenames:
        if test_cases := getattr(metafunc.cls, "param", None):
            metafunc.parametrize("param", test_cases)


@dataclasses.dataclass
class Param:
    expr: S.Expr
    expected: tuple[int | None, int | None]
    known_bounds: dict[S.Var, tuple[int, int] | S.Expr] = dataclasses.field(default_factory=dict)
    constraints: list[S.Expr | bool] = dataclasses.field(default_factory=list)

    @property
    def __name__(self) -> str:
        return str(self.expr).replace("\n", "; ")


class _Test:
    def test_body(self, param: Param) -> None:
        analyzer = S.Analyzer()
        for var, bounds in param.known_bounds.items():
            if isinstance(bounds, S.Expr):
                analyzer.bind(var, bounds)
            else:
                const_int_bound_update(analyzer, var, ConstIntBound(*bounds))
        with contextlib.ExitStack() as exit_stack:
            for constraint in param.constraints:
                assert isinstance(constraint, S.Expr)
                exit_stack.enter_context(enter_constraint(analyzer, constraint))
            bounds = modular_set(analyzer, param.expr)
        expected_coeff, expected_base = param.expected
        if expected_coeff is not None:
            assert bounds.coeff == expected_coeff
        if expected_base is not None:
            assert bounds.base == expected_base


class TestCast(_Test):
    x = S.Var("x", dtype="int8")

    param = (
        Param((x * 3).astype("uint32"), (3, 0)),
        Param((x * 3 + 1).astype("float32").astype("int32"), (3, 1)),
    )


class TestAddSub(_Test):
    x = S.Var("x", "int64")
    y = S.Var("y", "int64")

    param = (
        Param(x * 6 + y * 4, (2, 0)),
        Param(1 - y, (4, 0), known_bounds={y: x * 4 + 1}),
    )


class TestMul(_Test):
    x = S.Var("x")
    y = S.Var("y")

    param = (Param((x * 4 + 2) * (y * 6 + 1), (4, 2)),)


class TestFloorMod(_Test):
    x = S.Var("x")
    y = S.Var("y")

    param = (Param((x * 128 + y * 4) % 256, (4, 0)),)


class TestDivShift(_Test):
    x = S.Var("x")

    param = (
        Param(tdiv(x * 4 + 2, 2), (1, 0)),
        # right shift always round down so it is fine
        Param((x * 4 + 2) >> 1, (2, 1)),
        Param((x * 4 + 2) // 2, (2, 1)),
        # x is non-negative
        Param(tdiv(x * 4 + 2, 2), (2, 1), known_bounds={x: (0, 100)}),
    )


class TestMod(_Test):
    x = S.Var("x")

    param = (
        # not sure if x is non-negative
        Param(tmod(x * 4 + 1, 4), (1, 0)),
        # no need to be positive if base == 0
        Param(tmod(x * 4, 4), (4, 0)),
        # floor mod tests
        Param((x * 4 + 3) % 2, (2, 1)),
        Param((x * 4 + 3) % 8, (4, 3)),
        # x is non-negative
        Param(tmod(x * 4 + 3, 2), (2, 1), known_bounds={x: (0, 100)}),
    )


class TestMinMaxSelect(_Test):
    x = S.Var("x")
    y = S.Var("y")

    param = (
        Param(S.min(x * 3, y * 9), (3, 0)),
        Param(S.max(x * 3 + 1, y * 9 + 4), (3, 1)),
        Param(S.select(x > 0, x * 3 + 1, y * 9 + 2), (1, 0)),
    )


class TestMixIndex(_Test):
    x = S.Var("x")
    y = S.Var("y")

    param = (
        Param(x * 4 + y * 6 + 7, (2, 1)),
        Param((x * 4 + 1) * (y * 8 + 3), (4, 3)),
        Param(tdiv(x * 4 + 1, y * 8 + 3), (1, 0)),
        Param((x * 4 + 1) * tdiv(y * 8, 4), (2, 0)),
        Param(x * 12 + S.min(y * 3 * 7, 2), (1, 0)),
    )


class TestBitwiseAnd(_Test):
    x = S.Var("x")
    y = S.Var("y")

    param = (
        Param((x * 16 + y * 4) & 31, (4, 0)),
        Param((x * 16 + y * 4) & 17, (1, 0)),
    )


class TestLet(_Test):
    x = S.Var("x")
    y = S.Var("y")

    param = (Param(S.let(x, y * 10, x + 1), (10, 1)),)


class TestConstraintScope(_Test):
    a = S.Var("a")
    b = S.Var("b")

    param = (
        Param(
            b + a * 2,
            (4, 0),
            constraints=[
                tmod(b, 4) == 2,
                tmod(a, 2) == 1,
            ],
        ),
        Param(
            b + a * 2,
            (2, 0),
            constraints=[
                tmod(b, 4) == 2,
            ],
        ),
        Param(b + 1, (1, 0)),
    )


class TestIntersect(_Test):
    a = S.Var("x")
    b = S.Var("y")

    param = (
        Param(
            a,
            (12, 1),
            constraints=[
                tmod(a, 4) == 1,
                tmod(a, 3) == 1,
            ],
        ),
        Param(
            a,
            (105, 23),
            constraints=[
                tmod(a, 3) == 2,
                tmod(a, 5) == 3,
                tmod(a, 7) == 2,
            ],
        ),
    )
