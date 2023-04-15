import mlc.dataclasses as mlcd
import mlc.printer as mlcp
from mlc import ObjectPath


@mlcd.py_class
class Expr(mlcd.PyClass): ...


@mlcd.py_class
class Stmt(mlcd.PyClass): ...


@mlcd.py_class
class Var(Expr):
    name: str

    def __ir_print__(self, printer: mlcp.IRPrinter, path: ObjectPath) -> mlcp.ast.Node:
        return mlcp.ast.Id(self.name, source_paths=[path])


@mlcd.py_class
class Add(Expr):
    lhs: Expr
    rhs: Expr

    def __ir_print__(self, printer: mlcp.IRPrinter, path: ObjectPath) -> mlcp.ast.Node:
        lhs = printer(self.lhs, path["a"])
        rhs = printer(self.rhs, path["b"])
        return lhs + rhs


@mlcd.py_class
class Assign(Stmt):
    lhs: Var
    rhs: Expr

    def __ir_print__(self, printer: mlcp.IRPrinter, path: ObjectPath) -> mlcp.ast.Node:
        rhs = printer(self.rhs, path["b"])
        printer.var_def(self, printer.frames[-1], self.lhs.name)
        lhs = printer(self.lhs, path["a"])
        return mlcp.ast.Assign(lhs=lhs, rhs=rhs)


@mlcd.py_class
class Func(mlcd.PyClass):
    name: str
    args: list[Var]
    stmts: list[Stmt]
    ret: Var

    def __ir_print__(self, printer: mlcp.IRPrinter, path: ObjectPath) -> mlcp.ast.Node:
        with printer.with_frame(mlcp.DefaultFrame()):
            for arg in self.args:
                printer.var_def(arg, printer.frames[-1], arg.name)
            args = [printer(arg, path=path["args"][i]) for i, arg in enumerate(self.args)]
            stmts = [printer(stmt, path=path["stmts"][i]) for i, stmt in enumerate(self.stmts)]
            ret_stmt = mlcp.ast.Return(value=printer(self.ret, path["ret"]))
            return mlcp.ast.Function(
                name=mlcp.ast.Id(self.name),
                args=[mlcp.ast.Assign(lhs=arg, rhs=None) for arg in args],
                decorators=[],
                return_type=None,
                body=[*stmts, ret_stmt],
            )


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
