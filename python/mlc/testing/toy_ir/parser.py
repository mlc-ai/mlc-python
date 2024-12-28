from __future__ import annotations

import ast
from typing import Callable

import mlc.parser as mlcs

from .ir import Assign, Node, Var
from .ir_builder import FunctionFrame, IRBuilder


class Parser(ast.NodeVisitor):
    base: mlcs.Parser

    def __init__(self, env: mlcs.Env) -> None:
        self.base = mlcs.Parser(env, include_builtins=True, extra_vars=None)

    def visit_Assign(self, node: ast.Assign) -> None:
        if len(node.targets) != 1:
            self.base.report_error(node, "Continuous assignment is not supported")
        (target,) = node.targets
        if not isinstance(target, ast.Name):
            self.base.report_error(target, "Invalid assignment target")
        assert isinstance(target, ast.Name)
        value = self.base.eval_assign(
            target=target,
            source=self.base.eval_expr(node.value),
        )[target.id]
        var = Var(name=target.id)
        self.base.var_def(name=target.id, value=var)
        IRBuilder.get().frames[-1].stmts.append(
            Assign(
                lhs=var,
                rhs=value,
            )
        )

    def visit_FunctionDef(self, node: ast.FunctionDef) -> None:
        with mlcs.Frame().scope(self.base):
            with FunctionFrame(node.name) as frame:
                for node_arg in node.args.args:
                    self.base.var_def(
                        name=node_arg.arg,
                        value=frame.add_arg(Var(name=node_arg.arg)),
                    )
                for node_stmt in node.body:
                    self.visit(node_stmt)

    def visit_Return(self, node: ast.Return) -> None:
        if not isinstance(node.value, ast.Name):
            self.base.report_error(node, "Return statement must return a single variable")
        assert isinstance(node.value, ast.Name)
        frame: FunctionFrame = IRBuilder.get().frames[-1]
        frame.ret = self.base.eval_expr(node.value)


def parse_func(source: Callable) -> Node:
    env = mlcs.Env.from_function(source)
    parser = Parser(env)
    node = ast.parse(
        env.source,
        filename=env.source_name,
    )
    with IRBuilder() as ib:
        parser.visit(node)
    return ib.result
