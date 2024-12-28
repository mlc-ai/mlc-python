import contextlib
from collections.abc import Generator
from typing import Any, Optional, TypeVar

import mlc.dataclasses as mlcd
from mlc.core import Func, Object, ObjectPath

from .ast import Expr, Node, PrinterConfig, Stmt
from .cprint import cprint


@mlcd.c_class("mlc.printer.VarInfo")
class VarInfo(Object):
    name: Optional[str]
    creator: Func


FrameType = TypeVar("FrameType")


@mlcd.c_class("mlc.printer.DefaultFrame", init=False)
class DefaultFrame(Object):
    stmts: list[Stmt]

    def __init__(self, stmts: Optional[list[Stmt]] = None) -> None:
        if stmts is None:
            stmts = []
        self._mlc_init(stmts)


@mlcd.c_class("mlc.printer.IRPrinter", init=False)
class IRPrinter(Object):
    cfg: PrinterConfig
    obj2info: dict[Any, VarInfo]
    defined_names: dict[str, int]
    frames: list[Any]
    frame_vars: dict[Any, Any]

    def __init__(self, cfg: Optional[PrinterConfig] = None) -> None:
        if cfg is None:
            cfg = PrinterConfig()
        self._mlc_init(cfg, {}, {}, [], {})

    def var_is_defined(self, obj: Any) -> bool:
        return bool(IRPrinter._C(b"var_is_defined", self, obj))

    def var_def(
        self,
        name: str,
        obj: Any,
        frame: Optional[Any] = None,
    ) -> None:
        return IRPrinter._C(b"var_def", self, name, obj, frame)

    def var_def_no_name(
        self,
        creator: Func,
        obj: Any,
        frame: Optional[Any] = None,
    ) -> None:
        IRPrinter._C(b"var_def_no_name", self, creator, obj, frame)

    def var_remove(self, obj: Any) -> None:
        IRPrinter._C(b"var_remove", self, obj)

    def var_get(self, obj: Any) -> Optional[Expr]:
        return IRPrinter._C(b"var_get", self, obj)

    def frame_push(self, frame: Any) -> None:
        IRPrinter._C(b"frame_push", self, frame)

    def frame_pop(self) -> None:
        IRPrinter._C(b"frame_pop", self)

    def __call__(self, obj: Node, path: ObjectPath) -> Node:
        return IRPrinter._C(b"__call__", self, obj, path)

    @contextlib.contextmanager
    def with_frame(self, frame: FrameType) -> Generator[FrameType, None, None]:
        self.frame_push(frame)
        try:
            yield frame
        finally:
            self.frame_pop()


_C_ToPython = Func.get("mlc.printer.ToPython")


def to_python(obj: Any, cfg: Optional[PrinterConfig] = None) -> str:
    if cfg is None:
        cfg = PrinterConfig()
    return _C_ToPython(obj, cfg)


def print_python(
    obj: Any,
    cfg: Optional[PrinterConfig] = None,
    style: Optional[str] = None,
) -> None:
    cprint(to_python(obj, cfg=cfg), style=style)
