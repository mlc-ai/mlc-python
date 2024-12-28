from __future__ import annotations

import ast
import builtins
import copy
from collections.abc import Generator
from contextlib import contextmanager
from typing import Any

from .diagnostic import DiagnosticError, raise_diagnostic_error
from .env import Env, Span

Value = Any


class Frame:
    name2value: dict[str, Value]

    def __init__(self) -> None:
        self.name2value = {}

    def add(self, name: str, value: Value, override: bool = False) -> None:
        if name in self.name2value and not override:
            raise ValueError(f"Variable already defined in current scope: {name}")
        self.name2value[name] = value

    @contextmanager
    def scope(self, parser: Parser) -> Generator[Frame, None, None]:
        parser.frames.append(self)
        try:
            yield self
        finally:
            parser.frames.pop()


class Parser:
    env: Env
    frames: list[Frame]

    def __init__(
        self,
        env: Env,
        extra_vars: dict[str, Value] | None = None,
        include_builtins: bool = True,
    ) -> None:
        self.env = env
        self.frames = [Frame()]
        if include_builtins:
            for name, value in builtins.__dict__.items():
                self.var_def(name, value)
        if extra_vars is not None:
            for name, value in extra_vars.items():
                self.var_def(name, value)

    def var_def(
        self,
        name: str,
        value: Value,
        frame: Frame | None = None,
        override: bool = False,
    ) -> None:
        if frame is None:
            frame = self.frames[-1]
        frame.add(name, value, override=override)

    def report_error(self, node: ast.AST | Span, err: Exception | str) -> None:
        if isinstance(err, DiagnosticError):
            raise err
        span = Span.from_ast(node, self.env) if isinstance(node, ast.AST) else node
        raise_diagnostic_error(
            source=self.env.source_full,
            source_name=self.env.source_name,
            span=span,
            err=err,
        )

    def eval_expr(self, node: ast.expr) -> Value:
        var_tab: dict[str, Value] = {}
        for frame in self.frames:
            var_tab.update({name: value for name, value in frame.name2value.items()})
        return ExprEvaluator(parser=self, var_tab=var_tab).run(node)

    def eval_assign(self, target: ast.expr, source: Value) -> dict[str, Value]:
        var_name = "__mlc_rhs_var__"
        dict_locals: dict[str, Value] = {var_name: source}
        mod = ast.fix_missing_locations(
            ast.Module(
                body=[
                    ast.Assign(
                        targets=[target],
                        value=ast.Name(id=var_name, ctx=ast.Load()),
                    )
                ],
                type_ignores=[],
            )
        )
        exe = compile(mod, filename="<ast>", mode="exec")
        exec(exe, {}, dict_locals)  # pylint: disable=exec-used
        del dict_locals[var_name]
        return dict_locals


class ExprEvaluator(ast.NodeTransformer):
    parser: Parser
    var_tab: dict[str, Value]
    num_tmp_vars: int

    def __init__(self, parser: Parser, var_tab: dict[str, Value]) -> None:
        self.parser = parser
        self.var_tab = var_tab
        self.num_tmp_vars = 0

    def run(self, node: ast.AST) -> Value:
        var_name = "_mlc_evaluator_tmp_var_result"
        self._eval_subexpr(
            expr=self.visit(node),
            target=ast.Name(id=var_name, ctx=ast.Store()),
        )
        return self.var_tab[var_name]

    def _make_intermediate_var(self) -> ast.Name:
        var_name = f"_mlc_evaluator_tmp_var_{self.num_tmp_vars}"
        self.num_tmp_vars += 1
        return ast.Name(id=var_name, ctx=ast.Store())

    def visit_Name(self, node: ast.Name) -> ast.Name:
        if node.id not in self.var_tab:
            self.parser.report_error(node, f"Undefined variable: {node.id}")
        return node

    def visit_Call(self, node: ast.Call) -> ast.Name:
        node.func = self.visit(node.func)
        node.args = [self.visit(arg) for arg in node.args]
        node.keywords = [self.visit(kw) for kw in node.keywords]
        return self._eval_subexpr(
            expr=node,
            target=self._make_intermediate_var(),
        )

    def visit_UnaryOp(self, node: ast.UnaryOp) -> ast.Name:
        node.operand = self.visit(node.operand)
        return self._eval_subexpr(
            expr=node,
            target=self._make_intermediate_var(),
        )

    def visit_BinOp(self, node: ast.BinOp) -> ast.Name:
        node.left = self.visit(node.left)
        node.right = self.visit(node.right)
        return self._eval_subexpr(
            expr=node,
            target=self._make_intermediate_var(),
        )

    def visit_Compare(self, node: ast.Compare) -> ast.Name:
        node.left = self.visit(node.left)
        node.comparators = [self.visit(comp) for comp in node.comparators]
        return self._eval_subexpr(
            expr=node,
            target=self._make_intermediate_var(),
        )

    def visit_BoolOp(self, node: ast.BoolOp) -> ast.Name:
        node.values = [self.visit(value) for value in node.values]
        return self._eval_subexpr(
            expr=node,
            target=self._make_intermediate_var(),
        )

    def visit_IfExp(self, node: ast.IfExp) -> ast.Name:
        node.test = self.visit(node.test)
        node.body = self.visit(node.body)
        node.orelse = self.visit(node.orelse)
        return self._eval_subexpr(
            expr=node,
            target=self._make_intermediate_var(),
        )

    def visit_Attribute(self, node: ast.Attribute) -> ast.Name:
        node.value = self.visit(node.value)
        return self._eval_subexpr(
            expr=node,
            target=self._make_intermediate_var(),
        )

    def visit_Subscript(self, node: ast.Subscript) -> ast.Name:
        node.value = self.visit(node.value)
        node.slice = self.visit(node.slice)
        return self._eval_subexpr(
            expr=node,
            target=self._make_intermediate_var(),
        )

    def visit_Slice(self, node: ast.Slice) -> ast.Name:
        if node.lower is not None:
            node.lower = self.visit(node.lower)
        if node.upper is not None:
            node.upper = self.visit(node.upper)
        if node.step is not None:
            node.step = self.visit(node.step)
        return self._eval_subexpr(
            expr=node,
            target=self._make_intermediate_var(),
        )

    def _eval_subexpr(self, expr: ast.expr, target: ast.Name) -> ast.Name:
        target.ctx = ast.Store()
        mod = ast.fix_missing_locations(
            ast.Module(
                body=[ast.Assign(targets=[target], value=copy.copy(expr))],  # target = node
                type_ignores=[],
            )
        )
        exe = compile(mod, filename="<ast>", mode="exec")
        try:
            exec(exe, {}, self.var_tab)
        except Exception as err:
            self.parser.report_error(node=expr, err=err)
        target.ctx = ast.Load()
        return target
