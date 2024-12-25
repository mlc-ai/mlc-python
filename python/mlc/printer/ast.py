from collections.abc import Sequence
from typing import Any, Optional

import mlc.dataclasses as mlcd
from mlc.core import Object, ObjectPath

from .cprint import cprint


@mlcd.c_class("mlc.printer.PrinterConfig")
class PrinterConfig(Object):
    indent_spaces: int = 2
    print_line_numbers: int = 0
    num_context_lines: int = -1
    path_to_underline: list[ObjectPath] = mlcd.field(default_factory=list)


@mlcd.c_class("mlc.printer.ast.Node")
class Node(mlcd.PyClass):
    source_paths: list[ObjectPath] = mlcd.field(default_factory=list)

    def to_python(self, config: Optional[PrinterConfig] = None) -> str:
        if config is None:
            config = PrinterConfig()
        return type(self)._C(b"to_python", self, config)

    def print_python(
        self,
        config: Optional[PrinterConfig] = None,
        style: Optional[str] = None,
    ) -> None:
        if config is None:
            config = PrinterConfig()
        cprint(self.to_python(config), style=style)

    def add_path(self, path: ObjectPath) -> "Node":
        self.source_paths.append(path)
        return self


@mlcd.c_class("mlc.printer.ast.Expr")
class Expr(Node):
    def __neg__(self) -> "Expr":
        return Operation(OperationKind.USub, [self])

    def __invert__(self) -> "Expr":
        return Operation(OperationKind.Invert, [self])

    def __not__(self) -> "Expr":
        return Operation(OperationKind.Not, [self])

    def __add__(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.Add, [self, other])

    def __sub__(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.Sub, [self, other])

    def __mul__(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.Mult, [self, other])

    def __truediv__(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.Div, [self, other])

    def __floordiv__(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.FloorDiv, [self, other])

    def __mod__(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.Mod, [self, other])

    def __pow__(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.Pow, [self, other])

    def __lshift__(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.LShift, [self, other])

    def __rshift__(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.RShift, [self, other])

    def __and__(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.BitAnd, [self, other])

    def __or__(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.BitOr, [self, other])

    def __xor__(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.BitXor, [self, other])

    def __lt__(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.Lt, [self, other])

    def __le__(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.LtE, [self, other])

    def __gt__(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.Gt, [self, other])

    def __ge__(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.GtE, [self, other])

    def logical_and(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.And, [self, other])

    def logical_or(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.Or, [self, other])

    def if_then_else(self, then: "Expr", else_: "Expr") -> "Expr":
        return Operation(OperationKind.IfThenElse, [self, then, else_])

    def eq(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.Eq, [self, other])

    def ne(self, other: "Expr") -> "Expr":
        return Operation(OperationKind.NotEq, [self, other])

    def attr(self, name: str) -> "Expr":
        return type(self)._C(b"attr", self, name)

    def index(self, indices: Sequence["Expr"]) -> "Expr":
        return type(self)._C(b"index", self, indices)

    def call(self, *args: "Expr") -> "Expr":
        return type(self)._C(b"call", self, args)

    def call_kw(
        self,
        args: Sequence["Expr"],
        kwargs_keys: Sequence[str],
        kwargs_values: Sequence["Expr"],
    ) -> "Expr":
        if not isinstance(args, Sequence):
            args = (args,)
        if not isinstance(kwargs_keys, Sequence):
            kwargs_keys = (kwargs_keys,)
        if not isinstance(kwargs_values, Sequence):
            kwargs_values = (kwargs_values,)
        return type(self)._C(b"call_kw", self, args, kwargs_keys, kwargs_values)

    def __getitem__(self, indices: "Expr | Sequence[Expr]") -> "Expr":
        if not isinstance(indices, Sequence):
            indices = (indices,)
        return self.index(indices)


@mlcd.c_class("mlc.printer.ast.Stmt")
class Stmt(Node):
    comment: Optional[str] = None


@mlcd.c_class("mlc.printer.ast.StmtBlock")
class StmtBlock(Stmt):
    stmts: list[Stmt]


@mlcd.c_class("mlc.printer.ast.Literal")
class Literal(Expr):
    value: Any  # int, str, float, None

    @staticmethod
    def Int(value: int) -> "Literal":
        return Literal(value=value)

    @staticmethod
    def Str(value: str) -> "Literal":
        return Literal(value=value)

    @staticmethod
    def Float(value: float) -> "Literal":
        return Literal(value=value)

    @staticmethod
    def Null() -> "Literal":
        return Literal(value=None)


@mlcd.c_class("mlc.printer.ast.Id")
class Id(Expr):
    name: str


@mlcd.c_class("mlc.printer.ast.Attr")
class Attr(Expr):
    obj: Expr
    name: str


@mlcd.c_class("mlc.printer.ast.Index")
class Index(Expr):
    obj: Expr
    idx: list[Expr]


@mlcd.c_class("mlc.printer.ast.Call")
class Call(Expr):
    callee: Expr
    args: list[Expr]
    kwargs_keys: list[str]
    kwargs_values: list[Expr]


class OperationKind:
    ### Unary operations
    _UnaryStart = 0
    USub = 1  # -x
    Invert = 2  # ~x
    Not = 3  # not x
    _UnaryEnd = 4
    ### Binary operations
    _BinaryStart = 5
    Add = 6  # +
    Sub = 7  # -
    Mult = 8  # *
    Div = 9  # /
    FloorDiv = 10  # // in Python
    Mod = 11  # % in Python
    Pow = 12  # ** in Python
    LShift = 13  # <<
    RShift = 14  # >>
    BitAnd = 15  # &
    BitOr = 16  # |
    BitXor = 17  # ^
    Lt = 18  # <
    LtE = 19  # <=
    Eq = 20  # ==
    NotEq = 21  # !=
    Gt = 22  # >
    GtE = 23  # >=
    And = 24  # and
    Or = 25  # or
    _BinaryEnd = 26
    # Special operations
    _SpecialStart = 27
    IfThenElse = 28  # <operands[1]> if <operands[0]> else <operands[2]>
    SpecialEnd = 29


@mlcd.c_class("mlc.printer.ast.Operation")
class Operation(Expr):
    op: int  # OperationKind
    operands: list[Expr]


@mlcd.c_class("mlc.printer.ast.Lambda")
class Lambda(Expr):
    args: list[Id]
    body: Expr


@mlcd.c_class("mlc.printer.ast.Tuple")
class Tuple(Expr):
    values: list[Expr]


@mlcd.c_class("mlc.printer.ast.List")
class List(Expr):
    values: list[Expr]


@mlcd.c_class("mlc.printer.ast.Dict")
class Dict(Expr):
    keys: list[Expr]
    values: list[Expr]


@mlcd.c_class("mlc.printer.ast.Slice", init=False)
class Slice(Expr):
    start: Optional[Expr]
    stop: Optional[Expr]
    step: Optional[Expr]

    def __init__(
        self,
        start: Optional[Expr] = None,
        stop: Optional[Expr] = None,
        step: Optional[Expr] = None,
    ) -> None:
        self._mlc_init((), start, stop, step)


@mlcd.c_class("mlc.printer.ast.Assign", init=False)
class Assign(Stmt):
    lhs: Expr
    rhs: Optional[Expr]
    annotation: Optional[Expr]

    def __init__(  # noqa: PLR0913, RUF100
        self,
        lhs: Expr,
        rhs: Optional[Expr] = None,
        annotation: Optional[Expr] = None,
        comment: Optional[str] = None,
        source_paths: Optional[list[ObjectPath]] = None,
    ) -> None:
        if source_paths is None:
            source_paths = []
        self._mlc_init(source_paths, comment, lhs, rhs, annotation)


@mlcd.c_class("mlc.printer.ast.If")
class If(Stmt):
    cond: Expr
    then_branch: list[Stmt]
    else_branch: list[Stmt]


@mlcd.c_class("mlc.printer.ast.While")
class While(Stmt):
    cond: Expr
    body: list[Stmt]


@mlcd.c_class("mlc.printer.ast.For")
class For(Stmt):
    lhs: Expr
    rhs: Expr
    body: list[Stmt]


@mlcd.c_class("mlc.printer.ast.With")
class With(Stmt):
    lhs: Optional[Expr]
    rhs: Expr
    body: list[Stmt]


@mlcd.c_class("mlc.printer.ast.ExprStmt")
class ExprStmt(Stmt):
    expr: Expr


@mlcd.c_class("mlc.printer.ast.Assert", init=False)
class Assert(Stmt):
    cond: Expr
    msg: Optional[Expr]

    def __init__(
        self,
        cond: Expr,
        msg: Optional[Expr] = None,
        comment: Optional[str] = None,
        source_paths: Optional[list[ObjectPath]] = None,
    ) -> None:
        if source_paths is None:
            source_paths = []
        self._mlc_init(source_paths, comment, cond, msg)


@mlcd.c_class("mlc.printer.ast.Return")
class Return(Stmt):
    value: Optional[Expr]


@mlcd.c_class("mlc.printer.ast.Function")
class Function(Stmt):
    name: Id
    args: list[Assign]
    decorators: list[Expr]
    return_type: Optional[Expr]
    body: list[Stmt]


@mlcd.c_class("mlc.printer.ast.Class")
class Class(Stmt):
    name: Id
    decorators: list[Expr]
    body: list[Stmt]


@mlcd.c_class("mlc.printer.ast.Comment", init=False)
class Comment(Stmt):
    def __init__(
        self,
        comment: Optional[str],
        source_paths: Optional[list[ObjectPath]] = None,
    ) -> None:
        if source_paths is None:
            source_paths = []
        self._mlc_init(source_paths, comment)


@mlcd.c_class("mlc.printer.ast.DocString", init=False)
class DocString(Stmt):
    def __init__(
        self,
        comment: Optional[str],
        source_paths: Optional[list[ObjectPath]] = None,
    ) -> None:
        if source_paths is None:
            source_paths = []
        self._mlc_init(source_paths, comment)
