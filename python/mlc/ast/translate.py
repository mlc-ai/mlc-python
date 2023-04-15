from __future__ import annotations

import ast
import typing

from mlc.core import List

from . import mlc_ast

TYPE_PY_TO_MLC_TRANSLATOR = typing.Callable[[ast.AST], mlc_ast.AST]
TYPE_MLC_TO_PY_TRANSLATOR = typing.Callable[[mlc_ast.AST], ast.AST]


def translate_py_to_mlc(py_node: ast.AST) -> mlc_ast.AST:
    assert isinstance(py_node, ast.AST), f"Expected AST node, got {type(py_node)}"
    if (translator := _PY_TO_MLC_VTABLE.get(type(py_node))) is not None:
        return translator(py_node)
    raise NotImplementedError(f"Translation not implemented: {type(py_node)}")


def translate_mlc_to_py(mlc_node: mlc_ast.AST) -> ast.AST:
    assert isinstance(mlc_node, mlc_ast.AST), f"Expected AST node, got {type(mlc_node)}"
    if (translator := _MLC_TO_PY_VTABLE.get(type(mlc_node))) is not None:
        return translator(mlc_node)
    raise NotImplementedError(f"Translation not implemented: {type(mlc_node)}")


def py_to_mlc_vtable_create() -> dict[type[ast.AST], TYPE_PY_TO_MLC_TRANSLATOR]:
    def _translate_field(value: typing.Any) -> typing.Any:
        if isinstance(value, ast.AST):
            return translate_py_to_mlc(value)
        elif value is ...:
            return mlc_ast.Ellipsis()
        elif isinstance(value, list):
            return [_translate_field(item) for item in value]
        return value

    def _create_entry(
        mlc_cls: type[mlc_ast.AST], py_cls: type[ast.AST]
    ) -> TYPE_PY_TO_MLC_TRANSLATOR:
        def py_to_mlc_default(py_node: ast.AST) -> mlc_ast.AST:
            return mlc_cls(
                **{
                    field: _translate_field(getattr(py_node, field, None))
                    for field in mlc_field_names
                }
            )

        mlc_field_names = list(typing.get_type_hints(mlc_cls).keys())
        return py_to_mlc_default

    def _create_entry_constant() -> TYPE_PY_TO_MLC_TRANSLATOR:
        # Special handling to convert `bytea` to `str`, because `bytes` is not supported in MLC
        entry = _create_entry(mlc_ast.Constant, ast.Constant)

        def py_to_mlc_constant(py_node: ast.Constant) -> mlc_ast.Constant:
            if isinstance(py_node.value, bytes):
                py_node.value = py_node.value.decode("utf-8")
            return entry(py_node)

        return typing.cast(TYPE_PY_TO_MLC_TRANSLATOR, py_to_mlc_constant)

    vtable: dict[type[ast.AST], TYPE_PY_TO_MLC_TRANSLATOR] = {
        type(...): lambda _: mlc_ast.Ellipsis(),  # type: ignore
        ast.Constant: _create_entry_constant(),
    }
    for cls_name in mlc_ast.__all__:
        mlc_cls = getattr(mlc_ast, cls_name)
        if py_cls := getattr(ast, cls_name, None):
            if py_cls not in vtable:
                vtable[py_cls] = _create_entry(mlc_cls, py_cls)
    return vtable


def mlc_to_py_vtable_create() -> dict[type[mlc_ast.AST], TYPE_MLC_TO_PY_TRANSLATOR]:
    def _translate_field(value: typing.Any) -> typing.Any:
        if isinstance(value, mlc_ast.AST):
            return translate_mlc_to_py(value)
        elif isinstance(value, (list, List)):
            return [_translate_field(item) for item in value]
        return value

    def _create_entry(
        py_cls: type[ast.AST], mlc_cls: type[mlc_ast.AST]
    ) -> TYPE_MLC_TO_PY_TRANSLATOR:
        def mlc_to_py_default(mlc_node: mlc_ast.AST) -> ast.AST:
            return py_cls(
                **{
                    field: _translate_field(getattr(mlc_node, field, None))
                    for field in py_field_names
                }
            )

        py_field_names = list(typing.get_type_hints(mlc_cls).keys())
        return mlc_to_py_default

    vtable: dict[type[mlc_ast.AST], TYPE_MLC_TO_PY_TRANSLATOR] = {
        mlc_ast.Ellipsis: lambda _: ...,  # type: ignore
    }
    for cls_name in mlc_ast.__all__:
        mlc_cls = getattr(mlc_ast, cls_name)
        if py_cls := getattr(ast, cls_name, None):
            if mlc_cls not in vtable:
                vtable[mlc_cls] = _create_entry(py_cls, mlc_cls)
    return vtable


_PY_TO_MLC_VTABLE = py_to_mlc_vtable_create()
_MLC_TO_PY_VTABLE = mlc_to_py_vtable_create()
