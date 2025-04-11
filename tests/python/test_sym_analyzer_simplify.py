import dataclasses
from collections.abc import Mapping
from types import MappingProxyType
from typing import Literal

import pytest
from mlc import sym as S


def pytest_generate_tests(metafunc: pytest.Metafunc) -> None:
    if "param" in metafunc.fixturenames:
        if test_cases := getattr(metafunc.cls, "param", None):
            metafunc.parametrize("param", test_cases)


@pytest.fixture
def analyzer() -> S.Analyzer:
    return S.Analyzer()


def test_index_flatten(analyzer: S.Analyzer) -> None:
    i0 = S.Var("i0", "int64")
    i1 = S.Var("i1", "int64")
    analyzer.bind(i0, S.Range.from_const("int64", 0, 8))
    analyzer.bind(i1, S.Range.from_const("int64", 0, 3))

    i_flattened = i0 * 3 + i1
    before = (i_flattened) // 12 * 12 + (i_flattened) % 12 // 4 * 4 + (i_flattened) % 4
    expected_after = i_flattened
    after = analyzer.simplify(before)
    expected_after.eq_s(after, assert_mode=True)


@pytest.mark.parametrize(
    "dtype",
    (
        "uint8",
        "uint16",
        "uint32",
        "uint64",
        "int8",
        "int16",
        "int32",
        "int64",
        "float16",
        "float32",
        "float64",
    ),
)
def test_can_prove_self_identity(analyzer: S.Analyzer, dtype: str) -> None:
    n = S.Var("n", dtype)
    assert analyzer.can_prove(n == n)  # type: ignore[arg-type] # noqa: PLR0124
    assert analyzer.can_prove_equal(n, n)


class TestSymbolicCompare:
    @dataclasses.dataclass
    class Param:
        expr: S.Expr
        expected: bool = True
        strength: Literal["default", "symbolic_bound"] = "symbolic_bound"
        bounds: Mapping[S.Var, S.Range] = dataclasses.field(default_factory=dict)

    i0 = S.Var("i0", "int64")
    i1 = S.Var("i1", "int64")
    n = S.ShapeVar("n", "int64")
    m = S.ShapeVar("m", "int64")
    bounds = MappingProxyType(
        {
            i0: S.Range(0, (n + 31) // 32),
            i1: S.Range.from_const("int64", 0, 32),
        }
    )

    param = (
        Param(
            i0 * 32 + i1 < (n + 31) // 32 * 32,
            strength="default",
            expected=False,
            bounds=bounds,
        ),
        Param(i0 * 32 + i1 < (n + 31) // 32 * 32, bounds=bounds),
        Param(i0 * 32 + i1 < (n + 31) // 32 * 32 + m, bounds=bounds),
        Param(i0 * 32 + i1 + 1 <= (n + 31) // 32 * 32, bounds=bounds),
        Param((n + 31) // 32 * 32 >= i0 * 32 + i1 + 1, bounds=bounds),
        Param((n + 31) // 32 * 32 >= i0 * 32 + i1, bounds=bounds),
    )

    @staticmethod
    def test_body(analyzer: S.Analyzer, param: Param) -> None:
        if param.bounds:
            for var, bound in param.bounds.items():
                analyzer.bind(var, bound)
        assert analyzer.can_prove(param.expr, strength=param.strength) == param.expected
