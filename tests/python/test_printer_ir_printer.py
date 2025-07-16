import re

import mlc.printer as mlcp
import pytest
from mlc.printer import ObjectPath
from mlc.testing.toy_ir import Add, Assign, Func, Stmt, Var


def test_var_print() -> None:
    a = Var(name="a")
    assert mlcp.to_python(a) == "a"


def test_var_print_name_normalize() -> None:
    a = Var(name="a/0/b")
    assert mlcp.to_python(a) == "a_0_b"
    assert mlcp.to_python(a) == "a_0_b"


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
    stmts: list[Stmt] = [
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


def test_print_none() -> None:
    printer = mlcp.IRPrinter()
    path = mlcp.ObjectPath.root()
    node = printer(None, path)
    assert node.to_python() == "None"


def test_print_int() -> None:
    printer = mlcp.IRPrinter()
    path = mlcp.ObjectPath.root()
    node = printer(42, path)
    assert node.to_python() == "42"


def test_print_str() -> None:
    printer = mlcp.IRPrinter()
    path = mlcp.ObjectPath.root()
    node = printer("hey", path)
    assert node.to_python() == '"hey"'


def test_print_bool() -> None:
    printer = mlcp.IRPrinter()
    path = mlcp.ObjectPath.root()
    node = printer(True, path)
    assert node.to_python() == "True"


def test_duplicated_vars() -> None:
    a = Var(name="a")
    b = Var(name="a")
    f = Func(
        name="f",
        args=[a],
        stmts=[Assign(lhs=b, rhs=Add(a, a))],
        ret=b,
    )
    assert (
        mlcp.to_python(f)
        == """
def f(a):
  a_1 = a + a
  return a_1
""".strip()
    )
    assert re.fullmatch(
        r"^def f\(a\):\n"
        r"  a_0x[0-9A-Fa-f]+ = a \+ a\n"
        r"  return a_0x[0-9A-Fa-f]+$",
        mlcp.to_python(f, mlcp.PrinterConfig(print_addr_on_dup_var=True)),
    )


@pytest.mark.parametrize(
    "path, expected",
    [
        (
            ObjectPath.root()["args"][0],
            """
def f(a, b):
      ^
  c = a + b
  return c
""",
        ),
        (
            ObjectPath.root()["args"][1],
            """
def f(a, b):
         ^
  c = a + b
  return c
""",
        ),
        (
            ObjectPath.root()["stmts"][0],
            """
def f(a, b):
  c = a + b
  ^^^^^^^^^
  return c
""",
        ),
        (
            ObjectPath.root()["stmts"][0],
            """
def f(a, b):
  c = a + b
  ^^^^^^^^^
  return c
""",
        ),
        (
            ObjectPath.root()["stmts"][0]["lhs"],
            """
def f(a, b):
  c = a + b
  ^
  return c
""",
        ),
        (
            ObjectPath.root()["stmts"][0]["rhs"],
            """
def f(a, b):
  c = a + b
      ^^^^^
  return c
""",
        ),
        (
            ObjectPath.root()["stmts"][0]["rhs"]["lhs"],
            """
def f(a, b):
  c = a + b
      ^
  return c
""",
        ),
        (
            ObjectPath.root()["stmts"][0]["rhs"]["rhs"],
            """
def f(a, b):
  c = a + b
          ^
  return c
""",
        ),
        (
            ObjectPath.root()["ret"],
            """
def f(a, b):
  c = a + b
  return c
         ^
""",
        ),
    ],
)
def test_print_underscore(path: ObjectPath, expected: str) -> None:
    a = Var(name="a")
    b = Var(name="b")
    c = Var(name="c")
    f = Func(
        name="f",
        args=[a, b],
        stmts=[
            Assign(lhs=c, rhs=Add(a, b)),
        ],
        ret=c,
    )
    actual = mlcp.to_python(
        f,
        mlcp.PrinterConfig(path_to_underline=[path]),
    )
    assert actual.strip() == expected.strip()
