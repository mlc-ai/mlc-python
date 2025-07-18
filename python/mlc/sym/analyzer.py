from __future__ import annotations

from typing import TYPE_CHECKING, Literal

import mlc.dataclasses as mlcd

if TYPE_CHECKING:
    from .expr import Expr, Range, Var


@mlcd.c_class("mlc.sym.Analyzer")
class Analyzer:
    def mark_global_non_neg_value(self, v: Expr) -> None:
        Analyzer._C(b"_mark_global_non_neg_value", self, v)  # type: ignore[attr-defined]

    def bind(
        self,
        v: Var,
        bound: Range | Expr | int | float,
        allow_override: bool = False,
    ) -> None:
        from .expr import Expr, Range, const  # noqa: PLC0415

        if isinstance(bound, Range):
            Analyzer._C(b"_bind_range", self, v, bound, allow_override)  # type: ignore[attr-defined]
        elif isinstance(bound, Expr):
            Analyzer._C(b"_bind_expr", self, v, bound, allow_override)  # type: ignore[attr-defined]
        elif isinstance(bound, (int, float)):
            Analyzer._C(b"_bind_expr", self, v, const(v.dtype, bound), allow_override)  # type: ignore[attr-defined]
        else:
            raise TypeError(f"Unsupported type for bound: {type(bound)}")

    def can_prove_greater_equal(self, a: Expr, b: int) -> bool:
        assert isinstance(b, int)
        return Analyzer._C(b"can_prove_greater_equal", self, a, b)  # type: ignore[attr-defined]

    def can_prove_less(self, a: Expr, b: int) -> bool:
        assert isinstance(b, int)
        return Analyzer._C(b"can_prove_less", self, a, b)  # type: ignore[attr-defined]

    def can_prove_equal(self, a: Expr, b: Expr | int) -> bool:
        from .expr import Expr, const  # noqa: PLC0415

        assert isinstance(a, Expr)
        if isinstance(b, int):
            b = const(a.dtype, b)
        return Analyzer._C(b"can_prove_equal", self, a, b)  # type: ignore[attr-defined]

    def can_prove_less_equal_than_symbolic_shape_value(self, a: Expr, b: Expr) -> bool:
        return Analyzer._C(b"can_prove_less_equal_than_symbolic_shape_value", self, a, b)  # type: ignore[attr-defined]

    def can_prove(
        self,
        cond: Expr,
        *,
        strength: Literal["default", "symbolic_bound"] = "default",
    ) -> bool:
        return Analyzer._C(b"can_prove", self, cond, _STRENGTH[strength])  # type: ignore[attr-defined]

    def simplify(self, expr: Expr, *, steps: int = 2) -> Expr:
        return Analyzer._C(b"simplify", self, expr, steps)  # type: ignore[attr-defined]


_STRENGTH = {
    "default": 0,
    "symbolic_bound": 1,
}
