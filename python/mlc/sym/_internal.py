"""System internals that are not part of the public API. Testing-only."""

from __future__ import annotations

import contextlib
from collections.abc import Generator

import mlc.dataclasses as mlcd
from mlc.core import Func, Object

from .analyzer import Analyzer
from .expr import Expr, Var, const


@mlcd.c_class("mlc.sym.ConstIntBound")
class ConstIntBound(Object):
    min_value: int
    max_value: int


@mlcd.c_class("mlc.sym.IntervalSet", init=False)
class IntervalSet(Object):
    min_value: Expr
    max_value: Expr

    def __init__(self, min_value: Expr | int, max_value: Expr | int) -> None:
        if isinstance(min_value, int) and isinstance(max_value, int):
            min_value = const("int32", min_value)
            max_value = const("int32", max_value)
        if isinstance(min_value, int):
            assert isinstance(max_value, Expr)
            min_value = const(max_value.dtype, min_value)
        if isinstance(max_value, int):
            assert isinstance(min_value, Expr)
            max_value = const(min_value.dtype, max_value)
        self._mlc_init(min_value, max_value)


@mlcd.c_class("mlc.sym.ModularSet")
class ModularSet(Object):
    coeff: int
    base: int


def const_int_bound(analyzer: Analyzer, expr: Expr) -> ConstIntBound:
    return _Analyzer_ConstIntBound(analyzer, expr)


def modular_set(analyzer: Analyzer, expr: Expr) -> ModularSet:
    return _Analyzer_ModularSet(analyzer, expr)


def rewrite_simplify(analyzer: Analyzer, expr: Expr) -> Expr:
    return _Analyzer_RewriteSimplify(analyzer, expr)


def canonical_simplify(analyzer: Analyzer, expr: Expr) -> Expr:
    return _Analyzer_CanonicalSimplify(analyzer, expr)


def interval_set(
    analyzer: Analyzer,
    expr: Expr,
    dom_map: dict[Var, IntervalSet],
) -> IntervalSet:
    return _Analyzer_IntervalSet(analyzer, expr, dom_map)


def const_int_bound_update(
    analyzer: Analyzer,
    var: Var,
    info: ConstIntBound,
    allow_override: bool = False,
) -> None:
    return _Analyzer_ConstIntBoundUpdate(analyzer, var, info, allow_override)


def get_enabled_extensions(analyzer: Analyzer) -> int:
    return _Analyzer_GetEnabledExtensions(analyzer)


def set_enabled_extensions(analyzer: Analyzer, flags: int) -> None:
    return _Analyzer_SetEnabledExtensions(analyzer, flags)


@contextlib.contextmanager
def enter_constraint(analyzer: Analyzer, constraint: Expr | None) -> Generator[None, None, None]:
    if constraint is None:
        yield
        return
    exit_constraint: Func = _Analyzer_EnterConstraint(analyzer, constraint)
    try:
        yield
    finally:
        exit_constraint()


_Analyzer_ConstIntBound = Func.get("mlc.sym._internal.Analyzer.ConstIntBound")
_Analyzer_ModularSet = Func.get("mlc.sym._internal.Analyzer.ModularSet")
_Analyzer_RewriteSimplify = Func.get("mlc.sym._internal.Analyzer.RewriteSimplify")
_Analyzer_CanonicalSimplify = Func.get("mlc.sym._internal.Analyzer.CanonicalSimplify")
_Analyzer_IntervalSet = Func.get("mlc.sym._internal.Analyzer.IntervalSet")
_Analyzer_ConstIntBoundUpdate = Func.get("mlc.sym._internal.Analyzer.ConstIntBoundUpdate")
_Analyzer_GetEnabledExtensions = Func.get("mlc.sym._internal.Analyzer.GetEnabledExtensions")
_Analyzer_SetEnabledExtensions = Func.get("mlc.sym._internal.Analyzer.SetEnabledExtensions")
_Analyzer_EnterConstraint = Func.get("mlc.sym._internal.Analyzer.EnterConstraint")
