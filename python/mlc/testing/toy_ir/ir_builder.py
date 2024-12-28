from __future__ import annotations

from types import TracebackType
from typing import Any, ClassVar

from .ir import Func, Stmt, Var


class IRBuilder:
    _ctx: ClassVar[list[IRBuilder]] = []
    frames: list[Any]
    result: Any

    def __init__(self) -> None:
        self.frames = []
        self.result = None

    def __enter__(self) -> IRBuilder:
        IRBuilder._ctx.append(self)
        return self

    def __exit__(
        self,
        exc_type: type[BaseException],
        exc_value: BaseException,
        traceback: TracebackType,
    ) -> None:
        IRBuilder._ctx.pop()

    @staticmethod
    def get() -> IRBuilder:
        return IRBuilder._ctx[-1]


class FunctionFrame:
    name: str
    args: list[Var]
    stmts: list[Stmt]
    ret: Var | None

    def __init__(self, name: str) -> None:
        self.name = name
        self.args = []
        self.stmts = []
        self.ret = None

    def add_arg(self, arg: Var) -> Var:
        self.args.append(arg)
        return arg

    def __enter__(self) -> FunctionFrame:
        IRBuilder.get().frames.append(self)
        return self

    def __exit__(
        self,
        exc_type: type[BaseException],
        exc_value: BaseException,
        traceback: TracebackType,
    ) -> None:
        frame = IRBuilder.get().frames.pop()
        assert frame is self
        if exc_type is None:
            IRBuilder.get().result = Func(
                name=self.name,
                args=frame.args,
                stmts=frame.stmts,
                ret=frame.ret,
            )
