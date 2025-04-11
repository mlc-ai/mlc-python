from __future__ import annotations

import dataclasses
import typing

import pytest
from mlc import sym as S
from mlc.sym import floordiv as fld
from mlc.sym import floormod as flm
from mlc.sym import truncdiv as tdiv
from mlc.sym import truncmod as tmod
from mlc.sym._internal import (
    enter_constraint,
    rewrite_simplify,
)


def pytest_generate_tests(metafunc: pytest.Metafunc) -> None:
    if "param" in metafunc.fixturenames:
        if test_cases := getattr(metafunc.cls, "param", None):
            metafunc.parametrize("param", test_cases)


@dataclasses.dataclass
class Param:
    before: S.Expr
    after: S.Expr
    constraints: S.Expr | None = None

    def __init__(
        self,
        before: S.Expr | bool,
        after: S.Expr | int,
        constraints: list[S.Expr] | None = None,
    ) -> None:
        assert not isinstance(before, bool)
        self.before = before
        if isinstance(after, int):
            after = _i32(after)
        self.after = after
        if constraints is None:
            self.constraints = None
        elif isinstance(constraints, S.Expr):
            self.constraints = constraints
        else:
            self.constraints = constraints[0]
            for constraint in constraints[1:]:
                self.constraints = S.logical_and(self.constraints, constraint)

    @property
    def __name__(self) -> str:
        return str(self.before).replace("\n", "; ")


class _Test:
    def test_body(self, param: Param) -> None:
        analyzer = S.Analyzer()
        with enter_constraint(analyzer, param.constraints):
            after = rewrite_simplify(analyzer, param.before)
        if not param.after.eq_s(after):
            raise AssertionError(
                "RewriteSimplify did not produce the expected result.\n"
                f"Before: {param.before}\n"
                f"Expected: {param.after}\n"
                f"Actual: {after}\n"
                f"Reason: {param.after.eq_s_fail_reason(after)}"
            )


def _const(value: int | float, dtype: str) -> S.Expr:
    if dtype == "bool":
        value = bool(value)
    return S.const(a=value, dtype=dtype)


def _i32(value: int) -> S.IntImm:
    return S.IntImm(value, dtype="int32")


def _lt(a: S.Expr | int, b: S.Expr | int) -> S.Expr:
    return S.less(a, b)


def _eq(a: S.Expr | int, b: S.Expr | int) -> S.Expr:
    return S.equal(a, b)


def _ne(a: S.Expr | int, b: S.Expr | int) -> S.Expr:
    return S.not_equal(a, b)


def _le(a: S.Expr | int, b: S.Expr | int) -> S.Expr:
    return S.less_equal(a, b)


def _ge(a: S.Expr | int, b: S.Expr | int) -> S.Expr:
    return S.greater_equal(a, b)


def _gt(a: S.Expr | int, b: S.Expr | int) -> S.Expr:
    return S.greater(a, b)


class TestVector(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.Var("z", dtype="int32")
    x64 = S.Var("x", dtype="int64")
    vx = S.Var("vx", dtype="int32x2")
    vc = S.Var("vc", dtype="uint1")

    param = (
        # Add rules
        Param(S.ramp(x, 1, 4) + S.ramp(y, 2, 4), S.ramp(x + y, 3, 4)),
        Param(S.ramp(x, 1, 2) + y, S.ramp(x + y, 1, 2)),
        Param(y + S.ramp(x, 1, 2), S.ramp(y + x, 1, 2)),
        Param(y.astype("int32x2") + x.astype("int32x2"), (y + x).astype("int32x2")),
        Param(S.broadcast(_i32(0), 4) + y, S.broadcast(y, 4)),
        # int64 iterators with int32 lanes
        Param(
            S.broadcast(x64, 4) + S.ramp(S.IntImm(0, dtype="int64"), 1, 4),
            S.ramp(x64, 1, 4),
        ),
        Param(
            S.ramp(x, 1, 4).astype("float32x4") + S.broadcast(S.FloatImm(0.0, "float32"), 4),
            S.ramp(x, 1, 4).astype("float32x4"),
        ),
        # Sub rules
        Param(S.ramp(x, 4, 4) - S.ramp(y, 2, 4), S.ramp(x - y, 2, 4)),
        Param(S.ramp(x, 1, 2) - y, S.ramp(x - y, 1, 2)),
        Param(y - S.ramp(x, 1, 2), S.ramp(y - x, -1, 2)),
        Param(y.astype("int32x2") - x.astype("int32x2"), (y - x).astype("int32x2")),
        # Mul rules
        Param(y.astype("int32x2") * x.astype("int32x2"), (y * x).astype("int32x2")),
        Param(S.ramp(x, 4, 4) * 2, S.ramp(x * 2, 8, 4)),
        Param(2 * S.ramp(x, 4, 4), S.ramp(x * 2, 8, 4)),
        Param(S.broadcast(_i32(0), 4) * x, S.broadcast(_i32(0), 4)),
        Param(
            S.broadcast(S.FloatImm(0.0, "float32"), 4) * x,
            S.broadcast(S.FloatImm(0.0, "float32"), 4),
        ),
        ## DivMod rules
        # trunc div
        Param(tdiv(y.astype("int32x2"), x.astype("int32x2")), tdiv(y, x).astype("int32x2")),
        Param(tdiv(S.ramp(x, 4, 4), 2), S.ramp(tdiv(x, 2), 2, 4)),
        Param(tdiv(S.ramp(x * 8 + 1, 1, 4), 8), x.astype("int32x4"), x >= 0),
        Param(tdiv(S.ramp(x * 8 + 15, 1, 4), 8), tdiv(S.ramp(x * 8 + 15, 1, 4), 8)),
        # trunc mod
        Param(tmod(y.astype("int32x2"), x.astype("int32x2")), tmod(y, x).astype("int32x2")),
        Param(tmod(S.ramp(x, 4, 4), 2), S.broadcast(tmod(x, 2), 4)),
        Param(tmod(S.ramp(x * 8 + 1, 1, 4), 8), S.ramp(_i32(1), 1, 4), x >= 0),
        Param(tmod(S.ramp(x * 8 + 1, 15, 4), 8), tmod(S.ramp(_i32(1), 15, 4), 8), x >= 0),
        # floor div
        Param(fld(y.astype("int32x2"), x.astype("int32x2")), fld(y, x).astype("int32x2")),
        Param(fld(S.ramp(x, 4, 4), 2), S.ramp(fld(x, 2), 2, 4)),
        Param(fld(S.ramp(x * 8 + 1, 1, 4), 8), (x).astype("int32x4")),
        Param(fld(S.ramp(x * 8 + 15, 1, 4), 8), fld(S.ramp(x * 8 + 15, 1, 4), 8)),
        Param(fld(S.ramp(x, 8, 5), S.broadcast(_i32(4), 5)), S.ramp(fld(x, 4), 2, 5)),
        Param(
            fld(S.ramp(flm(x * 4, 256), 1, 4), S.broadcast(_i32(8), 4)),
            S.broadcast(fld(flm(x * 4, 256), 8), 4),
        ),
        Param(
            fld(S.ramp(x, 7, 4), S.broadcast(_i32(4), 4)),
            fld(S.ramp(x, 7, 4), S.broadcast(_i32(4), 4)),
        ),
        Param(fld(S.ramp(x * 8, 1, 4), S.broadcast(_i32(4), 4)), S.broadcast(x * 2, 4)),
        Param(
            fld(S.ramp(x * 8, 3, 4), S.broadcast(_i32(4), 4)),
            fld(S.ramp(x * 8, 3, 4), S.broadcast(_i32(4), 4)),
        ),
        Param(
            fld(S.ramp(x * 8 + 15, 1, 4), S.broadcast(_i32(4), 4)),
            fld(S.ramp(x * 8 + 15, 1, 4), S.broadcast(_i32(4), 4)),
        ),
        Param(
            fld(S.ramp(x * 4, 1, 4), S.broadcast(_i32(64), 4)),
            S.broadcast(fld(x, 16), 4),
        ),
        Param(
            fld(S.ramp(x * 8, 2, 4), S.broadcast(_i32(64), 4)),
            S.broadcast(fld(x, 8), 4),
        ),
        Param(
            fld(S.ramp(x * 4, 1, 5), S.broadcast(_i32(64), 5)),
            fld(S.ramp(x * 4, 1, 5), S.broadcast(_i32(64), 5)),
        ),  # Example negative case: x = 15; [60, 61, 62, 63, 64] / 64 = [0, 0, 0, 0, 1]
        Param(
            fld(S.ramp(x * 4 + 3, 1, 4), S.broadcast(_i32(64), 4)),
            fld(S.ramp(x * 4 + 3, 1, 4), S.broadcast(_i32(64), 4)),
        ),  # Example negative case: x = 15; [63, 64, 65, 66] % 64 = [0, 1, 1, 1]
        Param(
            fld(S.ramp(x * 7, 1, 4), S.broadcast(_i32(64), 4)),
            fld(S.ramp(x * 7, 1, 4), S.broadcast(_i32(64), 4)),
        ),  # Example negative case: x = 9; [63, 70, 77, 84] % 64 = [0, 1, 1, 1]
        # floor mod
        Param(flm(y.astype("int32x2"), x.astype("int32x2")), flm(y, x).astype("int32x2")),
        Param(flm(S.ramp(x, 4, 4), 2), S.broadcast(flm(x, 2), 4)),
        Param(flm(S.ramp(x * 8 + 1, 1, 4), 8), S.ramp(_i32(1), 1, 4)),
        Param(flm(S.ramp(x * 8 + 1, 15, 4), 8), flm(S.ramp(_i32(1), 15, 4), 8)),
        Param(flm(S.ramp(x, 8, 4), S.broadcast(_i32(4), 4)), S.broadcast(flm(x, 4), 4)),
        Param(
            flm(S.ramp(x, 7, 4), S.broadcast(_i32(4), 4)),
            flm(S.ramp(x, 7, 4), S.broadcast(_i32(4), 4)),
        ),
        Param(flm(S.ramp(x * 8, 1, 4), S.broadcast(_i32(4), 4)), S.ramp(_i32(0), 1, 4)),
        Param(
            flm(S.ramp(x * 8, 1, 5), S.broadcast(_i32(4), 5)),
            flm(S.ramp(_i32(0), 1, 5), S.broadcast(_i32(4), 5)),
        ),
        Param(
            flm(S.ramp(x * 8 + 7, 1, 4), S.broadcast(_i32(4), 4)),
            flm(S.ramp(_i32(3), 1, 4), S.broadcast(_i32(4), 4)),
        ),
        Param(
            flm(S.ramp(x * 4, 1, 4), S.broadcast(_i32(64), 4)),
            S.ramp(flm(x * 4, 64), 1, 4),
        ),
        Param(
            flm(S.ramp(x * 8, 2, 4), S.broadcast(_i32(64), 4)),
            S.ramp(flm(x * 8, 64), 2, 4),
        ),
        Param(
            flm(S.ramp(x * 4, 1, 5), S.broadcast(_i32(64), 5)),
            flm(S.ramp(x * 4, 1, 5), S.broadcast(_i32(64), 5)),
        ),  # Example negative case: x = 15; [60, 61, 62, 63, 64] % 64 = [60, 61, 62, 63, 0]
        Param(
            flm(S.ramp(x * 4 + 3, 1, 4), S.broadcast(_i32(64), 4)),
            flm(S.ramp(x * 4 + 3, 1, 4), S.broadcast(_i32(64), 4)),
        ),  # Example negative case: x = 15; [63, 64, 65, 66] % 64 = [63, 0, 1, 2]
        Param(
            flm(S.ramp(x * 2, 1, 8), S.broadcast(_i32(20), 8)),
            flm(S.ramp(x * 2, 1, 8), S.broadcast(_i32(20), 8)),
        ),  # Example negative case: x = 9; [18, 19, 20, ..., 25] % 20 = [18, 19, 0, 1, ..., 5]
        Param(
            flm(S.ramp(x * 7, 1, 4), S.broadcast(_i32(64), 4)),
            flm(S.ramp(x * 7, 1, 4), S.broadcast(_i32(64), 4)),
        ),  # Example negative case: x = 9; [63, 70, 77, 84] % 64 = [63, 6, 13, 20]
        # Min/Max rules
        Param(S.min(y.astype("int32x2"), x.astype("int32x2")), S.min(y, x).astype("int32x2")),
        Param(
            S.min(S.min(vx, y.astype("int32x2")), x.astype("int32x2")),
            S.min(vx, S.min(y, x).astype("int32x2")),
        ),
        Param(
            S.max(y.astype("int32x2"), x.astype("int32x2")),
            S.max(y, x).astype("int32x2"),
        ),
        Param(
            S.max(S.max(vx, y.astype("int32x2")), x.astype("int32x2")),
            S.max(vx, S.max(y, x).astype("int32x2")),
        ),
        ## Logical rules
        Param(
            y.astype("int32x2").equal(x.astype("int32x2")),
            (y.equal(x)).astype("uint1x2"),
        ),
        Param(
            y.astype("int32x2") != (x.astype("int32x2")),  # type: ignore[arg-type]
            (y != x).astype("uint1x2"),  # type: ignore[union-attr]
        ),
        Param(y.astype("int32x2") > x.astype("int32x2"), (x < y).astype("uint1x2")),
        Param(y.astype("int32x2") >= x.astype("int32x2"), (x <= y).astype("uint1x2")),
        Param(y.astype("int32x2") < x.astype("int32x2"), (y < x).astype("uint1x2")),
        Param(y.astype("int32x2") <= x.astype("int32x2"), (y <= x).astype("uint1x2")),
        Param(
            S.logical_and(y.astype("int32x2") <= x.astype("int32x2"), vc.astype("uint1x2")),
            S.logical_and(y <= x, vc).astype("uint1x2"),
        ),
        Param(
            S.logical_or(y.astype("int32x2") <= x.astype("int32x2"), vc.astype("uint1x2")),
            S.logical_or(y <= x, vc).astype("uint1x2"),
        ),
    )


class TestSelect(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.Var("z", dtype="int32")

    param = (
        # Add rules
        Param(
            S.select(x < 0, y, 0) + S.select(x < 0, 1, z),
            S.select(x < 0, y + 1, z),
        ),
        Param(
            S.select(x < 0, y, 1) - S.select(x < 0, 1, z),
            S.select(x < 0, y + (-1), 1 - z),
        ),
        Param(S.select(x < 0, y, z) - y, S.select(x < 0, 0, z - y)),
        Param(S.select(x < 0, y, z) - z, S.select(x < 0, y - z, 0)),
        Param(
            S.min(S.select(x < 0, y, 0), S.select(x < 0, 1, z)),
            S.select(x < 0, S.min(y, 1), S.min(0, z)),
        ),
        Param(
            S.max(S.select(x < 0, y, 0), S.select(x < 0, 1, z)),
            S.select(x < 0, S.max(y, 1), S.max(0, z)),
        ),
        Param(S.select(x * 3 + 1 != 0, y, z), y),
        Param(S.select(x * 3 + 1 == 0, y, z), z),
        Param(S.select(x > 0, y + 1, y + 1), y + 1),
    )


class TestCancellation(_Test):
    var_int8 = S.Var("var_int8", "int8")
    var_int32 = S.Var("var_int32", "int32")
    var_int64 = S.Var("var_int64", "int64")
    var_uint8 = S.Var("var_uint8", "uint8")
    var_uint32 = S.Var("var_uint32", "uint32")
    var_uint64 = S.Var("var_uint64", "uint64")

    param = (
        Param(_const(5, "int64") - _const(5, "int64"), _const(0, "int64")),
        Param(_const(5, "uint8") - _const(5, "uint8"), _const(0, "uint8")),
        Param(var_int8 - var_int8, _const(0, "int8")),
        Param(var_int32 - var_int32, _const(0, "int32")),
        Param(var_int64 - var_int64, _const(0, "int64")),
        Param(var_uint8 - var_uint8, _const(0, "uint8")),
        Param(var_uint32 - var_uint32, _const(0, "uint32")),
        Param(var_uint64 - var_uint64, _const(0, "uint64")),
        Param(_const(5, "int64") == _const(5, "int64"), _const(True, "bool")),
        Param(_const(5, "uint8") == _const(5, "uint8"), _const(True, "bool")),
        Param(var_int8 == var_int8, _const(True, "bool")),  # noqa: PLR0124
        Param(var_int32 == var_int32, _const(True, "bool")),  # noqa: PLR0124
        Param(var_int64 == var_int64, _const(True, "bool")),  # noqa: PLR0124
        Param(var_uint8 == var_uint8, _const(True, "bool")),  # noqa: PLR0124
        Param(var_uint32 == var_uint32, _const(True, "bool")),  # noqa: PLR0124
        Param(var_uint64 == var_uint64, _const(True, "bool")),  # noqa: PLR0124
        Param(_const(5, "int64") != _const(5, "int64"), _const(False, "bool")),
        Param(_const(5, "uint8") != _const(5, "uint8"), _const(False, "bool")),
        Param(var_int8 != var_int8, _const(False, "bool")),  # noqa: PLR0124
        Param(var_int32 != var_int32, _const(False, "bool")),  # noqa: PLR0124
        Param(var_int64 != var_int64, _const(False, "bool")),  # noqa: PLR0124
        Param(var_uint8 != var_uint8, _const(False, "bool")),  # noqa: PLR0124
        Param(var_uint32 != var_uint32, _const(False, "bool")),  # noqa: PLR0124
        Param(var_uint64 != var_uint64, _const(False, "bool")),  # noqa: PLR0124
    )


class TestAddIndex(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.Var("z", dtype="int32")

    param = (
        Param(x + (y - x), y),
        Param(x - (y + 1) + (y + 1), x),
        Param((x - 10) + (10 - z), x - z),
        Param((x - y) + (z - x), z - y),
        Param(S.min(x, y - z) + z, S.min(x + z, y)),
        Param(S.min(x - z, y) + z, S.min(x, y + z)),
        Param(S.max(x, y - 10) + 10, S.max(x + 10, y)),
        Param(S.max(x - 11, y) + 11, S.max(x, y + 11)),
        Param(S.max(x, y * 2) + S.min(x, y * 2), x + y * 2),
        Param(S.min(x, y * 2) + S.max(x, y * 2), x + y * 2),
        Param(S.max(x, y + 2) + (-2), S.max(x + (-2), y)),
        Param(S.min(x, y + 2) + (-2), S.min(x + (-2), y)),
        Param(S.min(x + 2, y + 3) + (-2), S.min(x, y + 1)),
        Param(S.max(0, 1 - x * 4) + x * 4, S.max(x * 4, 1)),
        Param(S.max(2 - x * 4, 0) + x * 4, S.max(x * 4, 2)),
        Param(S.min(0, 1 - x * 4) + x * 4, S.min(x * 4, 1)),
        Param(S.min(2 - x * 4, 0) + x * 4, S.min(x * 4, 2)),
        Param(x * y + x * 10, x * (y + 10)),
        Param(y * x + x * 10, x * (y + 10)),
        Param(y * x + 10 * x, x * (y + 10)),
        Param(x * y + 10 * x, x * (y + 10)),
        Param((2 * z) + S.min(x, y - (2 * z)), S.min(x + (z * 2), y)),
        Param(y * x + x, x * (y + 1)),
        Param(x * y + x, x * (y + 1)),
        Param((x + 10) + 13, x + 23),
        Param((x + 10) + (13 + z), x + z + 23),
        Param(x * y + 10 * x, x * (y + 10)),
        Param(y * x + x * 3, x * (y + 3)),
        Param(x + 3 + y, x + y + 3),
        Param((3 - y) + x, x - y + 3),
        # canonicalization
        Param(x + 2 + 3 + 4 + x, x * 2 + 9),
        Param(x + 2 + 3 + 4 + x * 3, x * 4 + 9),
        # DivMod rules
        # trunc div
        Param(y * tmod(x, 8) + 10 * tmod(x, 8), tmod(x, 8) * (y + 10)),
        Param(tdiv(x, 8) * 8 + tmod(x, 8), x),
        # floor div
        Param(y * flm(x, 8) + 10 * flm(x, 8), flm(x, 8) * (y + 10)),
        Param(fld(x, 8) * 8 + flm(x, 8), x),
        Param(fld(flm(x, 2) + 7, 2) + fld(x, 2), fld(x + 7, 2)),
    )


class TestSubIndex(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.Var("z", dtype="int32")

    param = (
        Param(x + y - y, x),
        Param(x + y - x, y),
        Param(x - (y + x), 0 - y),
        Param(x - (x + y), 0 - y),
        Param(S.min(x, y) - x, S.min(0, y - x)),
        Param(S.min(x, y) - y, S.min(x - y, 0)),
        Param(S.max(x, y) - x, S.max(0, y - x)),
        Param(S.max(x, y) - y, S.max(x - y, 0)),
        Param(x - S.min(x, y), S.max(0, x - y)),
        Param(y - S.min(x, y), S.max(y - x, 0)),
        Param(x - S.max(x, y), S.min(0, x - y)),
        Param(y - S.max(x, y), S.min(y - x, 0)),
        # mul co-efficient foldng
        Param(x - x, _i32(0)),
        Param(x * y - x, x * (y + (-1))),
        Param(x * y - 10 * x, x * (y + (-10))),
        Param(y * x - x * z, x * (y - z)),
        Param(y * x - z * x, x * (y - z)),
        Param(x + 10 - 20, x + (-10)),
        # 4-operands pattern
        Param((x + y) - (x + z), y - z),
        Param((y + x) - (x + z), y - z),
        Param((x + y) - (z + x), y - z),
        Param((y + x) - (z + x), y - z),
        Param(S.min(x + y, z) - x, S.min(y, z - x)),
        Param(S.min(y + x, z) - x, S.min(y, z - x)),
        Param(S.min(z, x + y) - x, S.min(z - x, y)),
        Param(S.min(z, y + x) - x, S.min(z - x, y)),
        Param(S.max(x + y, z) - x, S.max(y, z - x)),
        Param(S.max(y + x, z) - x, S.max(y, z - x)),
        Param(S.max(z, x + y) - x, S.max(z - x, y)),
        Param(S.max(z, y + x) - x, S.max(z - x, y)),
        Param(x - S.min(x + y, z), S.max(0 - y, x - z)),
        Param(x - S.min(y + x, z), S.max(0 - y, x - z)),
        Param(x - S.min(z, x + y), S.max(x - z, 0 - y)),
        Param(x - S.min(z, y + x), S.max(x - z, 0 - y)),
        Param(S.min(x, y) - S.min(y, x), _i32(0)),
        Param(S.max(x, y) - S.max(y, x), _i32(0)),
        Param(S.min(x, y) - S.min(x + 10, y + 10), _i32(-10)),
        Param(S.min(x + 10, y + 1) - S.min(x, y - 9), _i32(10)),
        Param(x - S.max(x + y, 0), S.min(0 - y, x)),
        Param(x - S.max(0, x + y), S.min(x, 0 - y)),
        Param(x - S.min(x + y, 0), S.max(0 - y, x)),
        Param(x - S.min(0, x + y), S.max(x, 0 - y)),
        # DivMod patterns
        # truc div
        Param(x - tdiv(x, 3) * 3, tmod(x, 3)),
        Param(tdiv(x + 5, 3) - tdiv(x, 3), tdiv(tmod(x, 3) + 5, 3), x >= 0),
        Param(tdiv(x + 5, 3) - tdiv(x + 1, 3), tdiv(tmod(x + 1, 3) + 4, 3), x >= -1),
        Param(y - tdiv(y, (-5)) * (-5), tmod(y, 5)),
        Param(tdiv(y, 3) * 3 - y, 0 - tmod(y, 3)),
        Param(y - tdiv(y - 6, 5) * 5, tmod(y + (-6), 5) + 6),
        Param(tdiv(y - 6, 5) * 5 - y, (-6) - tmod(y + (-6), 5)),
        Param(y - tdiv(y + z, 5) * 5, tmod(y + z, 5) - z),
        Param(tdiv(y + z, 5) * 5 - y, z - tmod(y + z, 5)),
        Param(y - tdiv(y - z, 5) * 5, tmod(y - z, 5) + z),
        Param(tdiv(y - z, 5) * 5 - y, 0 - tmod(y - z, 5) - z),
        Param(y * 3 - tdiv(y, 2) * 6, tmod(y, 2) * 3),
        Param(tdiv(y, 3) * 6 - y * 2, tmod(y, 3) * (-2)),
        Param(y * 5 - tdiv(y + z, 2) * 10, (tmod(y + z, 2) - z) * 5),
        Param(y * 5 - tdiv(y - z, 2) * 10, (tmod(y - z, 2) + z) * 5),
        Param(tdiv(y + z, 3) * 6 - y * 2, (z - tmod(y + z, 3)) * 2),
        Param(tdiv(y - z, 3) * 6 - y * 2, (0 - tmod(y - z, 3) - z) * 2),
        Param(5 * y - tdiv(y + z, 2) * 10, (tmod(y + z, 2) - z) * 5),
        Param(5 * y - 10 * tdiv(y - z, 2), (tmod(y - z, 2) + z) * 5),
        Param(6 * tdiv(y + z, 3) - y * 2, (z - tmod(y + z, 3)) * 2),
        Param(tdiv(y - z, 3) * 6 - 2 * y, (0 - tmod(y - z, 3) - z) * 2),
        # floor div
        Param(x - fld(x, 3) * 3, flm(x, 3)),
        Param(fld(x + 5, 3) - fld(x, 3), fld(flm(x, 3) + 5, 3)),
        Param(fld(x + 5, 3) - fld(x + 2, 3), fld(flm(x + 2, 3), 3) + 1),
        Param(fld(y, 3) * 3 - y, 0 - flm(y, 3)),
        Param(y - fld(y - 6, 5) * 5, flm(y + 4, 5) + 6),
        Param(fld(y - 6, 5) * 5 - y, (-6) - flm(y + 4, 5)),
        Param(y - fld(y + z, 5) * 5, flm(y + z, 5) - z),
        Param(fld(y + z, 5) * 5 - y, z - flm(y + z, 5)),
        Param(y - fld(y - z, 5) * 5, flm(y - z, 5) + z),
        Param(fld(y - z, 5) * 5 - y, 0 - flm(y - z, 5) - z),
        Param(y * 3 - fld(y, 2) * 6, flm(y, 2) * 3),
        Param(fld(y, 3) * 6 - y * 2, flm(y, 3) * (-2)),
        Param(y * 5 - fld(y + z, 2) * 10, (flm(y + z, 2) - z) * 5),
        Param(y * 5 - fld(y - z, 2) * 10, (flm(y - z, 2) + z) * 5),
        Param(fld(y + z, 3) * 6 - y * 2, (z - flm(y + z, 3)) * 2),
        Param(fld(y - z, 3) * 6 - y * 2, (0 - flm(y - z, 3) - z) * 2),
        Param(5 * y - fld(y + z, 2) * 10, (flm(y + z, 2) - z) * 5),
        Param(5 * y - 10 * fld(y - z, 2), (flm(y - z, 2) + z) * 5),
        Param(6 * fld(y + z, 3) - y * 2, (z - flm(y + z, 3)) * 2),
        Param(fld(y - z, 3) * 6 - 2 * y, (0 - flm(y - z, 3) - z) * 2),
    )


class TestMulIndex(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")

    param = (
        Param((x + 2) * 3, x * 3 + 6),
        Param((x * 2) * 3, x * 6),
        Param(S.min(x, y) * S.max(x, y), x * y),
        Param(S.max(x, y) * S.min(x, y), x * y),
        Param((x - y) * (-2), (y - x) * 2),
    )


class TestDivIndex(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.Var("z", dtype="int32")
    non_negative: typing.ClassVar[list[S.Expr]] = [x >= 0, y >= 0, z >= 0]

    param = (
        Param(tdiv(x, x), _i32(1)),
        Param(tdiv(tdiv(x, 2), 3), tdiv(x, 6)),
        Param(tdiv(tdiv(x, 2) + 1, 3), tdiv(x + 2, 6), non_negative),
        Param(tdiv(x * 2, 4), tdiv(x, 2)),
        Param(tdiv(x * 4, 2), x * 2),
        Param(tdiv(x * 4 + y, 2), x * 2 + tdiv(y, 2), non_negative),
        Param(tdiv(S.min(x * 6, y), 2), S.min(x * 3, tdiv(y, 2)), non_negative),
        Param(tdiv(S.max(x * 6, y), 2), S.max(x * 3, tdiv(y, 2)), non_negative),
        Param(tdiv(y + x * 4, 2), tdiv(y, 2) + x * 2, non_negative),
        Param(tdiv(S.min(y, x * 6), 2), S.min(tdiv(y, 2), x * 3), non_negative),
        Param(tdiv(S.max(y, x * 6), 2), S.max(tdiv(y, 2), x * 3), non_negative),
        # 3-operands
        Param(tdiv(x * 6 + y + z, 2), x * 3 + tdiv(y + z, 2), non_negative),
        Param(tdiv(x * 6 - y + (y + 3), 2), x * 3 + 1, non_negative),
        Param(tdiv(x * 6 + (y + 3) - y, 2), x * 3 + 1, non_negative),
        Param(tdiv(y + x * 6 + z, 2), x * 3 + tdiv(y + z, 2), non_negative),
        Param(tdiv(x + 4, 2), tdiv(x, 2) + 2, non_negative),
        Param(tdiv(x + y, x), tdiv(y, x) + 1, non_negative),
        Param(tdiv(y + x, x), tdiv(y, x) + 1, non_negative),
        Param(tdiv((x + y) + z, x), tdiv(y + z, x) + 1, non_negative),
        Param(tdiv((y + x) + z, x), tdiv(y + z, x) + 1, non_negative),
        Param(tdiv(y + (x + z), x), tdiv(y + z, x) + 1, non_negative),
        Param(tdiv(y + (z + x), x), tdiv(y + z, x) + 1, non_negative),
        Param(tdiv(x * y, y), x, non_negative),
        Param(tdiv(y * x, y), x, non_negative),
        Param(tdiv(x * z + y, z), x + tdiv(y, z), non_negative),
        Param(tdiv(z * x + y, z), x + tdiv(y, z), non_negative),
        Param(tdiv(y + x * z, z), tdiv(y, z) + x, non_negative),
        Param(tdiv(y + z * x, z), tdiv(y, z) + x, non_negative),
    )


class TestFloordivIndex(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.Var("z", dtype="int32")

    param = (
        Param(fld(fld(x, 2), 3), fld(x, 6)),
        Param(fld(fld(x, 2) + 1, 3), fld(x + 2, 6)),
        Param(fld(x - flm(x, 21), 21), fld(x, 21)),
        Param(fld(x * 2, 4), fld(x, 2)),
        Param(fld(x * 4, 2), x * 2),
        Param(fld(x * 8 + 7, 16), fld(x, 2)),
        Param(fld(x * 8 + 39, 16), fld(x, 2) + 2),
        Param(fld(x * 8 - 1, 16), fld(x * 8 + -1, 16)),
        Param(fld(x * 8 - 9, 16), fld(x, 2) + -1),
        # TODO(Lunderberg): Remove the necessity for the preconditions
        # in this section.  They shouldn't be necessary for floordiv,
        # where they would be required for truncdiv.
        Param(fld(x * 360 + y, 16), x * 22, [x >= 0, x < 2, y >= 0, y < 7]),
        Param(fld(x * 360 + y, 25), x * 14, [x >= 0, x < 2, y >= 0, y < 7]),
        Param(fld(x * 360 - 8, 25), fld(x * 360 + -8, 25)),
        Param(fld(x * 4 + y, 2), x * 2 + fld(y, 2)),
        Param(fld(S.min(x * 6, y), 2), S.min(x * 3, fld(y, 2))),
        Param(fld(S.max(x * 6, y), 2), S.max(x * 3, fld(y, 2))),
        Param(fld(y + x * 4, 2), x * 2 + fld(y, 2)),
        Param(fld(S.min(y, x * 6), 2), S.min(fld(y, 2), x * 3)),
        Param(fld(S.max(y, x * 6), 2), S.max(fld(y, 2), x * 3)),
        # 3-operands
        #
        # TODO(Lunderberg): Remove the necessity for the preconditions
        # in this section.  They shouldn't be required, since floordiv
        # has translational symmetry, even for negative.
        Param(fld(x * 6 + y + z, 2), x * 3 + fld(y + z, 2)),
        Param(fld(x * 6 - y + (y + 3), 2), x * 3 + 1),
        Param(fld(x * 6 + (y + 3) - y, 2), x * 3 + 1),
        Param(fld(y + x * 6 + z, 2), x * 3 + fld(y + z, 2)),
        Param(fld(x + 4, 2), fld(x, 2) + 2),
        Param(fld(x + y, x), fld(y, x) + 1, x >= 0),
        Param(fld(y + x, x), fld(y, x) + 1, x >= 0),
        Param(fld((x + y) + z, x), fld(y + z, x) + 1, x >= 0),
        Param(fld((y + x) + z, x), fld(y + z, x) + 1, x >= 0),
        Param(fld(y + (x + z), x), fld(y + z, x) + 1, x >= 0),
        Param(fld(y + (z + x), x), fld(y + z, x) + 1, x >= 0),
        Param(fld(x * y, y), x, y >= 0),
        Param(fld(y * x, y), x, y >= 0),
        Param(fld(x * z + y, z), x + fld(y, z), z >= 0),
        Param(fld(x * z * 2 + y, z * 2), x + fld(y, z * 2), z * 2 >= 0),
        Param(fld(z * x + y, z), x + fld(y, z), z >= 0),
        Param(fld(y + x * z, z), fld(y, z) + x, z >= 0),
        Param(fld(y + z * x, z), fld(y, z) + x, z >= 0),
        Param(fld(x * 32 + y, 64), fld(x, 2), [y >= 0, y < 32]),
        Param(fld(x * 128 + y * 4 + z, 512), fld(x, 4), [y >= 0, y < 32, z >= 0, z < 4]),
    )


class TestModIndex(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.Var("z", dtype="int32")
    nx = S.Var("nx", dtype="int32")
    ny = S.Var("ny", dtype="int32")

    param = (
        # TODO(Lunderberg): Loosen these preconditions.  When there's
        # a single term whose factor is divisible by the denominator,
        # the sign of the argument doesn't matter.
        Param(tmod(x * 10, 2), _i32(0), x >= 0),
        Param(tmod(x * 10 + y, 2), tmod(y, 2), [x >= 0, y >= 0]),
        Param(tmod(x + 10, 2), tmod(x, 2), x >= 0),
        Param(tmod(x + y * 10, 2), tmod(x, 2), [x >= 0, y >= 0]),
        Param(tmod(x * 10 + 1 + y * 2 + 2, 2), _i32(1), [x >= 0, y >= 0]),
        Param(tmod(x * 10, -2), _i32(0), x <= 0),
        Param(tmod(x * 10 + y, -2), tmod(y, 2), [x >= 0, y >= 0]),
        Param(tmod(x + 10, -2), tmod(x, 2), x >= 0),
        Param(tmod(x + y * 10, -2), tmod(x, 2), [x >= 0, y >= 0]),
        Param(tmod(x * 10 + 1 + y * 2 + 2, -2), _i32(1), [x >= 0, y >= 0]),
        Param(tmod(x * (-10), 2), _i32(0)),
        Param(tmod(x * (-10) + y, 2), tmod(x * (-10) + y, 2)),
        Param(tmod(x + (-10), 2), tmod(x + (-10), 2)),
        Param(tmod(x + y * (-10), 2), tmod(x + y * (-10), 2)),
        Param(tmod(x * (-10), -2), _i32(0)),
        Param(tmod(nx * 10, 2), _i32(0)),
        Param(tmod(nx * (-10) + y, 2), tmod(y, 2), [nx <= 0, y >= 0]),
        Param(tmod(x + ny * (-10), 2), tmod(x, 2), [x >= 0, ny <= 0]),
        Param(tmod(nx * (-10) + 1 + ny * (-2) + 2, 2), _i32(1), [nx <= 0, ny <= 0]),
        Param(tmod(nx * 10, -2), _i32(0)),
        Param(tmod(nx * (-10) + y, -2), tmod(y, 2), [nx <= 0, y >= 0]),
        Param(tmod(x + ny * (-10), -2), tmod(x, 2), [x >= 0, ny <= 0]),
    )


class TestFloormodIndex(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.Var("z", dtype="int32")

    param = (
        Param(flm(x * 10, 2), _i32(0)),
        Param(flm(x * 9600, 6400), flm(x * 3200, 6400)),
        Param(flm(x * 10 + y, 2), flm(y, 2)),
        Param(flm(x * 360 + y, 16), flm(x * 8 + y, 16)),
        Param(flm(x + 10, 2), flm(x, 2)),
        Param(flm(x + y * 10, 2), flm(x, 2)),
        Param(flm(x + y * 360, 16), flm(x + y * 8, 16)),
        Param(flm(x * (-10), 2), _i32(0)),
        Param(flm(x * (-10) + y, 2), flm(y, 2)),
        Param(flm(x + (-10), 2), flm(x, 2)),
        Param(flm(x + y * (-10), 2), flm(x, 2)),
        Param(flm(x * 32 + y, 64), flm(x, 2) * 32 + y, [y >= 0, y < 32]),
        Param(flm(x * 32 - y, 64), flm(x * 32 - y, 64), [y >= 0, y < 32]),
        Param(flm(x * z * 2 + y, z * 2), flm(y, z * 2), z * 2 >= 0),
        # NOTE: the followng case is covered by canonical simplify
        # long range simplifcation in general can be covered by canonical simplify
        # Param(flm(x * 10 + 1 + y * 2 + 2, 2), 1),
    )


class TestFloorModTwo(_Test):
    """Special-case simplifications for FloorMod(expr,2)

    Because FloorMod(expr,2) has only two possible values, it can be
    simplified more aggressively than most FloorMod expressions.  Some
    of these have analogues for other denominators (e.g. x%3 + (x+1)%3
    + (x+2)%3 == 0 + 1 + 2), but they don't appear as often and
    require identifying more related terms in order to apply.

    (x + c1)//2 - (x+c2)//2 => (x%2)*( c1%2 - c1%2 ) + (c1//2 - c2//2)

    We should not introduce extra negative coeficient to iterators
    however during simplification
    """

    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.Var("z", dtype="int32")

    param = (
        # Removing offsets from floormod
        Param(flm(x, 2) + flm(x + 1, 2), 1),
        Param(flm(x + 1, 2) + flm(x, 2), 1),
        # Difference of floordiv yields floormod
        Param(fld(x + 1, 2) - fld(x, 2), flm(x, 2)),
        Param(fld(x, 2) - fld(x - 1, 2), flm(x, 2) * -1 + 1),
        Param(fld(x + 5, 2) - fld(x - 2, 2), flm(x, 2) + 3),
        Param(fld(x + 5, 2) - fld(x - 3, 2), 4),
        Param(fld(flm(x, 2) + 1, 2), flm(x, 2)),
        # Sum of floordiv and floormod to yield floordiv
        Param(fld(x + 1, 2) - flm(x, 2), fld(x, 2)),
        Param(fld(x, 2) + flm(x, 2), fld(x + 1, 2)),
        # regression: although we can rewrite (x + 1) %2 => 1 - x%2
        # doing so would introduce negative co-efficient to iterators
        # which makes later iter map detection harder, in principle we
        # should not introduce additional negative signs of iterator in rewriting
        Param(flm(x + 1, 2), flm(x + 1, 2)),
        Param(flm(x + 5, 2), flm(x + 1, 2)),
        Param(flm(x + 1, 2) * 8192, flm(x + 1, 2) * 8192, [x >= 0, x < 2]),
    )


class TestFloorModPadded(_Test):
    """Special-case simplifications for divisibility proof
    such that (x - x % k) must be divisible by k
    """

    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    param = (
        Param(flm(x - flm(x, 9), 9), 0),
        Param(flm(x - flm(x, -9), 9), 0),
        Param(flm(x + flm(-x, 9), 9), 0),
        Param(flm(x + flm(8 * x, 9), 9), 0),
        Param(flm(x - flm(x, y), y), 0),
        Param(flm(x - flm(x, -y), y), 0),
        Param(flm(x + flm(-x, y), y), 0),
    )


class TestMinIndex(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.Var("z", dtype="int32")
    param = (
        # const int bound
        Param(S.min(tmod(x, 2), tmod(y, 2) + 10), tmod(x, 2)),
        Param(S.min(flm(x, 2), flm(y, 2) + 10), flm(x, 2)),
        Param(S.min(x + 1, x + 10), x + 1),
        Param(S.min(x + 111, x + 10), x + 10),
        Param(S.min(x + 1, x), x),
        Param(S.min(x, x + 2), x),
        Param(S.min(1 - x, 2 - x), 1 - x),
        Param(S.min(3 - x, 2 - x), 2 - x),
        Param(S.min(S.max(x, y), S.min(x, y)), S.min(x, y)),
        Param(S.min(S.max(x, y), S.min(y, x)), S.min(x, y)),
        Param(S.min(S.max(x, y), x), x),
        Param(S.min(S.max(y, x), x), x),
        Param(S.min(S.min(x, y), x), S.min(x, y)),
        Param(S.min(S.min(x, y), y), S.min(x, y)),
        Param(S.min(x, S.max(x, y)), x),
        Param(S.min(x, S.max(y, x)), x),
        Param(S.min(x, S.min(x, y)), S.min(x, y)),
        Param(S.min(y, S.min(x, y)), S.min(x, y)),
        Param(S.min(S.min(S.min(x, y), z), y), S.min(S.min(x, y), z)),
        Param(
            S.min(S.min(S.min(S.min(x, y), z), x * 2), y),
            S.min(S.min(S.min(x, y), z), x * 2),
        ),
        Param(
            S.min(S.min(S.min(S.min(S.min(x, y), z), x * 2), z * 2), y),
            S.min(S.min(S.min(S.min(x, y), z), x * 2), z * 2),
        ),
        Param(S.min(S.max(x, y), S.max(x, z)), S.max(S.min(y, z), x)),
        Param(S.min(S.max(x, y), S.max(z, x)), S.max(S.min(y, z), x)),
        Param(S.min(S.max(y, x), S.max(x, z)), S.max(S.min(y, z), x)),
        Param(S.min(S.max(y, x), S.max(z, x)), S.max(S.min(y, z), x)),
        Param(S.min(y + x, z + x), S.min(y, z) + x),
        Param(S.min(y + x, x + z), S.min(y, z) + x),
        Param(S.min(x + y, z + x), S.min(y, z) + x),
        Param(S.min(x + y, x + z), S.min(y, z) + x),
        Param(S.min(x - y, x - z), x - S.max(y, z)),
        Param(S.min(y - x, z - x), S.min(y, z) - x),
        Param(S.min(S.min(x, 1), 10), S.min(x, 1)),
        Param(S.min(S.min(x, 11), 10), S.min(x, 10)),
        Param(S.min(x * 3, 9), S.min(x, 3) * 3),
        Param(S.min(x * 2, 0), S.min(x, 0) * 2),
        Param(S.min(0 - x * 2, 0), S.max(x, 0) * -2),
        Param(S.min(3 - x, 2), 3 - S.max(x, 1)),
        Param(S.min(x * (-2), -4), S.max(x, 2) * -2),
        Param(S.min(x * (-2), 4), S.max(x, -2) * -2),
        Param(S.min(x * (0), 4), 0),
        Param(S.min(x * (0), -4), -4),
        # DivMod rules
        # truc div
        Param(S.min(tdiv(x + 3, 4) * 4, x), x),
        Param(S.min(x, tdiv(x + 3, 4) * 4), x),
        Param(S.min(tdiv(x + 3, 4) * 4, S.max(x, 4)), S.max(x, 4), x > 0),
        Param(S.min(S.max(x, 4), tdiv(x + 3, 4) * 4), S.max(x, 4), x > 0),
        Param(S.min(tdiv(x, 10), tdiv(y, 10)), tdiv(S.min(x, y), 10)),
        Param(S.min(tdiv(x, (-10)), tdiv(y, (-10))), tdiv(S.max(x, y), (-10))),
        # floor div
        Param(S.min(fld(x + 3, 4) * 4, x), x),
        Param(S.min(x, fld(x + 3, 4) * 4), x),
        Param(S.min(x, fld(x, 4) * 4), fld(x, 4) * 4),
        Param(S.min(fld(x + 3, 4) * 4, S.max(x, 4)), S.max(x, 4), x > 0),
        Param(S.min(S.max(x, 4), fld(x + 3, 4) * 4), S.max(x, 4), x > 0),
        Param(S.min(fld(x, 10), fld(y, 10)), fld(S.min(x, y), 10)),
        Param(S.min(fld(x, (-10)), fld(y, (-10))), fld(S.max(x, y), (-10))),
    )


class TestMaxIndex(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.Var("z", dtype="int32")

    param = (
        # const int bound
        Param(S.max(tmod(x, 2), tmod(y, 2) + 10), tmod(y, 2) + 10),
        Param(S.max(flm(x, 2), flm(y, 2) + 10), flm(y, 2) + 10),
        Param(S.max(x + 1, x + 10), x + 10),
        Param(S.max(x + 111, x + 10), x + 111),
        Param(S.max(x + 1, x), x + 1),
        Param(S.max(x, x + 2), x + 2),
        Param(S.max(1 - x, 2 - x), 2 - x),
        Param(S.max(3 - x, 2 - x), 3 - x),
        Param(S.max(S.min(x, y), S.max(x, y)), S.max(x, y)),
        Param(S.max(S.min(x, y), S.max(y, x)), S.max(x, y)),
        Param(S.max(S.min(x, y), x), x),
        Param(S.max(S.min(y, x), x), x),
        Param(S.max(S.max(x, y), x), S.max(x, y)),
        Param(S.max(S.max(x, y), y), S.max(x, y)),
        Param(S.max(x, S.min(x, y)), x),
        Param(S.max(x, S.min(y, x)), x),
        Param(S.max(x, S.max(x, y)), S.max(x, y)),
        Param(S.max(y, S.max(x, y)), S.max(x, y)),
        Param(S.max(S.max(S.max(x, y), z), y), S.max(S.max(x, y), z)),
        Param(
            S.max(S.max(S.max(S.max(x, y), z), x * 2), y),
            S.max(S.max(S.max(x, y), z), x * 2),
        ),
        Param(
            S.max(S.max(S.max(S.max(S.max(x, y), z), x * 2), z * 2), y),
            S.max(S.max(S.max(S.max(x, y), z), x * 2), z * 2),
        ),
        Param(S.max(S.min(x, y), S.min(x, z)), S.min(S.max(y, z), x)),
        Param(S.max(S.min(x, y), S.min(z, x)), S.min(S.max(y, z), x)),
        Param(S.max(S.min(y, x), S.min(x, z)), S.min(S.max(y, z), x)),
        Param(S.max(S.min(y, x), S.min(z, x)), S.min(S.max(y, z), x)),
        Param(S.max(y + x, z + x), S.max(y, z) + x),
        Param(S.max(y + x, x + z), S.max(y, z) + x),
        Param(S.max(x + y, z + x), S.max(y, z) + x),
        Param(S.max(x + y, x + z), S.max(y, z) + x),
        Param(S.max(x - y, x - z), x - S.min(y, z)),
        Param(S.max(y - x, z - x), S.max(y, z) - x),
        Param(S.max(S.max(x, 1), 10), S.max(x, 10)),
        Param(S.max(S.max(x, 11), 10), S.max(x, 11)),
        Param(S.max(x * 3, 9), S.max(x, 3) * 3),
        Param(S.max(3 - x, 1), 3 - S.min(x, 2)),
        Param(S.max(x * 2, 0), S.max(x, 0) * 2),
        Param(S.max(0 - x * 2, 0), S.min(x, 0) * -2),
        Param(S.max(x * (-2), -4), S.min(x, 2) * -2),
        Param(S.max(x * (-2), 4), S.min(x, -2) * -2),
        Param(S.max(x * (0), 4), 4),
        Param(S.max(x * (0), -4), 0),
        # DivMod rules
        # truc div
        Param(S.max(tdiv(x, 10), tdiv(y, 10)), tdiv(S.max(x, y), 10)),
        Param(S.max(tdiv(x, (-10)), tdiv(y, (-10))), tdiv(S.min(x, y), (-10))),
        Param(S.max(tdiv(x + 3, 4) * 4, x), tdiv(x + 3, 4) * 4),
        # floordiv
        Param(S.max(fld(x, 10), fld(y, 10)), fld(S.max(x, y), 10)),
        Param(S.max(fld(x, (-10)), fld(y, (-10))), fld(S.min(x, y), (-10))),
        Param(S.max(fld(x + 3, 4) * 4, x), fld(x + 3, 4) * 4),
        Param(S.max(fld(x, 4) * 4, x), x),
        Param(S.max(x, fld(x, 4) * 4), x),
    )


class TestComparisons(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.Var("z", dtype="int32")

    param = (
        # const int bound
        Param((tmod(x, 2) + 10).equal(0), _const(0, "bool")),
        Param((tmod(x, 2) + 10) != 0, _const(1, "bool")),
        Param(tmod(x, 2) + 10 > 1, _const(1, "bool")),
        Param(tmod(x, 2) + 10 <= 1, _const(0, "bool")),
        Param(flm(x, 2) + 2 > 1, _const(1, "bool")),
        Param(flm(x, 2) + 10 <= 1, _const(0, "bool")),
        Param(x * 3 + 10 == 0, _const(0, "bool")),
        Param(x * 3 + 10 != 0, _const(1, "bool")),
        # canonicalization
        Param((x - 10).equal(0), x.equal(10)),
        Param((10 - x).equal(0), x.equal(10)),
        Param((x * y).equal(0), S.logical_or(x.equal(0), y.equal(0))),
        # Write _lt as _le for integer arguments, if possible
        Param(x - 1 < y, x <= y),
        Param(x + (-1) < y, x <= y),
        Param(x < y - (-1), x <= y),
        Param(x < y + 1, x <= y),
        Param(x + 2 < y + 3, x <= y),
        Param(x - 3 < y - 2, x <= y),
        Param(x - 3 < y + (-2), x <= y),
        Param(x + (-3) < y - 2, x <= y),
        # Merge constants on the LHS/RHS of a _lt expression.
        Param(x + 10 < y + 10, x < y),
        Param(x + 5 < y + 10, x < y + 5),
        Param(x + 10 < y + 5, x + 5 < y),
        Param(x - 5 < y - 10, x + 5 < y),
        Param(x - 10 < y - 5, x < y + 5),
        Param(x < y - 10, x + 10 < y),
        Param(x - 10 < y, x < y + 10),
        # cmp bound
        Param(x + y < x + z, y < z),
        Param(x + y < z + x, y < z),
        Param(y + x < x + z, y < z),
        Param(y + x < z + x, y < z),
        Param(y - x < z - x, y < z),
        Param(x - y < x - z, z < y),
        Param(x < z + x, _lt(0, z)),
        Param(x < x + z, _lt(0, z)),
        Param(100 < x + 1, _lt(99, x)),
        Param(1 < 100 - x, _lt(x, 99)),
        Param(x * 3 < y * 3, x < y),
        Param(x * (-3) < y * (-3), y < x),
        Param(x * 3 >= y * 3, y <= x),
        Param(x * 4 >= 2, _le(1, x)),
        Param(x * 2 >= 50, _le(25, x)),
        Param(x * 4 <= 2, x <= 0),
        Param((0 - x * 3) <= 0, _le(0, x)),
        Param((0 - x * 3) >= 0, _le(x, 0)),
        Param(2 * x <= 0, x <= 0),
        Param(x * 2 >= 3, _le(2, x)),
        Param(x * 2 >= 2, _le(1, x)),
        Param(x * 2 >= 1, _le(1, x)),
        Param(x * 2 >= 0, _le(0, x)),
        Param(x * 2 >= -1, _le(0, x)),
        Param(x * 2 >= -2, _le(-1, x)),
        Param(x * 2 >= -3, _le(-1, x)),
        Param(x * 2 <= 3, _le(x, 1)),
        Param(x * 2 <= 2, _le(x, 1)),
        Param(x * 2 <= 1, _le(x, 0)),
        Param(x * 2 <= 0, _le(x, 0)),
        Param(x * 2 <= -1, _le(x, -1)),
        Param(x * 2 <= -2, _le(x, -1)),
        Param(x * 2 <= -3, _le(x, -2)),
        Param(x * (-2) >= 3, _le(x, -2)),
        Param(x * (-2) >= 2, _le(x, -1)),
        Param(x * (-2) >= 1, _le(x, -1)),
        Param(x * (-2) >= 0, _le(x, 0)),
        Param(x * (-2) >= -1, _le(x, 0)),
        Param(x * (-2) >= -2, _le(x, 1)),
        Param(x * (-2) >= -3, _le(x, 1)),
        Param(x * (-2) <= 3, _le(-1, x)),
        Param(x * (-2) <= 2, _le(-1, x)),
        Param(x * (-2) <= 1, _le(0, x)),
        Param(x * (-2) <= 0, _le(0, x)),
        Param(x * (-2) <= -1, _le(1, x)),
        Param(x * (-2) <= -2, _le(1, x)),
        Param(x * (-2) <= -3, _le(2, x)),
        # DivMod rules
        # truc div
        Param(tdiv(x, 2) < 3, x < 6),
        Param(3 < tdiv(x, 2), _lt(7, x)),
        Param(tdiv(x, 3) >= 0, _le(-2, x)),
        Param(tdiv(x, 2) >= 1, _le(2, x)),
        Param(tdiv(x, 2) >= 0, _le(-1, x)),
        Param(tdiv(x, 2) >= -1, _le(-3, x)),
        Param(tdiv(x, 2) <= 1, _le(x, 3)),
        Param(tdiv(x, 2) <= 0, _le(x, 1)),
        Param(tdiv(x, 2) <= -1, _le(x, -2)),
        Param(tdiv(x, 4) * 4 < x, _lt(0, tmod(x, 4))),
        Param(tdiv(x, 4) * 4 >= x, _le(tmod(x, 4), 0)),
        Param(tdiv(x, 4) * 4 < x + y, _lt(0, tmod(x, 4) + y)),
        Param(tdiv(x, 4) * 4 < x - y, _lt(y, tmod(x, 4))),
        Param(tdiv(x + 2, 4) * 4 >= x, _le(tmod(x + 2, 4), 2)),
        Param(tdiv(x + 2, 4) * 4 >= x + y, _le(tmod(x + 2, 4) + y, 2)),
        Param(tdiv(x + 2, 4) * 4 >= x - y, _le(tmod(x + 2, 4), y + 2)),
        # floor div
        Param(fld(x, 2) < 3, x < 6),
        Param(3 < fld(x, 2), _lt(7, x)),
        Param(-3 < fld(x, 2), _lt(-5, x)),
        Param(fld(x, 3) >= 0, _le(0, x)),
        Param(fld(x, 2) >= 1, _le(2, x)),
        Param(fld(x, 2) >= 0, _le(0, x)),
        Param(fld(x, 2) >= -1, _le(-2, x)),
        Param(fld(x, 2) <= 1, _le(x, 3)),
        Param(fld(x, 2) <= 0, _le(x, 1)),
        Param(fld(x, 2) <= -1, _le(x, -1)),
        Param(fld(x, 4) * 4 < x, _lt(0, flm(x, 4))),
        Param(fld(x, 4) * 4 >= x, _eq(flm(x, 4), 0)),
        Param(fld(x, 4) * 4 < x + y, _lt(0, flm(x, 4) + y)),
        Param(fld(x, 4) * 4 < x - y, _lt(y, flm(x, 4))),
        Param(fld(x + 2, 4) * 4 >= x, _le(flm(x + 2, 4), 2)),
        Param(fld(x + 2, 4) * 4 >= x + y, _le(flm(x + 2, 4) + y, 2)),
        Param(fld(x + 2, 4) * 4 >= x - y, _le(flm(x + 2, 4), y + 2)),
        # End DivMod Rules
        # merging flm/fld into known value
        Param(S.logical_and(fld(x, 8) == 3, flm(x, 8) == 4), x == 28),
        Param(S.logical_and(flm(x, 8) == 4, fld(x, 8) == 3), x == 28),
        Param(S.logical_and(fld(x, 8) == -3, flm(x, 8) == 4), x == -20),
        Param(S.logical_and(flm(x, 8) == 4, fld(x, 8) == -3), x == -20),
        # Rewrite based on definition of integer division
        Param(S.logical_and(_i32(0) <= x - y * 5, x - y * 5 < 5), y == fld(x, 5)),
        Param(S.logical_and(x - y * 5 < 5, _i32(0) <= x - y * 5), y == fld(x, 5)),
        # Narrow upper bound using floormod
        Param(S.logical_and(x < 20, flm(x, 5) < 2), S.logical_and(x < 17, flm(x, 5) < 2)),
        Param(S.logical_and(x < 18, flm(x, 5) < 2), S.logical_and(x < 17, flm(x, 5) < 2)),
        Param(S.logical_and(x <= 19, flm(x, 5) < 2), S.logical_and(x < 17, flm(x, 5) < 2)),
        Param(S.logical_and(x <= 18, flm(x, 5) < 2), S.logical_and(x < 17, flm(x, 5) < 2)),
        Param(S.logical_and(x < -20, flm(x, 5) < 2), S.logical_and(x < -23, flm(x, 5) < 2)),
        Param(S.logical_and(x < 18 - 40, flm(x, 5) < 2), S.logical_and(x < 17 - 40, flm(x, 5) < 2)),
        Param(S.logical_and(x <= -21, flm(x, 5) < 2), S.logical_and(x < -23, flm(x, 5) < 2)),
        Param(S.logical_and(x <= -22, flm(x, 5) < 2), S.logical_and(x < -23, flm(x, 5) < 2)),
        # No change if the floormod cannot help narrow the upper bound
        Param(S.logical_and(x < 16, flm(x, 5) < 2), S.logical_and(x < 16, flm(x, 5) < 2)),
        Param(S.logical_and(x <= 15, flm(x, 5) < 2), S.logical_and(x <= 15, flm(x, 5) < 2)),
        # Merge a known floordiv and an upper bound of floormod into a value range
        Param(
            S.logical_and(fld(x, 10) == 5, flm(x, 10) < 7),
            S.logical_and(_i32(50) <= x, x < 57),
        ),
        Param(
            S.logical_and(fld(x, 10) == 5, flm(x, 10) <= 7),
            S.logical_and(_i32(50) <= x, x <= 57),
        ),
        Param(
            S.logical_and(fld(x, 10) == -5, flm(x, 10) < 7),
            S.logical_and(_i32(-50) <= x, x < -43),
        ),
        Param(
            S.logical_and(fld(x, 10) == -5, flm(x, 10) <= 7),
            S.logical_and(_i32(-50) <= x, x <= -43),
        ),
        # Merge a known floordiv and an lower bound of floormod into a value range
        Param(
            S.logical_and(fld(x, 10) == 5, _i32(7) < flm(x, 10)),
            S.logical_and(_i32(57) < x, x < 60),
        ),
        Param(
            S.logical_and(fld(x, 10) == 5, _i32(7) <= flm(x, 10)),
            S.logical_and(_i32(57) <= x, x < 60),
        ),
        Param(
            S.logical_and(fld(x, 10) == -5, _i32(7) < flm(x, 10)),
            S.logical_and(_i32(-43) < x, x < -40),
        ),
        Param(
            S.logical_and(fld(x, 10) == -5, _i32(7) <= flm(x, 10)),
            S.logical_and(_i32(-43) <= x, x < -40),
        ),
        Param(S.min(x, 11) < 10, x < 10),
        Param(S.min(x, 8) < 10, _const(1, "bool")),
        Param(S.max(8, x) > 10, _lt(10, x)),
        Param(x + 1 < S.max(8, x), x < 7),
        Param(x < 11, _const(1, "bool"), x <= 10),
        Param(x <= 10, _const(1, "bool"), x <= 10),
        Param(z <= 5, _const(1, "bool"), z <= 5),
        Param(x + y <= 10, _const(1, "bool"), [x <= 10, y <= 0]),
        Param(x + y >= -10, _const(1, "bool"), [x >= 0, y >= -10]),
        Param(z - 5 <= y + 10, _const(1, "bool"), [z <= 5, y >= -10]),
        Param(S.logical_and(x > -1, z <= x + 5), _const(1, "bool"), [x >= 0, z <= 5]),
        Param(x * y <= 0, _const(1, "bool"), [x >= 0, y <= 0]),
        Param((x + 1) * (y - 1) < 0, _const(1, "bool"), [x >= 0, y <= 0]),
        Param(y * y >= 0, _const(1, "bool"), y <= 0),
        Param(x * 6 <= -3, _const(0, "bool"), x >= 0),
        Param(tmod(y - 1, 3) == 0, tmod(y + (-1), 3) == 0),
        Param(
            ((x != 0).astype("int8") - (x != 0).astype("int8")).astype("int32") == 0,  # type: ignore[union-attr]
            S.BoolImm(True),
        ),
    )


class TestLogical(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.Var("z", dtype="int32")

    param = (
        Param(S.logical_and(_eq(x, y), _ne(x, y)), _const(False, "bool")),
        Param(S.logical_and(_ne(x, y), _eq(x, y)), _const(False, "bool")),
        Param(S.logical_and(x > 1, S.logical_not(x > 1)), _const(False, "bool")),
        Param(S.logical_and(x <= y, y < x), _const(False, "bool")),
        Param(S.logical_and(y < x, x <= y), _const(False, "bool")),
        Param(S.logical_and(x < 1, 0 < x), _const(False, "bool")),
        Param(S.logical_and(x < 0, 1 < x), _const(False, "bool")),
        Param(S.logical_and(x < 1, 1 <= x), _const(False, "bool")),
        Param(S.logical_and(x <= 1, 1 < x), _const(False, "bool")),
        Param(S.logical_and(1 <= x, x < 1), _const(False, "bool")),
        Param(S.logical_and(1 < x, x <= 1), _const(False, "bool")),
        Param(S.logical_and(x <= 1, 2 <= x), _const(False, "bool")),
        Param(S.logical_and(2 <= x, x <= 1), _const(False, "bool")),
        Param(S.logical_and(x == 1, x != 2), x == 1),
        Param(S.logical_and(x == 1, x == 2), _const(False, "bool")),
        Param(S.logical_or(_eq(x, y), _ne(x, y)), _const(True, "bool")),
        Param(S.logical_or(_ne(x, y), _eq(x, y)), _const(True, "bool")),
        Param(S.logical_or(x > y, S.logical_not(x > y)), _const(True, "bool")),
        Param(S.logical_or(x <= y, y < x), _const(True, "bool")),
        Param(S.logical_or(y < x, y >= x), _const(True, "bool")),
        Param(S.logical_or(x < 1, 0 < x), _const(True, "bool")),
        Param(S.logical_or(0 < x, x < 1), _const(True, "bool")),
        Param(S.logical_or(x < 1, 1 <= x), _const(True, "bool")),
        Param(S.logical_or(x <= 1, 1 < x), _const(True, "bool")),
        Param(S.logical_or(1 <= x, x < 1), _const(True, "bool")),
        Param(S.logical_or(1 < x, x <= 1), _const(True, "bool")),
        Param(S.logical_or(x <= 1, 2 <= x), _const(True, "bool")),
        Param(S.logical_or(2 <= x, x <= 1), _const(True, "bool")),
        Param(S.logical_or(x != 1, x == 2), x != 1),
        Param(S.logical_or(x != 1, x != 2), _const(True, "bool")),
        Param(
            S.logical_or(x == 1, S.logical_or(y == 1, z == 1)),
            S.logical_or(S.logical_or(x == 1, y == 1), z == 1),
        ),
        Param(
            S.logical_and(x == 1, S.logical_and(y == 1, z == 1)),
            S.logical_and(S.logical_and(x == 1, y == 1), z == 1),
        ),
    )


class TestLet(_Test):
    x = S.Var("x", dtype="int32")
    y = S.Var("y", dtype="int32")
    z = S.let(x, 1, x + 1)

    param = (Param(z + z, 4),)


class TestCast(_Test):
    @staticmethod
    def _generate_tests() -> typing.Generator[Param, None, None]:
        x = S.Var("x", dtype="int32")
        dtypes = ["float32", "float16", "int32", "int8", "bool"]
        for dtype1 in dtypes:
            yield Param((x - x).astype(dtype1), _const(0, dtype1))
            yield Param((x == x).astype(dtype1), _const(1, dtype1))  # type: ignore[union-attr] # noqa: PLR0124
            for dtype2 in dtypes:
                for i in [0, 1, 2, 3]:
                    if i <= 1 or (dtype1 != "bool" and dtype2 != "bool"):
                        yield Param(_const(i, dtype2).astype(dtype1), _const(i, dtype1))

    param: typing.ClassVar[list[Param]] = list(_generate_tests.__func__())  # type: ignore[attr-defined]


class TestShiftLeft(_Test):
    z = S.Call("int32", S.Op.get("mlc.sym.left_shift"), [_i32(1), _i32(10)])
    param = (Param(z, _const(1 << 10, "int32")),)


class TestIfThenElse(_Test):
    x = S.Var("x", dtype="int32")
    param = (
        Param(
            S.if_then_else(x > 1, _i32(1), 0),
            S.if_then_else(_lt(1, x), _i32(1), 0),
        ),
        Param(
            S.if_then_else(x < 5, S.if_then_else(x > 1, _i32(1), 0), 0),
            S.if_then_else(S.logical_and(_lt(x, 5), _lt(1, x)), _i32(1), 0),
        ),
        # Param(  TODO: this requires iter simplify
        #     S.if_then_else(x > 2, S.if_then_else(x > 1, _i32(1), 0), 0),
        #     S.if_then_else(_lt(2, x), _i32(1), 0),
        # ),
    )


# TODO: add tests for
# - TestComparisonOfProductAndSum, which requires extensions
# - TestDivZero
