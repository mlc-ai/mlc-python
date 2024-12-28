from __future__ import annotations

import mlc.dataclasses as mlcd
import mlc.printer as mlcp
import mlc.printer.ast as mlt


@mlcd.py_class
class Node(mlcd.PyClass): ...


@mlcd.py_class
class Expr(Node): ...


@mlcd.py_class
class Stmt(Node): ...


@mlcd.py_class(structure="var")
class Var(Expr):
    name: str = mlcd.field(structure=None)

    def __add__(self, other: Var) -> Add:
        return Add(lhs=self, rhs=other)

    def __ir_print__(self, printer: mlcp.IRPrinter, path: mlcp.ObjectPath) -> mlt.Node:
        if not printer.var_is_defined(obj=self):
            printer.var_def(self.name, obj=self)
        ret = printer.var_get(obj=self)
        assert ret is not None
        return ret


@mlcd.py_class(structure="nobind")
class Add(Expr):
    lhs: Expr
    rhs: Expr

    def __ir_print__(self, printer: mlcp.IRPrinter, path: mlcp.ObjectPath) -> mlt.Node:
        lhs: mlt.Expr = printer(self.lhs, path=path["a"])
        rhs: mlt.Expr = printer(self.rhs, path=path["b"])
        return lhs + rhs


@mlcd.py_class(structure="bind")
class Assign(Stmt):
    rhs: Expr
    lhs: Var = mlcd.field(structure="bind")

    def __ir_print__(self, printer: mlcp.IRPrinter, path: mlcp.ObjectPath) -> mlt.Node:
        rhs: mlt.Expr = printer(self.rhs, path=path["b"])
        printer.var_def(self.lhs.name, obj=self.lhs)
        lhs: mlt.Expr = printer(self.lhs, path=path["a"])
        return mlt.Assign(lhs=lhs, rhs=rhs)


@mlcd.py_class(structure="bind")
class Func(Node):
    name: str = mlcd.field(structure=None)
    args: list[Var] = mlcd.field(structure="bind")
    stmts: list[Stmt]
    ret: Var

    def __ir_print__(self, printer: mlcp.IRPrinter, path: mlcp.ObjectPath) -> mlt.Node:
        with printer.with_frame(mlcp.DefaultFrame()):
            for arg in self.args:
                printer.var_def(arg.name, obj=arg)
            args: list[mlt.Expr] = [
                printer(arg, path=path["args"][i]) for i, arg in enumerate(self.args)
            ]
            stmts: list[mlt.Expr] = [
                printer(stmt, path=path["stmts"][i]) for i, stmt in enumerate(self.stmts)
            ]
            ret_stmt = mlt.Return(printer(self.ret, path=path["ret"]))
            return mlt.Function(
                name=mlt.Id(self.name),
                args=[mlt.Assign(lhs=arg, rhs=None) for arg in args],
                decorators=[],
                return_type=None,
                body=[*stmts, ret_stmt],
            )
