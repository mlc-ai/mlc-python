from __future__ import annotations

import ast
import inspect
import linecache
import sys
from typing import Any


class Source:
    source_name: str
    start_line: int
    start_column: int
    source: str
    full_source: str

    def __init__(
        self,
        program: str | ast.AST,
    ) -> None:
        if isinstance(program, str):
            self.source_name = "<str>"
            self.start_line = 1
            self.start_column = 0
            self.source = program
            self.full_source = program
            return

        self.source_name = inspect.getsourcefile(program)  # type: ignore
        lines, self.start_line = getsourcelines(program)  # type: ignore
        if lines:
            self.start_column = len(lines[0]) - len(lines[0].lstrip())
        else:
            self.start_column = 0
        if self.start_column and lines:
            self.source = "\n".join([l[self.start_column :].rstrip() for l in lines])
        else:
            self.source = "".join(lines)
        try:
            # It will cause a problem when running in Jupyter Notebook.
            # `mod` will be <module '__main__'>, which is a built-in module
            # and `getsource` will throw a TypeError
            mod = inspect.getmodule(program)
            if mod:
                self.full_source = inspect.getsource(mod)
            else:
                self.full_source = self.source
        except TypeError:
            # It's a work around for Jupyter problem.
            # Since `findsource` is an internal API of inspect, we just use it
            # as a fallback method.
            src, _ = inspect.findsource(program)  # type: ignore
            self.full_source = "".join(src)


def getfile(obj: Any) -> str:
    if not inspect.isclass(obj):
        return _getfile(obj)
    mod = getattr(obj, "__module__", None)
    if mod is not None:
        file = getattr(sys.modules[mod], "__file__", None)
        if file is not None:
            return file
    for _, member in inspect.getmembers(obj):
        if inspect.isfunction(member):
            if obj.__qualname__ + "." + member.__name__ == member.__qualname__:
                return inspect.getfile(member)
    raise TypeError(f"Source for {obj:!r} not found")


def getsourcelines(obj: Any) -> tuple[list[str], int]:
    obj = inspect.unwrap(obj)
    lines, l_num = findsource(obj)
    return inspect.getblock(lines[l_num:]), l_num + 1


def findsource(obj: Any) -> tuple[list[str], int]:  # noqa: PLR0912
    if not inspect.isclass(obj):
        return _findsource(obj)

    file = inspect.getsourcefile(obj)
    if file:
        linecache.checkcache(file)
    else:
        file = inspect.getfile(obj)
        if not (file.startswith("<") and file.endswith(">")):
            raise OSError("source code not available")

    module = inspect.getmodule(obj, file)
    if module:
        lines = linecache.getlines(file, module.__dict__)
    else:
        lines = linecache.getlines(file)
    if not lines:
        raise OSError("could not get source code")
    qual_names = obj.__qualname__.replace(".<locals>", "<locals>").split(".")
    in_comment = 0
    scope_stack: list[str] = []
    indent_info: dict[str, int] = {}
    for i, line in enumerate(lines):
        n_comment = line.count('"""')
        if n_comment:
            # update multi-line comments status
            in_comment = in_comment ^ (n_comment & 1)
            continue
        if in_comment:
            # skip lines within multi-line comments
            continue
        indent = len(line) - len(line.lstrip())
        tokens = line.split()
        if len(tokens) > 1:
            name = None
            if tokens[0] == "def":
                name = tokens[1].split(":")[0].split("(")[0] + "<locals>"
            elif tokens[0] == "class":
                name = tokens[1].split(":")[0].split("(")[0]
            # pop scope if we are less indented
            while scope_stack and indent_info[scope_stack[-1]] >= indent:
                scope_stack.pop()
            if name:
                scope_stack.append(name)
                indent_info[name] = indent
                if scope_stack == qual_names:
                    return lines, i
    raise OSError("could not find class definition")


_getfile = inspect.getfile
_findsource = inspect.findsource
inspect.getfile = getfile  # type: ignore[assignment]
