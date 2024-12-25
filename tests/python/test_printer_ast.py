import itertools
from typing import TYPE_CHECKING, Optional

import mlc.printer as mlcp
import pytest

if TYPE_CHECKING:
    import _pytest


@pytest.mark.parametrize(
    "doc,expected",
    [
        (mlcp.ast.Literal(None), "None"),
        (mlcp.ast.Literal(True), "1"),
        (mlcp.ast.Literal(False), "0"),
        (mlcp.ast.Literal("test"), '"test"'),
        (mlcp.ast.Literal(""), '""'),
        (mlcp.ast.Literal('""'), r'"\"\""'),
        (mlcp.ast.Literal("\n\t\\test\r"), r'"\n\t\\test\r"'),
        (mlcp.ast.Literal(0), "0"),
        (mlcp.ast.Literal(-1), "-1"),
        (mlcp.ast.Literal(3.25), "3.25"),
        (mlcp.ast.Literal(-0.5), "-0.5"),
    ],
    ids=itertools.count(),
)
def test_print_literal(doc: mlcp.ast.Node, expected: str) -> None:
    assert doc.to_python() == expected


@pytest.mark.parametrize(
    "name",
    [
        "test",
        "_test",
        "TestCase",
        "test_case",
        "test123",
    ],
    ids=itertools.count(),
)
def test_print_id(name: str) -> None:
    doc = mlcp.ast.Id(name)
    assert doc.to_python() == name


@pytest.mark.parametrize(
    "attr",
    [
        "attr",
        "_attr",
        "Attr",
        "attr_1",
    ],
    ids=itertools.count(),
)
def test_print_attr(attr: str) -> None:
    doc = mlcp.ast.Id("x").attr(attr)
    assert doc.to_python() == f"x.{attr}"


@pytest.mark.parametrize(
    "indices, expected",
    [
        (
            (),
            "[()]",
        ),
        (
            (mlcp.ast.Literal(1),),
            "[1]",
        ),
        (
            (mlcp.ast.Literal(2), mlcp.ast.Id("x")),
            "[2, x]",
        ),
        (
            (mlcp.ast.Slice(mlcp.ast.Literal(1), mlcp.ast.Literal(2)),),
            "[1:2]",
        ),
        (
            (mlcp.ast.Slice(mlcp.ast.Literal(1)), mlcp.ast.Id("y")),
            "[1:, y]",
        ),
        (
            (mlcp.ast.Slice(), mlcp.ast.Id("y")),
            "[:, y]",
        ),
        (
            (mlcp.ast.Id("x"), mlcp.ast.Id("y"), mlcp.ast.Id("z")),
            "[x, y, z]",
        ),
    ],
    ids=itertools.count(),
)
def test_print_index(indices: tuple[mlcp.ast.Node, ...], expected: str) -> None:
    doc = mlcp.ast.Id("x")[indices]
    assert doc.to_python() == f"x{expected}"


UNARY_OP_TOKENS = {
    mlcp.ast.OperationKind.USub: "-",
    mlcp.ast.OperationKind.Invert: "~",
    mlcp.ast.OperationKind.Not: "not ",
}


@pytest.mark.parametrize(
    "op_kind, expected_token",
    list(UNARY_OP_TOKENS.items()),
    ids=UNARY_OP_TOKENS.keys(),
)
def test_print_unary_operation(op_kind: int, expected_token: str) -> None:
    doc = mlcp.ast.Operation(op_kind, [mlcp.ast.Id("x")])
    assert doc.to_python() == f"{expected_token}x"


BINARY_OP_TOKENS = {
    mlcp.ast.OperationKind.Add: "+",
    mlcp.ast.OperationKind.Sub: "-",
    mlcp.ast.OperationKind.Mult: "*",
    mlcp.ast.OperationKind.Div: "/",
    mlcp.ast.OperationKind.FloorDiv: "//",
    mlcp.ast.OperationKind.Mod: "%",
    mlcp.ast.OperationKind.Pow: "**",
    mlcp.ast.OperationKind.LShift: "<<",
    mlcp.ast.OperationKind.RShift: ">>",
    mlcp.ast.OperationKind.BitAnd: "&",
    mlcp.ast.OperationKind.BitOr: "|",
    mlcp.ast.OperationKind.BitXor: "^",
    mlcp.ast.OperationKind.Lt: "<",
    mlcp.ast.OperationKind.LtE: "<=",
    mlcp.ast.OperationKind.Eq: "==",
    mlcp.ast.OperationKind.NotEq: "!=",
    mlcp.ast.OperationKind.Gt: ">",
    mlcp.ast.OperationKind.GtE: ">=",
    mlcp.ast.OperationKind.And: "and",
    mlcp.ast.OperationKind.Or: "or",
}


@pytest.mark.parametrize(
    "op_kind, expected_token",
    list(BINARY_OP_TOKENS.items()),
    ids=BINARY_OP_TOKENS.keys(),
)
def test_print_binary_operation(op_kind: int, expected_token: str) -> None:
    doc = mlcp.ast.Operation(op_kind, [mlcp.ast.Id("x"), mlcp.ast.Id("y")])
    assert doc.to_python() == f"x {expected_token} y"


SPECIAL_OP_CASES = [
    (
        mlcp.ast.OperationKind.IfThenElse,
        [mlcp.ast.Literal(True), mlcp.ast.Literal("true"), mlcp.ast.Literal("false")],
        '"true" if 1 else "false"',
    ),
    (
        mlcp.ast.OperationKind.IfThenElse,
        [mlcp.ast.Id("x"), mlcp.ast.Literal(None), mlcp.ast.Literal(1)],
        "None if x else 1",
    ),
]


@pytest.mark.parametrize(
    "op_kind, operands, expected",
    SPECIAL_OP_CASES,
    ids=[kind for (kind, *_) in SPECIAL_OP_CASES],
)
def test_print_special_operation(
    op_kind: int,
    operands: list[mlcp.ast.Node],
    expected: str,
) -> None:
    doc = mlcp.ast.Operation(op_kind, operands)
    assert doc.to_python() == expected


@pytest.mark.parametrize(
    "args, kwargs, expected",
    [
        (
            (),
            {},
            "()",
        ),
        (
            (),
            {"key0": mlcp.ast.Id("u")},
            "(key0=u)",
        ),
        (
            (),
            {"key0": mlcp.ast.Id("u"), "key1": mlcp.ast.Id("v")},
            "(key0=u, key1=v)",
        ),
        (
            (mlcp.ast.Id("x"),),
            {},
            "(x)",
        ),
        (
            (mlcp.ast.Id("x"),),
            {"key0": mlcp.ast.Id("u")},
            "(x, key0=u)",
        ),
        (
            (mlcp.ast.Id("x"),),
            {"key0": mlcp.ast.Id("u"), "key1": mlcp.ast.Id("v")},
            "(x, key0=u, key1=v)",
        ),
        (
            (mlcp.ast.Id("x"), (mlcp.ast.Id("y"))),
            {},
            "(x, y)",
        ),
        (
            (mlcp.ast.Id("x"), (mlcp.ast.Id("y"))),
            {"key0": mlcp.ast.Id("u")},
            "(x, y, key0=u)",
        ),
        (
            (mlcp.ast.Id("x"), (mlcp.ast.Id("y"))),
            {"key0": mlcp.ast.Id("u"), "key1": mlcp.ast.Id("v")},
            "(x, y, key0=u, key1=v)",
        ),
    ],
    ids=itertools.count(),
)
def test_print_call(
    args: tuple[mlcp.ast.Node, ...],
    kwargs: dict[str, mlcp.ast.Node],
    expected: str,
) -> None:
    kwargs_keys: list[str] = []
    kwargs_values: list[mlcp.ast.Node] = []
    for key, value in kwargs.items():
        kwargs_keys.append(key)
        kwargs_values.append(value)
    doc = mlcp.ast.Id("f").call_kw(
        args=args,
        kwargs_keys=kwargs_keys,
        kwargs_values=kwargs_values,
    )
    assert doc.to_python() == f"f{expected}"


@pytest.mark.parametrize(
    "args, expected",
    [
        (
            (),
            "lambda : 0",
        ),
        (
            (mlcp.ast.Id("x"),),
            "lambda x: 0",
        ),
        (
            (mlcp.ast.Id("x"), mlcp.ast.Id("y")),
            "lambda x, y: 0",
        ),
        (
            (mlcp.ast.Id("x"), mlcp.ast.Id("y"), mlcp.ast.Id("z")),
            "lambda x, y, z: 0",
        ),
    ],
    ids=itertools.count(),
)
def test_print_lambda(args: tuple[mlcp.ast.Id, ...], expected: str) -> None:
    doc = mlcp.ast.Lambda(
        args,
        body=mlcp.ast.Literal(0),
    )
    assert doc.to_python() == expected


@pytest.mark.parametrize(
    "elements, expected",
    [
        (
            (),
            "[]",
        ),
        (
            [mlcp.ast.Id("x")],
            "[x]",
        ),
        (
            [mlcp.ast.Id("x"), mlcp.ast.Id("y")],
            "[x, y]",
        ),
        (
            [mlcp.ast.Id("x"), mlcp.ast.Id("y"), mlcp.ast.Id("z")],
            "[x, y, z]",
        ),
    ],
    ids=itertools.count(),
)
def test_print_list(elements: list[mlcp.ast.Id], expected: str) -> None:
    doc = mlcp.ast.List(elements)
    assert doc.to_python() == expected


@pytest.mark.parametrize(
    "elements, expected",
    [
        (
            (),
            "()",
        ),
        (
            [mlcp.ast.Id("x")],
            "(x,)",
        ),
        (
            [mlcp.ast.Id("x"), mlcp.ast.Id("y")],
            "(x, y)",
        ),
        (
            [mlcp.ast.Id("x"), mlcp.ast.Id("y"), mlcp.ast.Id("z")],
            "(x, y, z)",
        ),
    ],
    ids=itertools.count(),
)
def test_print_tuple(elements: list[mlcp.ast.Id], expected: str) -> None:
    doc = mlcp.ast.Tuple(elements)
    assert doc.to_python() == expected


@pytest.mark.parametrize(
    "content, expected",
    [
        (
            {},
            "{}",
        ),
        (
            {mlcp.ast.Literal("key_x"): mlcp.ast.Id("x")},
            '{"key_x": x}',
        ),
        (
            {
                mlcp.ast.Literal("key_x"): mlcp.ast.Id("x"),
                mlcp.ast.Literal("key_y"): mlcp.ast.Id("y"),
            },
            '{"key_x": x, "key_y": y}',
        ),
        (
            {
                mlcp.ast.Literal("key_x"): mlcp.ast.Id("x"),
                mlcp.ast.Literal("key_y"): mlcp.ast.Id("y"),
                mlcp.ast.Literal("key_z"): mlcp.ast.Id("z"),
            },
            '{"key_x": x, "key_y": y, "key_z": z}',
        ),
    ],
    ids=itertools.count(),
)
def test_print_dict(content: dict[mlcp.ast.Expr, mlcp.ast.Expr], expected: str) -> None:
    keys = []
    values = []
    for key, value in content.items():
        keys.append(key)
        values.append(value)
    doc = mlcp.ast.Dict(keys, values)
    assert doc.to_python() == expected


@pytest.mark.parametrize(
    "slice_doc, expected",
    [
        (
            mlcp.ast.Slice(),
            ":",
        ),
        (
            mlcp.ast.Slice(mlcp.ast.Literal(1)),
            "1:",
        ),
        (
            mlcp.ast.Slice(None, mlcp.ast.Literal(2)),
            ":2",
        ),
        (
            mlcp.ast.Slice(mlcp.ast.Literal(1), mlcp.ast.Literal(2)),
            "1:2",
        ),
        (
            mlcp.ast.Slice(None, None, mlcp.ast.Literal(3)),
            "::3",
        ),
        (
            mlcp.ast.Slice(mlcp.ast.Literal(1), None, mlcp.ast.Literal(3)),
            "1::3",
        ),
        (
            mlcp.ast.Slice(None, mlcp.ast.Literal(2), mlcp.ast.Literal(3)),
            ":2:3",
        ),
        (
            mlcp.ast.Slice(mlcp.ast.Literal(1), mlcp.ast.Literal(2), mlcp.ast.Literal(3)),
            "1:2:3",
        ),
    ],
    ids=itertools.count(),
)
def test_print_slice(slice_doc: mlcp.ast.Slice, expected: str) -> None:
    doc = mlcp.ast.Id("x")[slice_doc]
    assert doc.to_python() == f"x[{expected}]"


@pytest.mark.parametrize(
    "stmts, expected",
    [
        (
            [],
            "",
        ),
        (
            [mlcp.ast.ExprStmt(mlcp.ast.Id("x"))],
            "x",
        ),
        (
            [mlcp.ast.ExprStmt(mlcp.ast.Id("x")), mlcp.ast.ExprStmt(mlcp.ast.Id("y"))],
            """
x
y""",
        ),
    ],
    ids=itertools.count(),
)
def test_print_stmt_block_doc(stmts: list[mlcp.ast.Stmt], expected: str) -> None:
    doc = mlcp.ast.StmtBlock(stmts)
    assert doc.to_python() == expected.strip()


@pytest.mark.parametrize(
    "doc, expected",
    [
        (
            mlcp.ast.Assign(mlcp.ast.Id("x"), mlcp.ast.Id("y"), None),
            "x = y",
        ),
        (
            mlcp.ast.Assign(mlcp.ast.Id("x"), mlcp.ast.Id("y"), mlcp.ast.Id("int")),
            "x: int = y",
        ),
        (
            mlcp.ast.Assign(mlcp.ast.Id("x"), None, mlcp.ast.Id("int")),
            "x: int",
        ),
        (
            mlcp.ast.Assign(
                mlcp.ast.Tuple([mlcp.ast.Id("x"), mlcp.ast.Id("y")]), mlcp.ast.Id("z"), None
            ),
            "x, y = z",
        ),
        (
            mlcp.ast.Assign(
                mlcp.ast.Tuple(
                    [mlcp.ast.Id("x"), mlcp.ast.Tuple([mlcp.ast.Id("y"), mlcp.ast.Id("z")])]
                ),
                mlcp.ast.Id("z"),
                None,
            ),
            "x, (y, z) = z",
        ),
    ],
    ids=itertools.count(),
)
def test_print_assign_doc(doc: mlcp.ast.Assign, expected: str) -> None:
    assert doc.to_python() == expected


@pytest.mark.parametrize(
    "then_branch, else_branch, expected",
    [
        (
            [mlcp.ast.ExprStmt(mlcp.ast.Id("x"))],
            [],
            """
if pred:
    x""",
        ),
        (
            [],
            [mlcp.ast.ExprStmt(mlcp.ast.Id("y"))],
            """
if pred:
    pass
else:
    y""",
        ),
        (
            [mlcp.ast.ExprStmt(mlcp.ast.Id("x"))],
            [mlcp.ast.ExprStmt(mlcp.ast.Id("y"))],
            """
if pred:
    x
else:
    y""",
        ),
    ],
    ids=itertools.count(),
)
def test_print_if_doc(
    then_branch: list[mlcp.ast.Stmt], else_branch: list[mlcp.ast.Stmt], expected: str
) -> None:
    doc = mlcp.ast.If(mlcp.ast.Id("pred"), then_branch, else_branch)
    assert doc.to_python(mlcp.PrinterConfig(indent_spaces=4)) == expected.strip()


@pytest.mark.parametrize(
    "body, expected",
    [
        (
            [mlcp.ast.ExprStmt(mlcp.ast.Id("x"))],
            """
while pred:
    x
            """,
        ),
        (
            [],
            """
while pred:
    pass
""",
        ),
    ],
    ids=itertools.count(),
)
def test_print_while_doc(body: list[mlcp.ast.Stmt], expected: str) -> None:
    doc = mlcp.ast.While(mlcp.ast.Id("pred"), body)
    assert doc.to_python(mlcp.PrinterConfig(indent_spaces=4)) == expected.strip()


@pytest.mark.parametrize(
    "body, expected",
    [
        (
            [mlcp.ast.ExprStmt(mlcp.ast.Id("x"))],
            """
for x in y:
    x
""",
        ),
        (
            [],
            """
for x in y:
    pass
""",
        ),
    ],
    ids=itertools.count(),
)
def test_print_for_doc(body: list[mlcp.ast.Stmt], expected: str) -> None:
    doc = mlcp.ast.For(mlcp.ast.Id("x"), mlcp.ast.Id("y"), body)
    assert doc.to_python(mlcp.PrinterConfig(indent_spaces=4)) == expected.strip()


@pytest.mark.parametrize(
    "lhs, body, expected",
    [
        (
            mlcp.ast.Id("c"),
            [mlcp.ast.ExprStmt(mlcp.ast.Id("x"))],
            """
with context() as c:
    x
""",
        ),
        (
            mlcp.ast.Id("c"),
            [],
            """
with context() as c:
    pass
""",
        ),
        (
            None,
            [],
            """
with context():
    pass
""",
        ),
        (
            None,
            [mlcp.ast.ExprStmt(mlcp.ast.Id("x"))],
            """
with context():
    x
""",
        ),
    ],
    ids=itertools.count(),
)
def test_print_with_scope(lhs: mlcp.ast.Id, body: list[mlcp.ast.Stmt], expected: str) -> None:
    doc = mlcp.ast.With(
        lhs,
        mlcp.ast.Id("context").call(),
        body,
    )
    assert doc.to_python(mlcp.PrinterConfig(indent_spaces=4)) == expected.strip()


def test_print_expr_stmt_doc() -> None:
    doc = mlcp.ast.ExprStmt(mlcp.ast.Id("f").call(mlcp.ast.Id("x")))
    assert doc.to_python() == "f(x)"


@pytest.mark.parametrize(
    "msg, expected",
    [
        (
            None,
            """
            assert 1
            """,
        ),
        (
            mlcp.ast.Literal("test message"),
            """
            assert 1, "test message"
            """,
        ),
    ],
    ids=itertools.count(),
)
def test_print_assert_doc(msg: Optional[mlcp.ast.Expr], expected: str) -> None:
    test = mlcp.ast.Literal(True)
    doc = mlcp.ast.Assert(test, msg)
    assert doc.to_python().strip() == expected.strip()


@pytest.mark.parametrize(
    "value, expected",
    [(mlcp.ast.Literal(None), "return None"), (mlcp.ast.Id("x"), "return x")],
    ids=itertools.count(),
)
def test_print_return_doc(value: mlcp.ast.Expr, expected: str) -> None:
    doc = mlcp.ast.Return(value)
    assert doc.to_python() == expected.strip()


@pytest.mark.parametrize(
    "args, decorators, return_type, body, expected",
    [
        (
            [],
            [],
            None,
            [],
            """
def func():
    pass
""",
        ),
        (
            [mlcp.ast.Assign(mlcp.ast.Id("x"), rhs=None, annotation=mlcp.ast.Id("int"))],
            [],
            mlcp.ast.Id("int"),
            [],
            """
def func(x: int) -> int:
    pass
""",
        ),
        (
            [
                mlcp.ast.Assign(
                    mlcp.ast.Id("x"), rhs=mlcp.ast.Literal(1), annotation=mlcp.ast.Id("int")
                )
            ],
            [],
            mlcp.ast.Literal(None),
            [],
            """
def func(x: int = 1) -> None:
    pass
""",
        ),
        (
            [],
            [mlcp.ast.Id("wrap")],
            mlcp.ast.Literal(None),
            [],
            """
@wrap
def func() -> None:
    pass
""",
        ),
        (
            [],
            [mlcp.ast.Id("wrap_outter"), mlcp.ast.Id("wrap_inner")],
            mlcp.ast.Literal(None),
            [],
            """
@wrap_outter
@wrap_inner
def func() -> None:
    pass
""",
        ),
        (
            [
                mlcp.ast.Assign(mlcp.ast.Id("x"), rhs=None, annotation=mlcp.ast.Id("int")),
                mlcp.ast.Assign(
                    mlcp.ast.Id("y"), rhs=mlcp.ast.Literal(1), annotation=mlcp.ast.Id("int")
                ),
            ],
            [mlcp.ast.Id("wrap")],
            mlcp.ast.Literal(None),
            [],
            """
@wrap
def func(x: int, y: int = 1) -> None:
    pass
""",
        ),
        (
            [
                mlcp.ast.Assign(mlcp.ast.Id("x"), rhs=None, annotation=mlcp.ast.Id("int")),
                mlcp.ast.Assign(
                    mlcp.ast.Id("y"), rhs=mlcp.ast.Literal(1), annotation=mlcp.ast.Id("int")
                ),
            ],
            [mlcp.ast.Id("wrap")],
            mlcp.ast.Literal(None),
            [
                mlcp.ast.Assign(
                    mlcp.ast.Id("y"),
                    mlcp.ast.Operation(
                        mlcp.ast.OperationKind.Add, [mlcp.ast.Id("x"), mlcp.ast.Literal(1)]
                    ),
                ),
                mlcp.ast.Assign(
                    mlcp.ast.Id("y"),
                    mlcp.ast.Operation(
                        mlcp.ast.OperationKind.Sub, [mlcp.ast.Id("y"), mlcp.ast.Literal(1)]
                    ),
                ),
            ],
            """
@wrap
def func(x: int, y: int = 1) -> None:
    y = x + 1
    y = y - 1
""",
        ),
    ],
    ids=itertools.count(),
)
def test_print_function_doc(
    args: list[mlcp.ast.Assign],
    decorators: list[mlcp.ast.Id],
    body: list[mlcp.ast.Stmt],
    return_type: Optional[mlcp.ast.Expr],
    expected: str,
) -> None:
    doc = mlcp.ast.Function(mlcp.ast.Id("func"), args, decorators, return_type, body)
    assert doc.to_python(mlcp.PrinterConfig(indent_spaces=4)) == expected.strip()


def get_func_doc_for_class(name: str) -> mlcp.ast.Function:
    args = [
        mlcp.ast.Assign(mlcp.ast.Id("x"), rhs=None, annotation=mlcp.ast.Id("int")),
        mlcp.ast.Assign(mlcp.ast.Id("y"), rhs=mlcp.ast.Literal(1), annotation=mlcp.ast.Id("int")),
    ]
    body = [
        mlcp.ast.Assign(
            mlcp.ast.Id("y"),
            mlcp.ast.Operation(mlcp.ast.OperationKind.Add, [mlcp.ast.Id("x"), mlcp.ast.Literal(1)]),
        ),
        mlcp.ast.Assign(
            mlcp.ast.Id("y"),
            mlcp.ast.Operation(mlcp.ast.OperationKind.Sub, [mlcp.ast.Id("y"), mlcp.ast.Literal(1)]),
        ),
    ]
    return mlcp.ast.Function(
        name=mlcp.ast.Id(name),
        args=args,
        decorators=[mlcp.ast.Id("wrap")],
        return_type=mlcp.ast.Literal(None),
        body=body,
    )


@pytest.mark.parametrize(
    "decorators, body, expected",
    [
        (
            [],
            [],
            """
class TestClass:
    pass
""",
        ),
        (
            [mlcp.ast.Id("wrap")],
            [],
            """
@wrap
class TestClass:
    pass
""",
        ),
        (
            [mlcp.ast.Id("wrap_outter"), mlcp.ast.Id("wrap_inner")],
            [],
            """
@wrap_outter
@wrap_inner
class TestClass:
    pass
""",
        ),
        (
            [mlcp.ast.Id("wrap")],
            [get_func_doc_for_class("f1")],
            """
@wrap
class TestClass:
    @wrap
    def f1(x: int, y: int = 1) -> None:
        y = x + 1
        y = y - 1
""",
        ),
        (
            [mlcp.ast.Id("wrap")],
            [get_func_doc_for_class("f1"), get_func_doc_for_class("f2")],
            """
@wrap
class TestClass:
    @wrap
    def f1(x: int, y: int = 1) -> None:
        y = x + 1
        y = y - 1

    @wrap
    def f2(x: int, y: int = 1) -> None:
        y = x + 1
        y = y - 1""",
        ),
    ],
    ids=itertools.count(),
)
def test_print_class_doc(
    decorators: list[mlcp.ast.Id],
    body: list[mlcp.ast.Function],
    expected: str,
) -> None:
    doc = mlcp.ast.Class(mlcp.ast.Id("TestClass"), decorators, body)
    assert doc.to_python(mlcp.PrinterConfig(indent_spaces=4)) == expected.strip()


@pytest.mark.parametrize(
    "comment, expected",
    [
        ("", "#"),
        ("test comment 1", "# test comment 1"),
        (
            "test comment 1\ntest comment 2",
            """
# test comment 1
# test comment 2
""",
        ),
    ],
    ids=itertools.count(),
)
def test_print_comment_doc(comment: str, expected: str) -> None:
    doc = mlcp.ast.Comment(comment)
    assert doc.to_python().strip() == expected.strip()


@pytest.mark.parametrize(
    "comment, expected",
    [
        (
            "",
            "",
        ),
        (
            "test comment 1",
            '''
"""
test comment 1
"""
            ''',
        ),
        (
            "test comment 1\ntest comment 2",
            '''
"""
test comment 1
test comment 2
"""
            ''',
        ),
    ],
    ids=itertools.count(),
)
def test_print_doc_string_doc(comment: str, expected: str) -> None:
    doc = mlcp.ast.DocString(comment)
    assert doc.to_python().strip() == expected.strip()


@pytest.mark.parametrize(
    "doc, comment, expected",
    [
        (
            mlcp.ast.Assign(mlcp.ast.Id("x"), mlcp.ast.Id("y"), mlcp.ast.Id("int")),
            "comment",
            """
x: int = y  # comment
""",
        ),
        (
            mlcp.ast.If(
                mlcp.ast.Id("x"),
                [mlcp.ast.ExprStmt(mlcp.ast.Id("y"))],
                [mlcp.ast.ExprStmt(mlcp.ast.Id("z"))],
            ),
            "comment",
            """
# comment
if x:
    y
else:
    z
""",
        ),
        (
            mlcp.ast.If(
                mlcp.ast.Id("x"),
                [mlcp.ast.ExprStmt(mlcp.ast.Id("y"))],
                [mlcp.ast.ExprStmt(mlcp.ast.Id("z"))],
            ),
            "comment line 1\ncomment line 2",
            """
# comment line 1
# comment line 2
if x:
    y
else:
    z
""",
        ),
        (
            mlcp.ast.While(
                mlcp.ast.Literal(True),
                [
                    mlcp.ast.Assign(mlcp.ast.Id("x"), mlcp.ast.Id("y")),
                ],
            ),
            "comment",
            """
# comment
while 1:
    x = y
""",
        ),
        (
            mlcp.ast.For(mlcp.ast.Id("x"), mlcp.ast.Id("y"), []),
            "comment",
            """
# comment
for x in y:
    pass
""",
        ),
        (
            mlcp.ast.With(mlcp.ast.Id("x"), mlcp.ast.Id("y"), []),
            "comment",
            """
# comment
with y as x:
    pass
""",
        ),
        (
            mlcp.ast.ExprStmt(mlcp.ast.Id("x")),
            "comment",
            """
x  # comment
            """,
        ),
        (
            mlcp.ast.Assert(mlcp.ast.Literal(True)),
            "comment",
            """
assert 1  # comment
            """,
        ),
        (
            mlcp.ast.Return(mlcp.ast.Literal(1)),
            "comment",
            """
return 1  # comment
            """,
        ),
        (
            get_func_doc_for_class("f"),
            "comment",
            '''
@wrap
def f(x: int, y: int = 1) -> None:
    """
    comment
    """
    y = x + 1
    y = y - 1
''',
        ),
        (
            get_func_doc_for_class("f"),
            "comment line 1\n\ncomment line 3",
            '''
@wrap
def f(x: int, y: int = 1) -> None:
    """
    comment line 1

    comment line 3
    """
    y = x + 1
    y = y - 1
''',
        ),
        (
            mlcp.ast.Class(mlcp.ast.Id("TestClass"), decorators=[mlcp.ast.Id("wrap")], body=[]),
            "comment",
            '''
@wrap
class TestClass:
    """
    comment
    """
    pass
''',
        ),
        (
            mlcp.ast.Class(mlcp.ast.Id("TestClass"), decorators=[mlcp.ast.Id("wrap")], body=[]),
            "comment line 1\n\ncomment line 3",
            '''
@wrap
class TestClass:
    """
    comment line 1

    comment line 3
    """
    pass
''',
        ),
    ],
    ids=itertools.count(),
)
def test_print_doc_comment(
    doc: mlcp.ast.Stmt,
    comment: str,
    expected: str,
) -> None:
    doc.comment = comment
    assert doc.to_python(mlcp.PrinterConfig(indent_spaces=4)) == expected.strip()


@pytest.mark.parametrize(
    "doc",
    [
        mlcp.ast.Assign(mlcp.ast.Id("x"), mlcp.ast.Id("y"), mlcp.ast.Id("int")),
        mlcp.ast.ExprStmt(mlcp.ast.Id("x")),
        mlcp.ast.Assert(mlcp.ast.Id("x")),
        mlcp.ast.Return(mlcp.ast.Id("x")),
    ],
)
def test_print_invalid_multiline_doc_comment(doc: mlcp.ast.Stmt) -> None:
    doc.comment = "1\n2"
    with pytest.raises(ValueError) as e:
        doc.to_python()
    assert "cannot have newline" in str(e.value)


def generate_expr_precedence_test_cases() -> list["_pytest.mark.ParameterSet"]:
    x = mlcp.ast.Id("x")
    y = mlcp.ast.Id("y")
    z = mlcp.ast.Id("z")

    def negative(a: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.USub, [a])

    def invert(a: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.Invert, [a])

    def not_(a: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.Not, [a])

    def add(a: mlcp.ast.Expr, b: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.Add, [a, b])

    def sub(a: mlcp.ast.Expr, b: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.Sub, [a, b])

    def mult(a: mlcp.ast.Expr, b: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.Mult, [a, b])

    def div(a: mlcp.ast.Expr, b: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.Div, [a, b])

    def mod(a: mlcp.ast.Expr, b: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.Mod, [a, b])

    def pow(a: mlcp.ast.Expr, b: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.Pow, [a, b])

    def lshift(a: mlcp.ast.Expr, b: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.LShift, [a, b])

    def bit_and(a: mlcp.ast.Expr, b: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.BitAnd, [a, b])

    def bit_or(a: mlcp.ast.Expr, b: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.BitOr, [a, b])

    def bit_xor(a: mlcp.ast.Expr, b: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.BitXor, [a, b])

    def lt(a: mlcp.ast.Expr, b: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.Lt, [a, b])

    def eq(a: mlcp.ast.Expr, b: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.Eq, [a, b])

    def not_eq(a: mlcp.ast.Expr, b: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.NotEq, [a, b])

    def and_(a: mlcp.ast.Expr, b: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.And, [a, b])

    def or_(a: mlcp.ast.Expr, b: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.Or, [a, b])

    def if_then_else(a: mlcp.ast.Expr, b: mlcp.ast.Expr, c: mlcp.ast.Expr) -> mlcp.ast.Expr:
        return mlcp.ast.Operation(mlcp.ast.OperationKind.IfThenElse, [a, b, c])

    test_cases = {
        "attr-call-index": [
            (
                add(x, y).attr("test"),
                "(x + y).test",
            ),
            (
                add(x, y.attr("test")),
                "x + y.test",
            ),
            (
                x[z].call(y),
                "x[z](y)",
            ),
            (
                x.call(y)[z],
                "x(y)[z]",
            ),
            (
                x.call(y).call(z),
                "x(y)(z)",
            ),
            (
                x.call(y).attr("test"),
                "x(y).test",
            ),
            (
                x.attr("test").call(y),
                "x.test(y)",
            ),
            (
                x.attr("test").attr("test2"),
                "x.test.test2",
            ),
            (
                mlcp.ast.Lambda([x], x).call(y),
                "(lambda x: x)(y)",
            ),
            (
                add(x, y)[z][add(z, z)].attr("name"),
                "(x + y)[z][z + z].name",
            ),
        ],
        "power": [
            (
                pow(pow(x, y), z),
                "(x ** y) ** z",
            ),
            (
                pow(x, pow(y, z)),
                "x ** y ** z",
            ),
            (
                pow(negative(x), negative(y)),
                "(-x) ** -y",
            ),
            (
                pow(add(x, y), add(y, z)),
                "(x + y) ** (y + z)",
            ),
        ],
        "unary": [
            (
                invert(negative(y)),
                "~-y",
            ),
            (
                negative(y).attr("test"),
                "(-y).test",
            ),
            (
                negative(y.attr("test")),
                "-y.test",
            ),
            (
                mult(negative(x), negative(y)),
                "-x * -y",
            ),
            (
                negative(add(invert(x), negative(y))),
                "-(~x + -y)",
            ),
        ],
        "add-mult": [
            (
                mult(x, mult(y, z)),
                "x * (y * z)",
            ),
            (
                mult(mult(x, y), z),
                "x * y * z",
            ),
            (
                mult(x, add(y, z)),
                "x * (y + z)",
            ),
            (
                mult(add(y, z), x),
                "(y + z) * x",
            ),
            (
                add(x, mod(y, z)),
                "x + y % z",
            ),
            (
                add(mult(y, z), x),
                "y * z + x",
            ),
            (
                add(add(x, y), add(y, z)),
                "x + y + (y + z)",
            ),
            (
                div(add(x, y), add(y, z)),
                "(x + y) / (y + z)",
            ),
        ],
        "shift": [
            (
                div(x, lshift(y, z)),
                "x / (y << z)",
            ),
            (
                mult(lshift(y, z), x),
                "(y << z) * x",
            ),
            (
                lshift(x, mult(y, z)),
                "x << y * z",
            ),
            (
                lshift(mult(x, y), z),
                "x * y << z",
            ),
            (
                lshift(mult(x, y), z),
                "x * y << z",
            ),
            (
                lshift(lshift(x, y), z),
                "x << y << z",
            ),
            (
                lshift(x, lshift(y, z)),
                "x << (y << z)",
            ),
        ],
        "bitwise": [
            (
                add(bit_or(x, y), bit_or(y, z)),
                "(x | y) + (y | z)",
            ),
            (
                bit_and(bit_or(x, y), bit_or(y, z)),
                "(x | y) & (y | z)",
            ),
            (
                bit_or(bit_and(x, y), bit_and(y, z)),
                "x & y | y & z",
            ),
            (
                bit_and(bit_xor(x, bit_or(y, z)), z),
                "(x ^ (y | z)) & z",
            ),
        ],
        "comparison": [
            (
                not_eq(add(x, y), z),
                "x + y != z",
            ),
            (
                eq(pow(x, y), z),
                "x ** y == z",
            ),
            (
                lt(x, div(y, z)),
                "x < y / z",
            ),
            (
                lt(x, if_then_else(y, y, y)),
                "x < (y if y else y)",
            ),
        ],
        "boolean": [
            (
                not_(and_(x, y)),
                "not (x and y)",
            ),
            (
                and_(not_(x), y),
                "not x and y",
            ),
            (
                and_(or_(x, y), z),
                "(x or y) and z",
            ),
            (
                or_(x, or_(y, z)),
                "x or (y or z)",
            ),
            (
                or_(or_(x, y), z),
                "x or y or z",
            ),
            (
                or_(and_(x, y), z),
                # Maybe we should consider adding parentheses here
                # for readability, even though it's not necessary.
                "x and y or z",
            ),
            (
                and_(or_(not_(x), y), z),
                "(not x or y) and z",
            ),
            (
                and_(lt(x, y), lt(y, z)),
                "x < y and y < z",
            ),
            (
                or_(not_(eq(x, y)), lt(y, z)),
                # Same as the previous one, the code here is not
                # readable without parentheses.
                "not x == y or y < z",
            ),
            (
                and_(if_then_else(x, y, z), x),
                "(y if x else z) and x",
            ),
            (
                not_(if_then_else(x, y, z)),
                "not (y if x else z)",
            ),
        ],
        "if-then-else": [
            (
                if_then_else(x, if_then_else(y, y, y), z),
                "y if y else y if x else z",
            ),
            (
                if_then_else(if_then_else(x, x, x), y, z),
                "y if (x if x else x) else z",
            ),
            (
                if_then_else(x, y, if_then_else(z, z, z)),
                "y if x else (z if z else z)",
            ),
            (
                if_then_else(lt(x, x), add(y, y), mult(z, z)),
                "y + y if x < x else z * z",
            ),
            (
                if_then_else(
                    mlcp.ast.Lambda([x], x), mlcp.ast.Lambda([y], y), mlcp.ast.Lambda([z], z)
                ),
                "(lambda y: y) if (lambda x: x) else (lambda z: z)",
            ),
        ],
        "lambda": [
            (
                mlcp.ast.Lambda([x, y], add(z, z)),
                "lambda x, y: z + z",
            ),
            (
                add(mlcp.ast.Lambda([x, y], z), z),
                "(lambda x, y: z) + z",
            ),
            (
                mlcp.ast.Lambda([x, y], add(z, z)).call(x, y),
                "(lambda x, y: z + z)(x, y)",
            ),
            (
                mlcp.ast.Lambda([x], mlcp.ast.Lambda([y], z)),
                "lambda x: lambda y: z",
            ),
        ],
    }

    return [
        pytest.param(*args, id=f"{group_name}-{i}")
        for group_name, cases in test_cases.items()
        for i, args in enumerate(cases)
    ]


@pytest.mark.parametrize("doc, expected", generate_expr_precedence_test_cases())
def test_expr_precedence(doc: mlcp.ast.Expr, expected: str) -> None:
    assert doc.to_python() == expected
