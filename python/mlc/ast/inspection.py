from __future__ import annotations

import contextlib
import dataclasses
import inspect
from collections.abc import Callable, Generator
from typing import Any

PY_GETFILE = inspect.getfile
PY_FINDSOURCE = inspect.findsource


@dataclasses.dataclass(init=False)
class InspectResult:
    source_name: str
    source_start_line: int
    source_start_column: int
    source: str
    source_full: str
    captured: dict[str, Any]
    annotations: dict[str, dict[str, Any]]

    def is_defined_in_class(
        self,
        frames: list[inspect.FrameInfo],  # obtained via `inspect.stack()`
        *,
        frame_offset: int = 2,
        is_decorator: Callable[[str], bool] = lambda line: line.startswith("@"),
    ) -> bool:
        # Step 1. Inspect `frames[frame_offset]`
        try:
            lineno = frames[frame_offset].lineno
            line = frames[frame_offset].code_context[0].strip()  # type: ignore
        except:
            return False
        # Step 2. Determine by the line itself
        if is_decorator(line):
            return True
        if not line.startswith("class"):
            return False
        # Step 3. Determine by its decorators
        source_lines = self.source_full.splitlines(keepends=True)
        lineno_offset = 2
        try:
            source_line = source_lines[lineno - lineno_offset]
        except IndexError:
            return False
        return is_decorator(source_line.strip())


def inspect_program(program: Callable | type) -> InspectResult:
    ret = InspectResult()
    source = inspect_source(program)
    ret.source_name, ret.source_start_line, ret.source_start_column, ret.source, ret.source_full = (
        source.source_name,
        source.source_start_line,
        source.source_start_column,
        source.source,
        source.source_full,
    )
    if inspect.isfunction(program):
        ret.captured = inspect_capture_function(program)
        ret.annotations = inspect_annotations_function(program)
    elif inspect.isclass(program):
        ret.captured = inspect_capture_class(program)
        ret.annotations = inspect_annotations_class(program)
    else:
        raise TypeError(f"{program!r} is not a function or class")
    return ret


@contextlib.contextmanager
def override_getfile() -> Generator[None, Any, None]:
    try:
        inspect.getfile = getfile  # type: ignore[assignment]
        yield
    finally:
        inspect.getfile = PY_GETFILE  # type: ignore[assignment]


def getfile(obj: Any) -> str:
    if not inspect.isclass(obj):
        return PY_GETFILE(obj)
    mod = getattr(obj, "__module__", None)
    if mod is not None:
        import sys

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
        return PY_FINDSOURCE(obj)

    import linecache

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


@dataclasses.dataclass
class Source:
    source_name: str
    source_start_line: int
    source_start_column: int
    source: str
    source_full: str


def inspect_source(program: Callable | type) -> Source:
    with override_getfile():
        source_name: str = inspect.getsourcefile(program)  # type: ignore
        lines, source_start_line = getsourcelines(program)  # type: ignore
        if lines:
            source_start_column = len(lines[0]) - len(lines[0].lstrip())
        else:
            source_start_column = 0
        if source_start_column and lines:
            source = "\n".join([l[source_start_column:].rstrip() for l in lines])
        else:
            source = "".join(lines)
        try:
            # It will cause a problem when running in Jupyter Notebook.
            # `mod` will be <module '__main__'>, which is a built-in module
            # and `getsource` will throw a TypeError
            mod = inspect.getmodule(program)
            if mod:
                source_full = inspect.getsource(mod)
            else:
                source_full = source
        except TypeError:
            # It's a work around for Jupyter problem.
            # Since `findsource` is an internal API of inspect, we just use it
            # as a fallback method.
            src, _ = inspect.findsource(program)  # type: ignore
            source_full = "".join(src)
        return Source(
            source_name=source_name,
            source_start_line=source_start_line,
            source_start_column=source_start_column,
            source=source,
            source_full=source_full,
        )


def inspect_annotations_function(program: Callable | type) -> dict[str, dict[str, Any]]:
    return {program.__name__: program.__annotations__}


def inspect_annotations_class(program: Callable | type) -> dict[str, dict[str, Any]]:
    annotations = {}
    for name, func in program.__dict__.items():
        if inspect.isfunction(func):
            annotations[name] = func.__annotations__
    return annotations


def inspect_capture_function(func: Callable) -> dict[str, Any]:
    def _getclosurevars(func: Callable) -> dict[str, Any]:
        # Mofiied from `inspect.getclosurevars`
        if inspect.ismethod(func):
            func = func.__func__
        if not inspect.isfunction(func):
            raise TypeError(f"{func!r} is not a Python function")
        code = func.__code__
        # Nonlocal references are named in co_freevars and resolved
        # by looking them up in __closure__ by positional index
        nonlocal_vars = {}
        if func.__closure__ is not None:
            for var, cell in zip(code.co_freevars, func.__closure__):
                try:
                    nonlocal_vars[var] = cell.cell_contents
                except ValueError as err:
                    # cell_contents may raise ValueError if the cell is empty.
                    if "empty" not in str(err):
                        raise
        return nonlocal_vars

    return {
        **func.__globals__,
        **_getclosurevars(func),
    }


def inspect_capture_class(cls: type) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for _, v in cls.__dict__.items():
        if inspect.isfunction(v):
            func_vars = inspect_capture_function(v)
            result.update(**func_vars)
    return result
