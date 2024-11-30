# Derived from
# https://github.com/python/typeshed/blob/main/stdlib/ast.pyi

from typing import Any, Optional

from mlc.dataclasses import PyClass, py_class

_Identifier = str


@py_class(type_key="mlc.ast.AST")
class AST(PyClass): ...


@py_class(type_key="mlc.ast.mod")
class mod(AST): ...


@py_class(type_key="mlc.ast.expr_context")
class expr_context(AST): ...


@py_class(type_key="mlc.ast.operator")
class operator(AST): ...


@py_class(type_key="mlc.ast.cmpop")
class cmpop(AST): ...


@py_class(type_key="mlc.ast.unaryop")
class unaryop(AST): ...


@py_class(type_key="mlc.ast.boolop")
class boolop(AST): ...


@py_class(type_key="mlc.ast.type_ignore")
class type_ignore(AST): ...


@py_class(type_key="mlc.ast.stmt")
class stmt(AST):
    lineno: Optional[int]
    col_offset: Optional[int]
    end_lineno: Optional[int]
    end_col_offset: Optional[int]


@py_class(type_key="mlc.ast.expr")
class expr(AST):
    lineno: Optional[int]
    col_offset: Optional[int]
    end_lineno: Optional[int]
    end_col_offset: Optional[int]


@py_class(type_key="mlc.ast.type_param")
class type_param(AST):
    lineno: Optional[int]
    col_offset: Optional[int]
    end_lineno: Optional[int]
    end_col_offset: Optional[int]


@py_class(type_key="mlc.ast.pattern")
class pattern(AST):
    lineno: Optional[int]
    col_offset: Optional[int]
    end_lineno: Optional[int]
    end_col_offset: Optional[int]


@py_class(type_key="mlc.ast.arg")
class arg(AST):
    lineno: Optional[int]
    col_offset: Optional[int]
    end_lineno: Optional[int]
    end_col_offset: Optional[int]
    arg: _Identifier
    annotation: Optional[expr]
    type_comment: Optional[str]


@py_class(type_key="mlc.ast.keyword")
class keyword(AST):
    lineno: Optional[int]
    col_offset: Optional[int]
    end_lineno: Optional[int]
    end_col_offset: Optional[int]
    arg: Optional[_Identifier]
    value: expr


@py_class(type_key="mlc.ast.alias")
class alias(AST):
    lineno: Optional[int]
    col_offset: Optional[int]
    end_lineno: Optional[int]
    end_col_offset: Optional[int]
    name: str
    asname: Optional[_Identifier]


@py_class(type_key="mlc.ast.arguments")
class arguments(AST):
    posonlyargs: list[arg]
    args: list[arg]
    vararg: Optional[arg]
    kwonlyargs: list[arg]
    kw_defaults: list[Optional[expr]]
    kwarg: Optional[arg]
    defaults: list[expr]


@py_class(type_key="mlc.ast.comprehension")
class comprehension(AST):
    target: expr
    iter: expr
    ifs: list[expr]
    is_async: int


@py_class(type_key="mlc.ast.withitem")
class withitem(AST):
    context_expr: expr
    optional_vars: Optional[expr]


@py_class(type_key="mlc.ast.TypeIgnore")
class TypeIgnore(type_ignore):
    lineno: Optional[int]
    tag: str


@py_class(type_key="mlc.ast.Module")
class Module(mod):
    body: list[stmt]
    type_ignores: list[TypeIgnore]


@py_class(type_key="mlc.ast.Interactive")
class Interactive(mod):
    body: list[stmt]


@py_class(type_key="mlc.ast.Expression")
class Expression(mod):
    body: expr


@py_class(type_key="mlc.ast.FunctionType")
class FunctionType(mod):
    argtypes: list[expr]
    returns: expr


@py_class(type_key="mlc.ast.FunctionDef")
class FunctionDef(stmt):
    name: _Identifier
    args: arguments
    body: list[stmt]
    decorator_list: list[expr]
    returns: Optional[expr]
    type_comment: Optional[str]
    type_params: Optional[list[type_param]]


@py_class(type_key="mlc.ast.AsyncFunctionDef")
class AsyncFunctionDef(stmt):
    name: _Identifier
    args: arguments
    body: list[stmt]
    decorator_list: list[expr]
    returns: Optional[expr]
    type_comment: Optional[str]
    type_params: Optional[list[type_param]]


@py_class(type_key="mlc.ast.ClassDef")
class ClassDef(stmt):
    name: _Identifier
    bases: list[expr]
    keywords: list[keyword]
    body: list[stmt]
    decorator_list: list[expr]
    type_params: Optional[list[type_param]]


@py_class(type_key="mlc.ast.Return")
class Return(stmt):
    value: Optional[expr]


@py_class(type_key="mlc.ast.Delete")
class Delete(stmt):
    targets: list[expr]


@py_class(type_key="mlc.ast.Assign")
class Assign(stmt):
    targets: list[expr]
    value: expr
    type_comment: Optional[str]


@py_class(type_key="mlc.ast.Attribute")
class Attribute(expr):
    value: expr
    attr: _Identifier
    ctx: expr_context


@py_class(type_key="mlc.ast.Subscript")
class Subscript(expr):
    value: expr
    slice: expr
    ctx: expr_context


@py_class(type_key="mlc.ast.AugAssign")
class AugAssign(stmt):
    target: Any  # Name | Attribute | Subscript
    op: operator
    value: expr


@py_class(type_key="mlc.ast.AnnAssign")
class AnnAssign(stmt):
    target: Any  # Name | Attribute | Subscript
    annotation: expr
    value: Optional[expr]
    simple: int


@py_class(type_key="mlc.ast.For")
class For(stmt):
    target: expr
    iter: expr
    body: list[stmt]
    orelse: list[stmt]
    type_comment: Optional[str]


@py_class(type_key="mlc.ast.AsyncFor")
class AsyncFor(stmt):
    target: expr
    iter: expr
    body: list[stmt]
    orelse: list[stmt]
    type_comment: Optional[str]


@py_class(type_key="mlc.ast.While")
class While(stmt):
    test: expr
    body: list[stmt]
    orelse: list[stmt]


@py_class(type_key="mlc.ast.If")
class If(stmt):
    test: expr
    body: list[stmt]
    orelse: list[stmt]


@py_class(type_key="mlc.ast.With")
class With(stmt):
    items: list[withitem]
    body: list[stmt]
    type_comment: Optional[str]


@py_class(type_key="mlc.ast.AsyncWith")
class AsyncWith(stmt):
    items: list[withitem]
    body: list[stmt]
    type_comment: Optional[str]


@py_class(type_key="mlc.ast.match_case")
class match_case(AST):
    pattern: pattern
    guard: Optional[expr]
    body: list[stmt]


@py_class(type_key="mlc.ast.Match")
class Match(stmt):
    subject: expr
    cases: list[match_case]


@py_class(type_key="mlc.ast.Raise")
class Raise(stmt):
    exc: Optional[expr]
    cause: Optional[expr]


@py_class(type_key="mlc.ast.ExceptHandler")
class ExceptHandler(AST):
    lineno: Optional[int]
    col_offset: Optional[int]
    end_lineno: Optional[int]
    end_col_offset: Optional[int]
    type: Optional[expr]
    name: Optional[_Identifier]
    body: list[stmt]


@py_class(type_key="mlc.ast.Try")
class Try(stmt):
    body: list[stmt]
    handlers: list[ExceptHandler]
    orelse: list[stmt]
    finalbody: list[stmt]


@py_class(type_key="mlc.ast.TryStar")
class TryStar(stmt):
    body: list[stmt]
    handlers: list[ExceptHandler]
    orelse: list[stmt]
    finalbody: list[stmt]


@py_class(type_key="mlc.ast.Assert")
class Assert(stmt):
    test: expr
    msg: Optional[expr]


@py_class(type_key="mlc.ast.Import")
class Import(stmt):
    names: list[alias]


@py_class(type_key="mlc.ast.ImportFrom")
class ImportFrom(stmt):
    module: Optional[str]
    names: list[alias]
    level: int


@py_class(type_key="mlc.ast.Global")
class Global(stmt):
    names: list[_Identifier]


@py_class(type_key="mlc.ast.Nonlocal")
class Nonlocal(stmt):
    names: list[_Identifier]


@py_class(type_key="mlc.ast.Expr")
class Expr(stmt):
    value: expr


@py_class(type_key="mlc.ast.Pass")
class Pass(stmt): ...


@py_class(type_key="mlc.ast.Break")
class Break(stmt): ...


@py_class(type_key="mlc.ast.Continue")
class Continue(stmt): ...


@py_class(type_key="mlc.ast.BoolOp")
class BoolOp(expr):
    op: boolop
    values: list[expr]


@py_class(type_key="mlc.ast.Name")
class Name(expr):
    id: _Identifier
    ctx: expr_context


@py_class(type_key="mlc.ast.NamedExpr")
class NamedExpr(expr):
    target: Name
    value: expr


@py_class(type_key="mlc.ast.BinOp")
class BinOp(expr):
    left: expr
    op: operator
    right: expr


@py_class(type_key="mlc.ast.UnaryOp")
class UnaryOp(expr):
    op: unaryop
    operand: expr


@py_class(type_key="mlc.ast.Lambda")
class Lambda(expr):
    args: arguments
    body: expr


@py_class(type_key="mlc.ast.IfExp")
class IfExp(expr):
    test: expr
    body: expr
    orelse: expr


@py_class(type_key="mlc.ast.Dict")
class Dict(expr):
    keys: list[Optional[expr]]
    values: list[expr]


@py_class(type_key="mlc.ast.Set")
class Set(expr):
    elts: list[expr]


@py_class(type_key="mlc.ast.ListComp")
class ListComp(expr):
    elt: expr
    generators: list[comprehension]


@py_class(type_key="mlc.ast.SetComp")
class SetComp(expr):
    elt: expr
    generators: list[comprehension]


@py_class(type_key="mlc.ast.DictComp")
class DictComp(expr):
    key: expr
    value: expr
    generators: list[comprehension]


@py_class(type_key="mlc.ast.GeneratorExp")
class GeneratorExp(expr):
    elt: expr
    generators: list[comprehension]


@py_class(type_key="mlc.ast.Await")
class Await(expr):
    value: expr


@py_class(type_key="mlc.ast.Yield")
class Yield(expr):
    value: Optional[expr]


@py_class(type_key="mlc.ast.YieldFrom")
class YieldFrom(expr):
    value: expr


@py_class(type_key="mlc.ast.Compare")
class Compare(expr):
    left: expr
    ops: list[cmpop]
    comparators: list[expr]


@py_class(type_key="mlc.ast.Call")
class Call(expr):
    func: expr
    args: list[expr]
    keywords: list[keyword]


@py_class(type_key="mlc.ast.FormattedValue")
class FormattedValue(expr):
    value: expr
    conversion: int
    format_spec: Optional[expr]


@py_class(type_key="mlc.ast.JoinedStr")
class JoinedStr(expr):
    values: list[expr]


@py_class(type_key="mlc.ast.Ellipsis")
class Ellipsis(PyClass): ...


@py_class(type_key="mlc.ast.Constant")
class Constant(expr):
    value: Any  # None, str, bytes, bool, int, float, complex, Ellipsis
    kind: Optional[str]


@py_class(type_key="mlc.ast.Starred")
class Starred(expr):
    value: expr
    ctx: expr_context


@py_class(type_key="mlc.ast.List")
class List(expr):
    elts: list[expr]
    ctx: expr_context


@py_class(type_key="mlc.ast.Tuple")
class Tuple(expr):
    elts: list[expr]
    ctx: expr_context
    dims: list[expr]


@py_class(type_key="mlc.ast.Slice")
class Slice(expr):
    lower: Optional[expr]
    upper: Optional[expr]
    step: Optional[expr]


@py_class(type_key="mlc.ast.Load")
class Load(expr_context): ...


@py_class(type_key="mlc.ast.Store")
class Store(expr_context): ...


@py_class(type_key="mlc.ast.Del")
class Del(expr_context): ...


@py_class(type_key="mlc.ast.And")
class And(boolop): ...


@py_class(type_key="mlc.ast.Or")
class Or(boolop): ...


@py_class(type_key="mlc.ast.Add")
class Add(operator): ...


@py_class(type_key="mlc.ast.Sub")
class Sub(operator): ...


@py_class(type_key="mlc.ast.Mult")
class Mult(operator): ...


@py_class(type_key="mlc.ast.MatMult")
class MatMult(operator): ...


@py_class(type_key="mlc.ast.Div")
class Div(operator): ...


@py_class(type_key="mlc.ast.Mod")
class Mod(operator): ...


@py_class(type_key="mlc.ast.Pow")
class Pow(operator): ...


@py_class(type_key="mlc.ast.LShift")
class LShift(operator): ...


@py_class(type_key="mlc.ast.RShift")
class RShift(operator): ...


@py_class(type_key="mlc.ast.BitOr")
class BitOr(operator): ...


@py_class(type_key="mlc.ast.BitXor")
class BitXor(operator): ...


@py_class(type_key="mlc.ast.BitAnd")
class BitAnd(operator): ...


@py_class(type_key="mlc.ast.FloorDiv")
class FloorDiv(operator): ...


@py_class(type_key="mlc.ast.Invert")
class Invert(unaryop): ...


@py_class(type_key="mlc.ast.Not")
class Not(unaryop): ...


@py_class(type_key="mlc.ast.UAdd")
class UAdd(unaryop): ...


@py_class(type_key="mlc.ast.USub")
class USub(unaryop): ...


@py_class(type_key="mlc.ast.Eq")
class Eq(cmpop): ...


@py_class(type_key="mlc.ast.NotEq")
class NotEq(cmpop): ...


@py_class(type_key="mlc.ast.Lt")
class Lt(cmpop): ...


@py_class(type_key="mlc.ast.LtE")
class LtE(cmpop): ...


@py_class(type_key="mlc.ast.Gt")
class Gt(cmpop): ...


@py_class(type_key="mlc.ast.GtE")
class GtE(cmpop): ...


@py_class(type_key="mlc.ast.Is")
class Is(cmpop): ...


@py_class(type_key="mlc.ast.IsNot")
class IsNot(cmpop): ...


@py_class(type_key="mlc.ast.In")
class In(cmpop): ...


@py_class(type_key="mlc.ast.NotIn")
class NotIn(cmpop): ...


@py_class(type_key="mlc.ast.MatchValue")
class MatchValue(pattern):
    value: expr


@py_class(type_key="mlc.ast.MatchSingleton")
class MatchSingleton(pattern):
    value: int  # boolean


@py_class(type_key="mlc.ast.MatchSequence")
class MatchSequence(pattern):
    patterns: list[pattern]


@py_class(type_key="mlc.ast.MatchMapping")
class MatchMapping(pattern):
    keys: list[expr]
    patterns: list[pattern]
    rest: Optional[_Identifier]


@py_class(type_key="mlc.ast.MatchClass")
class MatchClass(pattern):
    cls: expr
    patterns: list[pattern]
    kwd_attrs: list[_Identifier]
    kwd_patterns: list[pattern]


@py_class(type_key="mlc.ast.MatchStar")
class MatchStar(pattern):
    name: Optional[_Identifier]


@py_class(type_key="mlc.ast.MatchAs")
class MatchAs(pattern):
    pattern: Optional[pattern]
    name: Optional[_Identifier]


@py_class(type_key="mlc.ast.MatchOr")
class MatchOr(pattern):
    patterns: list[pattern]


@py_class(type_key="mlc.ast.TypeVar")
class TypeVar(type_param):
    name: _Identifier
    bound: Optional[expr]
    default_value: Optional[expr]


@py_class(type_key="mlc.ast.ParamSpec")
class ParamSpec(type_param):
    name: _Identifier
    default_value: Optional[expr]


@py_class(type_key="mlc.ast.TypeVarTuple")
class TypeVarTuple(type_param):
    name: _Identifier
    default_value: Optional[expr]


@py_class(type_key="mlc.ast.TypeAlias")
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
