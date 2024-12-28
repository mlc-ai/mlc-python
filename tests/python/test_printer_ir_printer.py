import mlc.printer as mlcp
from mlc.testing.toy_ir import Add, Assign, Func, Var


def test_var_print() -> None:
    a = Var(name="a")
    assert mlcp.to_python(a) == "a"


def test_add_print() -> None:
    a = Var(name="a")
    b = Var(name="b")
    c = Add(lhs=a, rhs=b)
    assert mlcp.to_python(c) == "a + b"


def test_assign_print() -> None:
    a = Var(name="a")
    b = Var(name="b")
    c = Assign(lhs=a, rhs=b)
    assert mlcp.to_python(c) == "a = b"


def test_func_print() -> None:
    a = Var(name="a")
    b = Var(name="b")
    c = Var(name="c")
    d = Var(name="d")
    e = Var(name="e")
    stmts = [
        Assign(lhs=d, rhs=Add(a, b)),
        Assign(lhs=e, rhs=Add(d, c)),
    ]
    f = Func(name="f", args=[a, b, c], stmts=stmts, ret=e)
    assert (
        mlcp.to_python(f)
        == """
def f(a, b, c):
  d = a + b
  e = d + c
  return e
""".strip()
    )
