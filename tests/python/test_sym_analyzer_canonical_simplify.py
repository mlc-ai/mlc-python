from __future__ import annotations

import dataclasses
from collections.abc import Mapping
from types import MappingProxyType

import pytest
from mlc import sym as S
from mlc.sym import floordiv as fld
from mlc.sym import floormod as flm
from mlc.sym import truncdiv as tdiv
from mlc.sym import truncmod as tmod
from mlc.sym._internal import (
    ConstIntBound,
    canonical_simplify,
    const_int_bound_update,
)


def pytest_generate_tests(metafunc: pytest.Metafunc) -> None:
    if "param" in metafunc.fixturenames:
        if test_cases := getattr(metafunc.cls, "param", None):
            metafunc.parametrize("param", test_cases)


@dataclasses.dataclass
class Param:
    before: S.Expr
    after: S.Expr
    bounds: Mapping[S.Var, tuple[int, int]] = dataclasses.field(default_factory=dict)

    def __init__(
        self,
        before: S.Expr | bool,
        after: S.Expr | int,
        bounds: Mapping[S.Var, tuple[int, int]] | None = None,
    ) -> None:
        assert not isinstance(before, bool), "`before` should not be a bool"
        self.before = before
        if isinstance(after, int):
            after = _i32(after)
        self.after = after
        self.bounds = bounds if bounds is not None else {}

    @property
    def __name__(self) -> str:
        return str(self.before).replace("\n", "; ")


class _Test:
    def test_body(self, param: Param) -> None:
        analyzer = S.Analyzer()
        self._add_bound(analyzer, param)
        after = canonical_simplify(analyzer, param.before)
        if not param.after.eq_s(after):
            raise AssertionError(
                "CanonicalSimplify did not produce the expected result.\n"
                f"Before: {param.before}\n"
                f"Expected: {param.after}\n"
                f"Actual: {after}\n"
                f"Reason: {param.after.eq_s_fail_reason(after)}"
            )

    def _add_bound(self, analyzer: S.Analyzer, param: Param) -> None:
        for var, bounds in param.bounds.items():
            const_int_bound_update(analyzer, var, ConstIntBound(*bounds))


def _i32(value: int) -> S.IntImm:
    return S.IntImm(value, dtype="int32")


class TestMulSum(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.Var("z", dtype="int32")

    param = (
        Param(2 + (3 * x + z + y + 1) * 4 + x, x * 13 + z * 4 + y * 4 + 6),
        Param(x * 3 - 4 * x + 1, 1 - x),
        Param(y + x * 3 - 5 * x + 1 + y, y * 2 + 1 - x * 2),
        Param(tdiv(x + y + x + y * 3, 2), y * 2 + x),
        Param(tmod(x + y + x + y * 3, 2), 0),
        Param(flm(x + x + y * 3, 2), flm(y * 3, 2)),
        Param(fld(x + y + x + y * 3, 2), y * 2 + x),
        Param(flm(x + y + x + y * 3, 2), 0),
        Param(fld(x + x + y * 3, 2), fld(y * 3, 2) + x),
    )


class TestSplitIndex(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.Var("z", dtype="int32")

    param = (
        # split div const
        Param(tdiv(x, 3) * 3 + tmod(x, 3), x),
        Param(tdiv(x, 6) * 6 + tmod(tdiv(x, 3), 2) * 3 + tmod(x, 3), x),
        Param(tdiv(tdiv(tmod(x, 16), 2) * 2, 4), tdiv(tmod(x, 16), 4)),
        Param(tdiv(tmod(x, 2), 8), 0),
        Param(tdiv(tmod(x, 2), 7), 0),
        Param(tdiv(tdiv(tmod(x, 16), 2) * 2, 6), tdiv(tmod(x, 16), 6)),
        # split mod const
        Param(tmod((x * 8), 16), tmod(x, 2) * 8),
        Param(tmod(x * 8, 2), 0),
        # simplify then fold
        Param(
            tdiv(x * 4 + y, 2) * 2 + tmod(x * 4 + y, 2),
            x * 4 + y,
            bounds={x: (0, 1000), y: (0, 1000)},
        ),
        # complex fold
        Param(tdiv(z * 9 + y, 2) * 2 + tmod(z * 9 + y, 2), z * 9 + y),
        Param(
            tdiv(x * 4 + y, 2) * 2 + tmod(x * 4 + y, 2),
            x * 4 + y,
            bounds={x: (-100, 1000), y: (-100, 1000)},
        ),
        # floordiv
        Param(fld(x * 5, 2), fld(x * 5, 2)),
        Param(fld(x, 3) * 3 + flm(x, 3), x),
        Param(fld(x, 6) * 6 + flm(fld(x, 3), 2) * 3 + flm(x, 3), x),
        Param(fld(fld(flm(x, 16), 2) * 2, 4), fld(flm(x, 16), 4)),
        Param(fld(flm(x, 2), 8), 0),
        Param(fld(flm(x, 2), 7), 0),
        Param(fld(fld(flm(x, 16), 2) * 2, 6), fld(flm(x, 16), 6)),
        # cannot simplify mixed case, unless we canonicalize into one mode.
        Param(tdiv(x, 6) * 2 + tmod(fld(x, 3), 2), tdiv(x, 6) * 2 + tmod(fld(x, 3), 2)),
        Param(tmod(-x, 2), tmod(x, -2) * -1),
    )


class TestDiv(_Test):
    x = S.Var("x", dtype="int32")

    param = (
        # truc div
        Param(tdiv(16 + 48 * x, 16), x * 3 + 1),
        # (17 + 48 * x) / 16 is not simplifiable for arbitrary x
        # because when 17 + 48 * x < 0, (17 + 48 * x) / 16 != 1 + 3 * x
        Param(tdiv(17 + 48 * x, 16), tdiv(x * 48 + 17, 16)),
        # However, when x >= 0, then 17+48*x >= 0 and (17+48*x)/16 can be simplified
        # ck.analyzer.update(x, tvm.arith.ConstIntBound(0, 10))
        Param(tdiv(17 + 48 * x, 16), x * 3 + 1, bounds={x: (0, 10)}),
        # Trying expressions that are not simplifiable for any values of the variables
        Param(tdiv(17 + 47 * x, 16), tdiv(x * 47 + 17, 16)),
        # floordiv
        Param(fld(16 + 48 * x, 16), x * 3 + 1),
        Param(fld(17 + 48 * x, 16), x * 3 + 1),
        Param(fld(17 + 47 * x, 16), fld(x * 47 + 17, 16)),
    )


class TestFloorMod(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")

    param = (
        Param(flm(flm((x * 4) + y - 466036, 24528) - 24512, 16), flm((x * 4) + y + 12, 16)),
        Param(flm(flm((x * 4), 16), 8), flm(x, 2) * 4),
        Param(flm(-x, 2), flm(x, -2) * -1),
        Param(flm(x * 10 + 1 + y * 2 + 2, 2), 1),
    )


class TestCast(_Test):
    i = S.Var("i", dtype="int32")
    j = S.Var("j", dtype="int32")
    i_64 = S.Var("i", dtype="int64")
    j_64 = S.Var("j", dtype="int64")

    param = (
        Param(
            (i + j + 1).astype("int64") - i.astype("int64"),
            j.astype("int64") + 1,
        ),
        Param(
            (i_64 + j_64 + 1).astype("int32") - i_64.astype("int32"),
            j_64.astype("int32") + 1,
            bounds={i_64: (0, 10), j_64: (0, 10)},
        ),
        Param(
            (i_64 + j_64 - 100).astype("int32"),
            (i_64 + j_64 - 100).astype("int32"),
            bounds={i_64: (0, 2**31 - 1), j_64: (0, 10)},
        ),
        Param(
            (i_64 % 7 * 2 + 1).astype("int32") + 1 - (i_64 % 7 * 2).astype("int32"),
            2,
            bounds={i_64: (0, 42)},
        ),
    )


class TestMulDiv(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.Var("z", dtype="int32")

    param = (
        Param(flm(x * 32 * x, x), 0),
        Param(flm(z * x * 32 * x * y, x * z), 0),
        Param(flm(z * x * 32 * x * y, x * z * y * 8 * x), 0),
        Param(flm(z * x * 32 * (x * y), 6 * x * z), flm(x * y * 16, 3) * (x * z * 2)),
        Param(flm(x * 32 * x, x * z), flm(x * 32, z) * x),
        Param(tmod(x * 32 * x, x), 0),
        Param(tmod(z * x * 32 * x * y, x * z), 0),
        Param(tmod(z * x * 32 * (x * y), 6 * x * z), tmod(x * y * 16, 3) * (x * z * 2)),
        Param(tmod(x * 32 * x, x * z), tmod(x * 32, z) * x),
        Param(fld(x * 2 * x * z, 4 * x * x * x), fld(z, x * 2)),
        Param(fld(x * (2 * y) * 3, 3 * y), x * 2),
        Param(fld(x * (2 * y) * 3, 3 * y * z), fld(x * 2, z)),
        Param(tdiv(x * 2 * x * z, 4 * x * x * x), tdiv(z, x * 2)),
        Param(tdiv(x * (2 * y) * 3, 3 * y), x * 2),
        Param(tdiv(x * (2 * y) * 3, 3 * y * z), tdiv(x * 2, z)),
    )


class TestMinValue(_Test):
    x = S.Var("x", dtype="int32")

    param = (
        Param(S.min_value("int32") - x == 0, x == S.min_value("int32")),
        Param(S.min_value("int32") + x == 0, S.const("bool", False)),
        Param(0 == S.min_value("int32") - x, x == S.min_value("int32")),
        Param(0 == S.min_value("int32") + x, S.const("bool", False)),
        Param(-x + S.min_value("int32") == 0, x == S.min_value("int32")),
        Param(x + S.min_value("int32") == 0, S.const("bool", False)),
        Param(0 == -x + S.min_value("int32"), x == S.min_value("int32")),
        Param(0 == x + S.min_value("int32"), S.const("bool", False)),
    )


class TestIfThenElse(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")

    param = (
        Param(
            S.if_then_else(
                S.greater_equal(x * 4 + y, 466036),
                S.if_then_else(
                    S.less_equal(24512, tmod(x * 4 + y - 466036, 24528)),
                    tmod(tmod(x * 4 + y - 466036, 24528) - 24512, 16),
                    x,
                ),
                y,
            ),
            S.if_then_else(
                S.less_equal(466036, x * 4 + y),
                S.if_then_else(
                    S.less_equal(24512, tmod(x * 4 + y - 4, 24528)),
                    tmod(x * 4 + y - 4, 16),
                    x,
                ),
                y,
            ),
        ),
        Param(
            S.if_then_else(
                S.greater_equal(x * 4, 466036 - y),
                S.if_then_else(
                    S.less_equal(24512, tmod(x * 4 + y - 466036, 24528)),
                    tmod(tmod(x * 4 + y - 466036, 24528) - 24512, 16),
                    x,
                ),
                y,
            ),
            S.if_then_else(
                S.less_equal(466036, x * 4 + y),
                S.if_then_else(
                    S.less_equal(24512, tmod(x * 4 + y - 4, 24528)),
                    tmod(x * 4 + y - 4, 16),
                    x,
                ),
                y,
            ),
        ),
        Param(
            S.select(S.logical_and(x >= -1, y >= 0), tmod(x + y + 100, 3), tmod(x + 100, 3)),
            S.select(
                S.logical_and(S.less_equal(-1, x), S.less_equal(0, y)),
                tmod(x + y + 1, 3),
                tmod(x + 100, 3),
            ),
        ),
        Param(
            S.select(S.greater_equal(x, 10), S.if_then_else(tdiv(x, 3) > 2, x, 0), 0),
            S.select(S.less_equal(10, x), x, 0),
        ),
        Param(
            S.select(x >= 10, S.if_then_else(tdiv(x, 3) < 2, x, 0), 0),
            0,
        ),
    )


class TestMixedComplex(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.const("int32", 3)

    param = (
        Param(tdiv(x, (z * z)) - tdiv(x, (z * z)), 0),
        Param(tdiv(x, (z + z)) - tdiv(x, (z + z)), 0),
        Param(x - 2 < 3, x < 5),
        Param(S.max(x, 1) - S.max(x, 1), 0),
        Param(S.min(x, 1) - S.min(x, 1), 0),
        Param(x * x - x * x, 0),
        Param(tmod(tdiv(tmod(x, 20), 2) * 2, 4), tdiv(tmod(x, 4), 2) * 2),
        Param(fld(x, (z * z)) - fld(x, (z * z)), 0),
        Param(fld(x, (z + z)) - fld(x, (z + z)), 0),
        Param(
            tdiv(tdiv(tmod(x * 128 + y, 1296), 36) * 2 + 1, 2) * 36
            + tdiv(tmod((x * 128) + y, 36) * 2 + 1, 2)
            - tmod((x * 128) + y, 1296)
            + 1,
            1,
            bounds={x: (0, 5), y: (0, 127)},
        ),
        Param(
            tdiv(x * 1024 + y, 65536)
            + tdiv(tmod(x * 1024 + y, 65536), 256)
            + tdiv(tmod(x * 1024 + y, 256), 16)
            + tmod(x * 1024 + y, 16)
            - tdiv(y, 256)
            - tdiv(tmod(y, 256), 16)
            - tmod(y, 16)
            - (x * 4),
            tdiv((x * 1024) + y, 256) - tdiv(y, 256) - (x * 4),
            bounds={x: (0, 5), y: (0, 1024)},
        ),
    )


class TestFp16ConstFold(_Test):
    zero = S.const("float16", 0)
    one = S.const("float16", 1)
    half = S.const("float16", 0.5)

    param = (
        Param(zero + half, half),
        Param(half - zero, half),
        Param(zero * half, zero),
        Param(half * one, half),
        # TODO: recover
        # Param(divide(half, one), half),
        # Param(divide(zero, half), zero),
    )


class TestLE(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.Var("z", dtype="int32")
    n = S.ShapeVar("n", dtype="int32")
    param = (
        Param(x * 8 + y < 16, x < 2, bounds={y: (0, 8)}),
        Param(x * 8 + z * 4 < 16, x < 2, bounds={z: (0, 2)}),
        Param(x * -8 + y < 16, S.less(-2, x), bounds={y: (0, 8)}),
        Param(x * -8 + z * 4 < 16, S.less(-2, x), bounds={z: (0, 2)}),
        Param(
            x * 8 + y + z < 16,
            x * 8 + y + z < 16,
            bounds={
                y: (0, 8),
                z: (0, 2),
            },
        ),
        Param(
            x * 8 + y - z < 16,
            x < 2,
            bounds={
                y: (0, 8),
                z: (0, 2),
            },
        ),
        Param(x * 8 + y < n, x * 8 + y < n, bounds={y: (0, 8)}),
        Param(x * 1024 + y < z * 7168, x - z * 7 < 0, bounds={y: (0, 1024)}),
    )

    def _add_bound(self, analyzer: S.Analyzer, param: Param) -> None:
        for var, bounds in param.bounds.items():
            assert var.dtype == "int32"
            min, extent = bounds
            assert min == 0
            bound = S.Range(_i32(min), _i32(extent))
            analyzer.bind(var, bound)


class TestThreadBinding(_Test):
    x1 = S.Var("x1", dtype="int32")
    x2 = S.Var("x2", dtype="int32")
    tx = S.Var("tx", dtype="int32")
    ty = S.Var("ty", dtype="int32")
    vec = S.Var("vec", dtype="int32")

    bounds = MappingProxyType(
        {
            x1: (0, 2),
            x2: (0, 3),
            tx: (0, 32),
            ty: (0, 8),
            vec: (0, 8),
        }
    )
    param = (
        Param(
            x1 * 5632 + (((x2 * 8 + ty) * 32 + tx) * 8 + vec) % 5632 < 11008,
            x1 * 22 + (x2 * 8 + ty) % 22 < 43,
            bounds=bounds,
        ),
        Param(tx // 2 % 8 + vec < 8, tx % 16 // 2 + vec < 8, bounds=bounds),
    )

    def _add_bound(self, analyzer: S.Analyzer, param: Param) -> None:
        for var, bounds in param.bounds.items():
            assert var.dtype == "int32"
            min, extent = bounds
            assert min == 0
            bound = S.Range(_i32(min), _i32(extent))
            analyzer.bind(var, bound)
