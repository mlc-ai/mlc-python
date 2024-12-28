from __future__ import annotations

from mlc.testing import toy_ir
from mlc.testing.toy_ir import Add, Assign, Func, Var


def test_parse_func() -> None:
    def source_code(a, b, c):  # noqa: ANN001, ANN202
        d = a + b
        e = d + c
        return e

    def _expected() -> Func:
        a = Var(name="_a")
        b = Var(name="_b")
        c = Var(name="_c")
        d = Var(name="_d")
        e = Var(name="_e")
        stmts = [
            Assign(lhs=d, rhs=Add(a, b)),
            Assign(lhs=e, rhs=Add(d, c)),
        ]
        f = Func(name="_f", args=[a, b, c], stmts=stmts, ret=e)
        return f

    result = toy_ir.parse_func(source_code)
    expected = _expected()
    result.eq_s(expected, assert_mode=True)
