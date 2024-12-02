# Derived from
# https://github.com/python/typeshed/blob/main/stdlib/ast.pyi

from typing import Any, Optional

from mlc import dataclasses as mlcd

_Identifier = str


@mlcd.py_class(type_key="mlc.ast.AST", structure="bind")
class AST(mlcd.PyClass): ...


@mlcd.py_class(type_key="mlc.ast.mod", structure="bind")
class mod(AST): ...


@mlcd.py_class(type_key="mlc.ast.expr_context", structure="bind")
class expr_context(AST): ...


@mlcd.py_class(type_key="mlc.ast.operator", structure="bind")
class operator(AST): ...


@mlcd.py_class(type_key="mlc.ast.cmpop", structure="bind")
class cmpop(AST): ...


@mlcd.py_class(type_key="mlc.ast.unaryop", structure="bind")
class unaryop(AST): ...


@mlcd.py_class(type_key="mlc.ast.boolop", structure="bind")
class boolop(AST): ...


@mlcd.py_class(type_key="mlc.ast.type_ignore", structure="bind")
class type_ignore(AST): ...


@mlcd.py_class(type_key="mlc.ast.stmt", structure="bind")
class stmt(AST):
    lineno: Optional[int]
    col_offset: Optional[int]
    end_lineno: Optional[int]
    end_col_offset: Optional[int]


@mlcd.py_class(type_key="mlc.ast.expr", structure="bind")
class expr(AST):
    lineno: Optional[int]
    col_offset: Optional[int]
    end_lineno: Optional[int]
    end_col_offset: Optional[int]


@mlcd.py_class(type_key="mlc.ast.type_param", structure="bind")
class type_param(AST):
    lineno: Optional[int]
    col_offset: Optional[int]
    end_lineno: Optional[int]
    end_col_offset: Optional[int]


@mlcd.py_class(type_key="mlc.ast.pattern", structure="bind")
class pattern(AST):
    lineno: Optional[int]
    col_offset: Optional[int]
    end_lineno: Optional[int]
    end_col_offset: Optional[int]


@mlcd.py_class(type_key="mlc.ast.arg", structure="bind")
class arg(AST):
    lineno: Optional[int]
    col_offset: Optional[int]
    end_lineno: Optional[int]
    end_col_offset: Optional[int]
    arg: _Identifier
    annotation: Optional[expr]
    type_comment: Optional[str]


@mlcd.py_class(type_key="mlc.ast.keyword", structure="bind")
class keyword(AST):
    lineno: Optional[int]
    col_offset: Optional[int]
    end_lineno: Optional[int]
    end_col_offset: Optional[int]
    arg: Optional[_Identifier]
    value: expr


@mlcd.py_class(type_key="mlc.ast.alias", structure="bind")
class alias(AST):
    lineno: Optional[int]
    col_offset: Optional[int]
    end_lineno: Optional[int]
    end_col_offset: Optional[int]
    name: str
    asname: Optional[_Identifier]


@mlcd.py_class(type_key="mlc.ast.arguments", structure="bind")
class arguments(AST):
    posonlyargs: list[arg]
    args: list[arg]
    vararg: Optional[arg]
    kwonlyargs: list[arg]
    kw_defaults: list[Optional[expr]]
    kwarg: Optional[arg]
    defaults: list[expr]


@mlcd.py_class(type_key="mlc.ast.comprehension", structure="bind")
class comprehension(AST):
    target: expr
    iter: expr
    ifs: list[expr]
    is_async: int


@mlcd.py_class(type_key="mlc.ast.withitem", structure="bind")
class withitem(AST):
    context_expr: expr
    optional_vars: Optional[expr]


@mlcd.py_class(type_key="mlc.ast.TypeIgnore", structure="bind")
class TypeIgnore(type_ignore):
    lineno: Optional[int]
    tag: str


@mlcd.py_class(type_key="mlc.ast.Module", structure="bind")
class Module(mod):
    body: list[stmt]
    type_ignores: list[TypeIgnore]


@mlcd.py_class(type_key="mlc.ast.Interactive", structure="bind")
class Interactive(mod):
    body: list[stmt]


@mlcd.py_class(type_key="mlc.ast.Expression", structure="bind")
class Expression(mod):
    body: expr


@mlcd.py_class(type_key="mlc.ast.FunctionType", structure="bind")
class FunctionType(mod):
    argtypes: list[expr]
    returns: expr


@mlcd.py_class(type_key="mlc.ast.FunctionDef", structure="bind")
class FunctionDef(stmt):
    name: _Identifier
    args: arguments
    body: list[stmt]
    decorator_list: list[expr]
    returns: Optional[expr]
    type_comment: Optional[str]
    type_params: Optional[list[type_param]]


@mlcd.py_class(type_key="mlc.ast.AsyncFunctionDef", structure="bind")
class AsyncFunctionDef(stmt):
    name: _Identifier
    args: arguments
    body: list[stmt]
    decorator_list: list[expr]
    returns: Optional[expr]
    type_comment: Optional[str]
    type_params: Optional[list[type_param]]


@mlcd.py_class(type_key="mlc.ast.ClassDef", structure="bind")
class ClassDef(stmt):
    name: _Identifier
    bases: list[expr]
    keywords: list[keyword]
    body: list[stmt]
    decorator_list: list[expr]
    type_params: Optional[list[type_param]]


@mlcd.py_class(type_key="mlc.ast.Return", structure="bind")
class Return(stmt):
    value: Optional[expr]


@mlcd.py_class(type_key="mlc.ast.Delete", structure="bind")
class Delete(stmt):
    targets: list[expr]


@mlcd.py_class(type_key="mlc.ast.Assign", structure="bind")
class Assign(stmt):
    targets: list[expr]
    value: expr
    type_comment: Optional[str]


@mlcd.py_class(type_key="mlc.ast.Attribute", structure="bind")
class Attribute(expr):
    value: expr
    attr: _Identifier
    ctx: expr_context


@mlcd.py_class(type_key="mlc.ast.Subscript", structure="bind")
class Subscript(expr):
    value: expr
    slice: expr
    ctx: expr_context


@mlcd.py_class(type_key="mlc.ast.AugAssign", structure="bind")
class AugAssign(stmt):
    target: Any  # Name | Attribute | Subscript
    op: operator
    value: expr


@mlcd.py_class(type_key="mlc.ast.AnnAssign", structure="bind")
class AnnAssign(stmt):
    target: Any  # Name | Attribute | Subscript
    annotation: expr
    value: Optional[expr]
    simple: int


@mlcd.py_class(type_key="mlc.ast.For", structure="bind")
class For(stmt):
    target: expr
    iter: expr
    body: list[stmt]
    orelse: list[stmt]
    type_comment: Optional[str]


@mlcd.py_class(type_key="mlc.ast.AsyncFor", structure="bind")
class AsyncFor(stmt):
    target: expr
    iter: expr
    body: list[stmt]
    orelse: list[stmt]
    type_comment: Optional[str]


@mlcd.py_class(type_key="mlc.ast.While", structure="bind")
class While(stmt):
    test: expr
    body: list[stmt]
    orelse: list[stmt]


@mlcd.py_class(type_key="mlc.ast.If", structure="bind")
class If(stmt):
    test: expr
    body: list[stmt]
    orelse: list[stmt]


@mlcd.py_class(type_key="mlc.ast.With", structure="bind")
class With(stmt):
    items: list[withitem]
    body: list[stmt]
    type_comment: Optional[str]


@mlcd.py_class(type_key="mlc.ast.AsyncWith", structure="bind")
class AsyncWith(stmt):
    items: list[withitem]
    body: list[stmt]
    type_comment: Optional[str]


@mlcd.py_class(type_key="mlc.ast.match_case", structure="bind")
class match_case(AST):
    pattern: pattern
    guard: Optional[expr]
    body: list[stmt]


@mlcd.py_class(type_key="mlc.ast.Match", structure="bind")
class Match(stmt):
    subject: expr
    cases: list[match_case]


@mlcd.py_class(type_key="mlc.ast.Raise", structure="bind")
class Raise(stmt):
    exc: Optional[expr]
    cause: Optional[expr]


@mlcd.py_class(type_key="mlc.ast.ExceptHandler", structure="bind")
class ExceptHandler(AST):
    lineno: Optional[int]
    col_offset: Optional[int]
    end_lineno: Optional[int]
    end_col_offset: Optional[int]
    type: Optional[expr]
    name: Optional[_Identifier]
    body: list[stmt]


@mlcd.py_class(type_key="mlc.ast.Try", structure="bind")
class Try(stmt):
    body: list[stmt]
    handlers: list[ExceptHandler]
    orelse: list[stmt]
    finalbody: list[stmt]


@mlcd.py_class(type_key="mlc.ast.TryStar", structure="bind")
class TryStar(stmt):
    body: list[stmt]
    handlers: list[ExceptHandler]
    orelse: list[stmt]
    finalbody: list[stmt]


@mlcd.py_class(type_key="mlc.ast.Assert", structure="bind")
class Assert(stmt):
    test: expr
    msg: Optional[expr]


@mlcd.py_class(type_key="mlc.ast.Import", structure="bind")
class Import(stmt):
    names: list[alias]


@mlcd.py_class(type_key="mlc.ast.ImportFrom", structure="bind")
class ImportFrom(stmt):
    module: Optional[str]
    names: list[alias]
    level: int


@mlcd.py_class(type_key="mlc.ast.Global", structure="bind")
class Global(stmt):
    names: list[_Identifier]


@mlcd.py_class(type_key="mlc.ast.Nonlocal", structure="bind")
class Nonlocal(stmt):
    names: list[_Identifier]


@mlcd.py_class(type_key="mlc.ast.Expr", structure="bind")
class Expr(stmt):
    value: expr


@mlcd.py_class(type_key="mlc.ast.Pass", structure="bind")
class Pass(stmt): ...


@mlcd.py_class(type_key="mlc.ast.Break", structure="bind")
class Break(stmt): ...


@mlcd.py_class(type_key="mlc.ast.Continue", structure="bind")
class Continue(stmt): ...


@mlcd.py_class(type_key="mlc.ast.BoolOp", structure="bind")
class BoolOp(expr):
    op: boolop
    values: list[expr]


@mlcd.py_class(type_key="mlc.ast.Name", structure="bind")
class Name(expr):
    id: _Identifier
    ctx: expr_context


@mlcd.py_class(type_key="mlc.ast.NamedExpr", structure="bind")
class NamedExpr(expr):
    target: Name
    value: expr


@mlcd.py_class(type_key="mlc.ast.BinOp", structure="bind")
class BinOp(expr):
    left: expr
    op: operator
    right: expr


@mlcd.py_class(type_key="mlc.ast.UnaryOp", structure="bind")
class UnaryOp(expr):
    op: unaryop
    operand: expr


@mlcd.py_class(type_key="mlc.ast.Lambda", structure="bind")
class Lambda(expr):
    args: arguments
    body: expr


@mlcd.py_class(type_key="mlc.ast.IfExp", structure="bind")
class IfExp(expr):
    test: expr
    body: expr
    orelse: expr


@mlcd.py_class(type_key="mlc.ast.Dict", structure="bind")
class Dict(expr):
    keys: list[Optional[expr]]
    values: list[expr]


@mlcd.py_class(type_key="mlc.ast.Set", structure="bind")
class Set(expr):
    elts: list[expr]


@mlcd.py_class(type_key="mlc.ast.ListComp", structure="bind")
class ListComp(expr):
    elt: expr
    generators: list[comprehension]


@mlcd.py_class(type_key="mlc.ast.SetComp", structure="bind")
class SetComp(expr):
    elt: expr
    generators: list[comprehension]


@mlcd.py_class(type_key="mlc.ast.DictComp", structure="bind")
class DictComp(expr):
    key: expr
    value: expr
    generators: list[comprehension]


@mlcd.py_class(type_key="mlc.ast.GeneratorExp", structure="bind")
class GeneratorExp(expr):
    elt: expr
    generators: list[comprehension]


@mlcd.py_class(type_key="mlc.ast.Await", structure="bind")
class Await(expr):
    value: expr


@mlcd.py_class(type_key="mlc.ast.Yield", structure="bind")
class Yield(expr):
    value: Optional[expr]


@mlcd.py_class(type_key="mlc.ast.YieldFrom", structure="bind")
class YieldFrom(expr):
    value: expr


@mlcd.py_class(type_key="mlc.ast.Compare", structure="bind")
class Compare(expr):
    left: expr
    ops: list[cmpop]
    comparators: list[expr]


@mlcd.py_class(type_key="mlc.ast.Call", structure="bind")
class Call(expr):
    func: expr
    args: list[expr]
    keywords: list[keyword]


@mlcd.py_class(type_key="mlc.ast.FormattedValue", structure="bind")
class FormattedValue(expr):
    value: expr
    conversion: int
    format_spec: Optional[expr]


@mlcd.py_class(type_key="mlc.ast.JoinedStr", structure="bind")
class JoinedStr(expr):
    values: list[expr]


@mlcd.py_class(type_key="mlc.ast.Ellipsis", structure="bind")
class Ellipsis(mlcd.PyClass): ...


@mlcd.py_class(type_key="mlc.ast.Constant", structure="bind")
class Constant(expr):
    value: Any  # None, str, bytes, bool, int, float, complex, Ellipsis
    kind: Optional[str]


@mlcd.py_class(type_key="mlc.ast.Starred", structure="bind")
class Starred(expr):
    value: expr
    ctx: expr_context


@mlcd.py_class(type_key="mlc.ast.List", structure="bind")
class List(expr):
    elts: list[expr]
    ctx: expr_context


@mlcd.py_class(type_key="mlc.ast.Tuple", structure="bind")
class Tuple(expr):
    elts: list[expr]
    ctx: expr_context
    dims: list[expr]


@mlcd.py_class(type_key="mlc.ast.Slice", structure="bind")
class Slice(expr):
    lower: Optional[expr]
    upper: Optional[expr]
    step: Optional[expr]


@mlcd.py_class(type_key="mlc.ast.Load", structure="bind")
class Load(expr_context): ...


@mlcd.py_class(type_key="mlc.ast.Store", structure="bind")
class Store(expr_context): ...


@mlcd.py_class(type_key="mlc.ast.Del", structure="bind")
class Del(expr_context): ...


@mlcd.py_class(type_key="mlc.ast.And", structure="bind")
class And(boolop): ...


@mlcd.py_class(type_key="mlc.ast.Or", structure="bind")
class Or(boolop): ...


@mlcd.py_class(type_key="mlc.ast.Add", structure="bind")
class Add(operator): ...


@mlcd.py_class(type_key="mlc.ast.Sub", structure="bind")
class Sub(operator): ...


@mlcd.py_class(type_key="mlc.ast.Mult", structure="bind")
class Mult(operator): ...


@mlcd.py_class(type_key="mlc.ast.MatMult", structure="bind")
class MatMult(operator): ...


@mlcd.py_class(type_key="mlc.ast.Div", structure="bind")
class Div(operator): ...


@mlcd.py_class(type_key="mlc.ast.Mod", structure="bind")
class Mod(operator): ...


@mlcd.py_class(type_key="mlc.ast.Pow", structure="bind")
class Pow(operator): ...


@mlcd.py_class(type_key="mlc.ast.LShift", structure="bind")
class LShift(operator): ...


@mlcd.py_class(type_key="mlc.ast.RShift", structure="bind")
class RShift(operator): ...


@mlcd.py_class(type_key="mlc.ast.BitOr", structure="bind")
class BitOr(operator): ...


@mlcd.py_class(type_key="mlc.ast.BitXor", structure="bind")
class BitXor(operator): ...


@mlcd.py_class(type_key="mlc.ast.BitAnd", structure="bind")
class BitAnd(operator): ...


@mlcd.py_class(type_key="mlc.ast.FloorDiv", structure="bind")
class FloorDiv(operator): ...


@mlcd.py_class(type_key="mlc.ast.Invert", structure="bind")
class Invert(unaryop): ...


@mlcd.py_class(type_key="mlc.ast.Not", structure="bind")
class Not(unaryop): ...


@mlcd.py_class(type_key="mlc.ast.UAdd", structure="bind")
class UAdd(unaryop): ...


@mlcd.py_class(type_key="mlc.ast.USub", structure="bind")
class USub(unaryop): ...


@mlcd.py_class(type_key="mlc.ast.Eq", structure="bind")
class Eq(cmpop): ...


@mlcd.py_class(type_key="mlc.ast.NotEq", structure="bind")
class NotEq(cmpop): ...


@mlcd.py_class(type_key="mlc.ast.Lt", structure="bind")
class Lt(cmpop): ...


@mlcd.py_class(type_key="mlc.ast.LtE", structure="bind")
class LtE(cmpop): ...


@mlcd.py_class(type_key="mlc.ast.Gt", structure="bind")
class Gt(cmpop): ...


@mlcd.py_class(type_key="mlc.ast.GtE", structure="bind")
class GtE(cmpop): ...


@mlcd.py_class(type_key="mlc.ast.Is", structure="bind")
class Is(cmpop): ...


@mlcd.py_class(type_key="mlc.ast.IsNot", structure="bind")
class IsNot(cmpop): ...


@mlcd.py_class(type_key="mlc.ast.In", structure="bind")
class In(cmpop): ...


@mlcd.py_class(type_key="mlc.ast.NotIn", structure="bind")
class NotIn(cmpop): ...


@mlcd.py_class(type_key="mlc.ast.MatchValue", structure="bind")
class MatchValue(pattern):
    value: expr


@mlcd.py_class(type_key="mlc.ast.MatchSingleton", structure="bind")
class MatchSingleton(pattern):
    value: int  # boolean


@mlcd.py_class(type_key="mlc.ast.MatchSequence", structure="bind")
class MatchSequence(pattern):
    patterns: list[pattern]


@mlcd.py_class(type_key="mlc.ast.MatchMapping", structure="bind")
class MatchMapping(pattern):
    keys: list[expr]
    patterns: list[pattern]
    rest: Optional[_Identifier]


@mlcd.py_class(type_key="mlc.ast.MatchClass", structure="bind")
class MatchClass(pattern):
    cls: expr
    patterns: list[pattern]
    kwd_attrs: list[_Identifier]
    kwd_patterns: list[pattern]


@mlcd.py_class(type_key="mlc.ast.MatchStar", structure="bind")
class MatchStar(pattern):
    name: Optional[_Identifier]


@mlcd.py_class(type_key="mlc.ast.MatchAs", structure="bind")
class MatchAs(pattern):
    pattern: Optional[pattern]
    name: Optional[_Identifier]


@mlcd.py_class(type_key="mlc.ast.MatchOr", structure="bind")
class MatchOr(pattern):
    patterns: list[pattern]


@mlcd.py_class(type_key="mlc.ast.TypeVar", structure="bind")
class TypeVar(type_param):
    name: _Identifier
    bound: Optional[expr]
    default_value: Optional[expr]


@mlcd.py_class(type_key="mlc.ast.ParamSpec", structure="bind")
class ParamSpec(type_param):
    name: _Identifier
    default_value: Optional[expr]


@mlcd.py_class(type_key="mlc.ast.TypeVarTuple", structure="bind")
class TypeVarTuple(type_param):
    name: _Identifier
    default_value: Optional[expr]


@mlcd.py_class(type_key="mlc.ast.TypeAlias", structure="bind")
class TypeAlias(stmt):
    name: Name
    type_params: Optional[list[type_param]]
    value: expr


__all__ = [
    "AST",
    "mod",
    "expr_context",
    "operator",
    "cmpop",
    "unaryop",
    "boolop",
    "type_ignore",
    "stmt",
    "expr",
    "type_param",
    "pattern",
    "arg",
    "keyword",
    "alias",
    "arguments",
    "comprehension",
    "withitem",
    "TypeIgnore",
    "Module",
    "Interactive",
    "Expression",
    "FunctionType",
    "FunctionDef",
    "AsyncFunctionDef",
    "ClassDef",
    "Return",
    "Delete",
    "Assign",
    "Attribute",
    "Subscript",
    "AugAssign",
    "AnnAssign",
    "For",
    "AsyncFor",
    "While",
    "If",
    "With",
    "AsyncWith",
    "match_case",
    "Match",
    "Raise",
    "ExceptHandler",
    "Try",
    "TryStar",
    "Assert",
    "Import",
    "ImportFrom",
    "Global",
    "Nonlocal",
    "Expr",
    "Pass",
    "Break",
    "Continue",
    "BoolOp",
    "Name",
    "NamedExpr",
    "BinOp",
    "UnaryOp",
    "Lambda",
    "IfExp",
    "Dict",
    "Set",
    "ListComp",
    "SetComp",
    "DictComp",
    "GeneratorExp",
    "Await",
    "Yield",
    "YieldFrom",
    "Compare",
    "Call",
    "FormattedValue",
    "JoinedStr",
    "Constant",
    "Starred",
    "List",
    "Tuple",
    "Slice",
    "Load",
    "Store",
    "Del",
    "And",
    "Or",
    "Add",
    "Sub",
    "Mult",
    "MatMult",
    "Div",
    "Mod",
    "Pow",
    "LShift",
    "RShift",
    "BitOr",
    "BitXor",
    "BitAnd",
    "FloorDiv",
    "Invert",
    "Not",
    "UAdd",
    "USub",
    "Eq",
    "NotEq",
    "Lt",
    "LtE",
    "Gt",
    "GtE",
    "Is",
    "IsNot",
    "In",
    "NotIn",
    "MatchValue",
    "MatchSingleton",
    "MatchSequence",
    "MatchMapping",
    "MatchClass",
    "MatchStar",
    "MatchAs",
    "MatchOr",
    "TypeVar",
    "ParamSpec",
    "TypeVarTuple",
    "TypeAlias",
]
