from __future__ import annotations

from collections.abc import Callable, Generator
from typing import Any

from mlc.core import Func, Object
from mlc.dataclasses import c_class

Stmt = Any
Var = Any


@c_class("mlc.core.DepNode")
class DepNode(Object):
    stmt: Stmt
    input_vars: list[Var]
    output_vars: list[Var]
    _prev: Object
    _next: Object

    @property
    def prev(self) -> DepNode | None:
        return self._prev

    @property
    def next(self) -> DepNode | None:
        return self._next


@c_class("mlc.core.DepGraph", init=False)
class DepGraph(Object):
    _stmt_to_inputs: Func
    _stmt_to_outputs: Func
    _stmt_to_node: dict
    _var_to_producer: dict
    _var_to_consumers: dict
    _head: DepNode

    @staticmethod
    def from_stmts(
        input_vars: list[Var],
        stmts: list[Stmt],
        stmt_to_inputs: Callable[[Stmt], list[Var]],
        stmt_to_outputs: Callable[[Stmt], list[Var]],
    ) -> DepGraph:
        return DepGraph._C(
            b"_init_from_stmts",
            input_vars,
            stmts,
            stmt_to_inputs,
            stmt_to_outputs,
        )

    def clear(self) -> None:
        DepGraph._C(b"clear", self)

    def create_node(self, stmt: Stmt) -> DepNode:
        return DepGraph._C(b"create_node", self, stmt)

    def get_node_from_stmt(self, stmt: Stmt) -> DepNode:
        return DepGraph._C(b"get_node_from_stmt", self, stmt)

    def insert_before(self, anchor: DepNode, node: DepNode) -> None:
        DepGraph._C(b"insert_before", self, anchor, node)

    def insert_after(self, anchor: DepNode, node: DepNode) -> None:
        DepGraph._C(b"insert_after", self, anchor, node)

    def erase_node(self, to_erase: DepNode) -> None:
        DepGraph._C(b"erase_node", self, to_erase)

    def replace(self, old_node: DepNode, new_node: DepNode) -> None:
        DepGraph._C(b"replace", self, old_node, new_node)

    def get_node_producers(self, node: DepNode) -> list[DepNode]:
        return DepGraph._C(b"get_node_producers", self, node)

    def get_node_consumers(self, node: DepNode) -> list[DepNode]:
        return DepGraph._C(b"get_node_consumers", self, node)

    def get_var_producer(self, v: Var) -> DepNode:
        return DepGraph._C(b"get_var_producer", self, v)

    def get_var_consumers(self, v: Var) -> list[DepNode]:
        return DepGraph._C(b"get_var_consumers", self, v)

    @property
    def nodes(self) -> Generator[DepNode, None, None]:
        node: DepNode | None = self._head
        while node is not None:
            yield node
            node = node.next
