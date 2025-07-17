from __future__ import annotations

import mlc
import mlc.dataclasses as mlcd
import pytest


@mlcd.py_class
class Expr:
    def __add__(self, other: Expr) -> Expr:
        return Add(a=self, b=other)


@mlcd.py_class(structure="var")
class Var(Expr):
    name: str = mlcd.field(structure=None)
    is_global: bool = mlcd.field(default=False)


@mlcd.py_class(structure="nobind")
class Add(Expr):
    a: Expr
    b: Expr


@mlcd.py_class(structure="bind")
class AddBind(Expr):
    a: Expr
    b: Expr


@mlcd.py_class(structure="bind")
class Let(Expr):
    rhs: Expr
    lhs: Var = mlcd.field(structure="bind")
    body: Expr


@mlcd.py_class(structure="bind")
class Func:
    name: str = mlcd.field(structure=None)
    args: list[Var] = mlcd.field(structure="bind")
    body: Expr


@mlcd.py_class(structure="nobind")
class Constant(Expr):
    value: int


@mlcd.py_class(structure="bind")
class TensorType:
    shape: list[int]
    dtype: mlc.DataType


@mlcd.py_class(structure="bind")
class Stmt: ...


@mlcd.py_class(structure="bind")
class FuncStmts:
    name: str = mlcd.field(structure=None)
    args: list[Var] = mlcd.field(structure="bind")
    stmts: list[Stmt]


@mlcd.py_class(structure="bind")
class AssignStmt(Stmt):
    rhs: Expr
    lhs: Var = mlcd.field(structure="bind")


def test_free_var_1() -> None:
    x = Var("x")
    lhs = x
    rhs = x
    mlc.eq_s(lhs, rhs, bind_free_vars=True, assert_mode=True)
    assert mlc.hash_s(lhs) == mlc.hash_s(rhs)
    with pytest.raises(ValueError) as e:
        mlc.eq_s(lhs, rhs, bind_free_vars=False, assert_mode=True)
    assert str(e.value) == "Structural equality check failed at {root}: Unbound variable"
    assert str(e.value) == mlc.eq_s_fail_reason(lhs, rhs, bind_free_vars=False)


def test_free_var_2() -> None:
    x = Var("x")
    lhs = Add(x, x)
    rhs = Add(x, x)
    mlc.eq_s(lhs, rhs, bind_free_vars=True, assert_mode=True)
    assert mlc.hash_s(lhs) == mlc.hash_s(rhs)
    with pytest.raises(ValueError) as e:
        mlc.eq_s(lhs, rhs, bind_free_vars=False, assert_mode=True)
    assert str(e.value) == "Structural equality check failed at {root}.a: Unbound variable"
    assert str(e.value) == mlc.eq_s_fail_reason(lhs, rhs, bind_free_vars=False)


def test_cyclic() -> None:
    x = Var("x")
    y = Var("y")
    z = Var("z")
    lhs = x + y + z
    rhs = y + z + x
    mlc.eq_s(lhs, rhs, bind_free_vars=True, assert_mode=True)
    assert mlc.hash_s(lhs) == mlc.hash_s(rhs)


def test_tensor_type() -> None:
    t1 = TensorType(shape=(1, 2, 3), dtype="float32")  # type: ignore[arg-type]
    t2 = TensorType(shape=(1, 2, 3), dtype="float32")  # type: ignore[arg-type]
    t3 = TensorType(shape=(1, 2, 3), dtype="int32")  # type: ignore[arg-type]
    t4 = TensorType(shape=(1, 2), dtype="float32")  # type: ignore[arg-type]
    # t1 == t2
    mlc.eq_s(t1, t2, bind_free_vars=False, assert_mode=True)
    assert mlc.hash_s(t1) == mlc.hash_s(t2)
    # t1 != t3, dtype mismatch
    with pytest.raises(ValueError) as e:
        mlc.eq_s(t1, t3, bind_free_vars=False, assert_mode=True)
    assert str(e.value) == "Structural equality check failed at {root}.dtype: float32 vs int32"
    assert str(e.value) == mlc.eq_s_fail_reason(t1, t3, bind_free_vars=False)
    assert mlc.hash_s(t1) != mlc.hash_s(t3)
    # t1 != t4, shape mismatch
    with pytest.raises(ValueError) as e:
        mlc.eq_s(t1, t4, bind_free_vars=False, assert_mode=True)
    assert (
        str(e.value)
        == "Structural equality check failed at {root}.shape: List length mismatch: 3 vs 2"
    )
    assert str(e.value) == mlc.eq_s_fail_reason(t1, t4, bind_free_vars=False)
    assert mlc.hash_s(t1) != mlc.hash_s(t4)


def test_constant() -> None:
    c1 = Constant(1)
    c2 = Constant(1)
    c3 = Constant(2)
    mlc.eq_s(c1, c2, bind_free_vars=False, assert_mode=True)
    with pytest.raises(ValueError) as e:
        mlc.eq_s(c1, c3, bind_free_vars=False, assert_mode=True)
    assert str(e.value) == "Structural equality check failed at {root}.value: 1 vs 2"
    assert str(e.value) == mlc.eq_s_fail_reason(c1, c3, bind_free_vars=False)
    assert mlc.hash_s(c1) == mlc.hash_s(c2)
    assert mlc.hash_s(c1) != mlc.hash_s(c3)


def test_let_1() -> None:
    x = Var("x")
    y = Var("y")
    """
    LHS:
        let y = x + x;
        y

    RHS:
        let x = y + y;
        x
    """
    lhs = Let(rhs=x + x, lhs=y, body=y)
    rhs = Let(rhs=y + y, lhs=x, body=x)
    mlc.eq_s(lhs, rhs, bind_free_vars=True, assert_mode=True)
    with pytest.raises(ValueError) as e:
        mlc.eq_s(lhs, rhs, bind_free_vars=False, assert_mode=True)
    assert str(e.value) == "Structural equality check failed at {root}.rhs.a: Unbound variable"
    assert str(e.value) == mlc.eq_s_fail_reason(lhs, rhs, bind_free_vars=False)
    assert mlc.hash_s(lhs) == mlc.hash_s(rhs)


def test_let_2() -> None:
    v1 = Var("v1")
    v2 = Var("v2")
    """
    l1: let v1 = 1; v1
    l2: let v2 = 1; v2
    l3: let v1 = 2; v1
    """
    l1 = Let(rhs=Constant(1), lhs=v1, body=v1)
    l2 = Let(rhs=Constant(1), lhs=v2, body=v2)
    l3 = Let(rhs=Constant(2), lhs=v1, body=v1)
    mlc.eq_s(l1, l2, bind_free_vars=True, assert_mode=True)
    with pytest.raises(ValueError) as e:
        mlc.eq_s(l1, l3, bind_free_vars=True, assert_mode=True)
    assert str(e.value) == "Structural equality check failed at {root}.rhs.value: 1 vs 2"
    assert str(e.value) == mlc.eq_s_fail_reason(l1, l3, bind_free_vars=True)
    assert mlc.hash_s(l1) == mlc.hash_s(l2)
    assert mlc.hash_s(l1) != mlc.hash_s(l3)


def test_non_scoped_compute_1() -> None:
    x = Var("x")
    """
    LHS:
        %y = %x + %x;  # no let
        %y + %y

    RHS:
        %y = %x + %x;  # no let
        %z = %x + %x;  # no let
        %y + %z
    """
    y = x + x
    z = x + x
    lhs = y + y
    rhs = y + z
    mlc.eq_s(lhs, rhs, bind_free_vars=True, assert_mode=True)
    assert mlc.hash_s(lhs) == mlc.hash_s(rhs)


def test_non_scoped_compute_2() -> None:
    x = Var("x")
    """
    LHS:
        %y = %x + %x;  # no let
        %y + %y

    RHS:
        %y = %x + %x;  # no let
        %z = %x + %x;  # no let
        %y + %z
    """
    y = AddBind(x, x)
    z = AddBind(x, x)
    lhs = AddBind(y, y)
    rhs = AddBind(y, z)
    with pytest.raises(ValueError) as e:
        mlc.eq_s(lhs, rhs, bind_free_vars=True, assert_mode=True)
    assert (
        str(e.value) == "Structural equality check failed at {root}.b: Inconsistent binding. "
        "LHS has been bound to a different node while RHS is not bound"
    )
    assert str(e.value) == mlc.eq_s_fail_reason(lhs, rhs, bind_free_vars=True)
    assert mlc.hash_s(lhs) != mlc.hash_s(rhs)


def test_func_1() -> None:
    """
    LHS:
        fn %lhs(%x: Var, %y: Var) {
            let %z = %x + %y;
            %z + %z
        }

    RHS:
        fn %rhs(%y: Var, %x: Var) {
            let %z = %y + %x;
            %z + %z
        }
    """
    x = Var("x")
    y = Var("y")
    z = Var("z")
    lhs = Func("lhs", args=[x, y], body=Let(rhs=x + y, lhs=z, body=z + z))
    rhs = Func("rhs", args=[y, x], body=Let(rhs=y + x, lhs=z, body=z + z))
    mlc.eq_s(lhs, rhs, bind_free_vars=False, assert_mode=True)
    assert mlc.hash_s(lhs) == mlc.hash_s(rhs)


def test_func_2() -> None:
    """
    fn %l1(%x: Var, %y: Var, %z: Var, %d: Var) {
        %x + %y + %z
    }

    fn %l2(%z: Var, %x: Var, %y: Var: %d: Var) {
        %z + %x + %y
    }

    fn %l3(%x: Var, %y: Var, %z: Var) {
        %x + %y + %z
    }
    """
    x = Var("x")
    y = Var("y")
    z = Var("z")
    d = Var("d")
    l1 = Func("l1", args=[x, y, z, d], body=x + y + z)
    l2 = Func("l2", args=[z, x, y, d], body=z + x + y)
    l3 = Func("l3", args=[x, y, z], body=x + y + z)
    # l1 == l2
    mlc.eq_s(l1, l2, bind_free_vars=False, assert_mode=True)
    # l1 != l3, arg length mismatch
    with pytest.raises(ValueError) as e:
        mlc.eq_s(l1, l3, bind_free_vars=False, assert_mode=True)
    assert (
        str(e.value)
        == "Structural equality check failed at {root}.args: List length mismatch: 4 vs 3"
    )
    assert str(e.value) == mlc.eq_s_fail_reason(l1, l3, bind_free_vars=False)
    assert mlc.hash_s(l1) == mlc.hash_s(l2)
    assert mlc.hash_s(l1) != mlc.hash_s(l3)


def test_func_stmts() -> None:
    """
    fn %f(%a: Var, %b: Var) {
        %c = %a + %b;
        %d = %a + %c;
    }

    fn %g(%a: Var, %b: Var) {
        %d = %a + %c;
        %c = %a + %b;
    }
    """
    a = Var("a")
    b = Var("b")
    c = Var("c")
    d = Var("d")
    func_f = FuncStmts(
        name="f",
        args=[a, b],
        stmts=[
            AssignStmt(rhs=a + b, lhs=c),
            AssignStmt(rhs=a + c, lhs=d),
        ],
    )
    func_g = FuncStmts(
        name="g",
        args=[a, b],
        stmts=[
            AssignStmt(rhs=a + c, lhs=d),
            AssignStmt(rhs=a + b, lhs=c),
        ],
    )
    mlc.eq_s(func_f, func_f, bind_free_vars=False, assert_mode=True)
    with pytest.raises(ValueError) as e:
        mlc.eq_s(func_f, func_g, bind_free_vars=False, assert_mode=True)
    assert (
        str(e.value)
        == "Structural equality check failed at {root}.stmts[0].rhs.b: Inconsistent binding. "
        "LHS has been bound to a different node while RHS is not bound"
    )
    assert str(e.value) == mlc.eq_s_fail_reason(func_f, func_g, bind_free_vars=False)
    assert mlc.hash_s(func_f) != mlc.hash_s(func_g)


def test_global_var() -> None:
    x = Var("x", is_global=False)
    y = Var("y", is_global=True)
    z = Var("z", is_global=True)
    lhs = x + y + z
    rhs = z + y + x
    with pytest.raises(ValueError) as e:
        mlc.eq_s(lhs, rhs, bind_free_vars=True, assert_mode=True)
    assert str(e.value) == "Structural equality check failed at {root}.a.a.is_global: False vs True"
    assert str(e.value) == mlc.eq_s_fail_reason(lhs, rhs, bind_free_vars=True)
    assert mlc.hash_s(lhs) != mlc.hash_s(rhs)

    lhs = x + y + z
    rhs = x + z + y
    mlc.eq_s(lhs, rhs, bind_free_vars=True, assert_mode=True)
    assert mlc.hash_s(lhs) == mlc.hash_s(rhs)
