import mlc.dataclasses as mlcd
import mlc.printer as mlcp
from mlc.printer import ast as mlt


@mlcd.py_class
class Expr(mlcd.PyClass): ...


@mlcd.py_class
class Stmt(mlcd.PyClass): ...


@mlcd.py_class
class Var(Expr):
    name: str

    def __ir_print__(self, printer: mlcp.IRPrinter, path: mlcp.ObjectPath) -> mlt.Node:
        if not printer.var_is_defined(obj=self):
            printer.var_def(obj=self, frame=printer.frames[-1], name=self.name)
        ret = printer.var_get(obj=self)
        assert isinstance(ret, mlt.Id)
        return ret


@mlcd.py_class
class Add(Expr):
    lhs: Expr
    rhs: Expr

    def __ir_print__(self, printer: mlcp.IRPrinter, path: mlcp.ObjectPath) -> mlt.Node:
        lhs: mlt.Expr = printer(obj=self.lhs, path=path["a"])
        rhs: mlt.Expr = printer(obj=self.rhs, path=path["b"])
        return lhs + rhs


@mlcd.py_class
class Assign(Stmt):
    lhs: Var
    rhs: Expr

    def __ir_print__(self, printer: mlcp.IRPrinter, path: mlcp.ObjectPath) -> mlt.Node:
        rhs: mlt.Expr = printer(obj=self.rhs, path=path["b"])
        printer.var_def(obj=self.lhs, frame=printer.frames[-1], name=self.lhs.name)
        lhs: mlt.Expr = printer(obj=self.lhs, path=path["a"])
        return mlt.Assign(lhs=lhs, rhs=rhs)


@mlcd.py_class
class Func(mlcd.PyClass):
    name: str
    args: list[Var]
    stmts: list[Stmt]
    ret: Var

    def __ir_print__(self, printer: mlcp.IRPrinter, path: mlcp.ObjectPath) -> mlt.Node:
        with printer.with_frame(mlcp.DefaultFrame()):
            for arg in self.args:
                printer.var_def(obj=arg, frame=printer.frames[-1], name=arg.name)
            args: list[mlt.Expr] = [
                printer(obj=arg, path=path["args"][i]) for i, arg in enumerate(self.args)
            ]
            stmts: list[mlt.Expr] = [
                printer(obj=stmt, path=path["stmts"][i]) for i, stmt in enumerate(self.stmts)
            ]
            ret_stmt = mlt.Return(printer(obj=self.ret, path=path["ret"]))
            return mlt.Function(
                name=mlt.Id(self.name),
                args=[mlt.Assign(lhs=arg, rhs=None) for arg in args],
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
