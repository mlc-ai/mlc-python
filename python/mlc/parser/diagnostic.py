from __future__ import annotations

import os
import sys
from types import TracebackType
from typing import Any, Literal

import colorama  # type: ignore[import-untyped]

from .env import Span

colorama.init(autoreset=True)


class DiagnosticError(Exception):
    pass


def raise_diagnostic_error(
    source: str,
    source_name: str,
    span: Span,
    err: Exception | str,
) -> None:
    diag_err = DiagnosticError("Diagnostics were emitted, please check rendered error message.")
    if isinstance(err, Exception):
        msg = type(err).__name__ + ": " + [i for i in str(err).split("\n") if i][-1]
        diag_err.with_traceback(err.__traceback__)
    else:
        msg = str(err)
    print(
        _render_at(
            source=source,
            source_name=source_name,
            span=span,
            message=msg,
            level="error",
        ),
        file=sys.stderr,
    )
    if isinstance(err, Exception):
        raise diag_err from err
    else:
        raise diag_err


def _render_at(
    source: str,
    source_name: str,
    span: Span,
    message: str,
    level: Literal["warning", "error", "bug", "note"] = "error",
) -> str:
    lines = source.splitlines()
    row_st = max(1, span.row_st)
    row_ed = min(len(lines), span.row_ed)
    # If no valid rows, just return the bare message.
    if row_st > row_ed:
        return message
    # Map the "level" to a color and label (similar to rang::fg usage in C++).
    color, diag_type = {
        "warning": (colorama.Fore.YELLOW, "warning"),
        "error": (colorama.Fore.RED, "error"),
        "bug": (colorama.Fore.BLUE, "bug"),
        "note": (colorama.Fore.RESET, "note"),
        "help": (colorama.Fore.RESET, "help"),
    }.get(level, (colorama.Fore.RED, "error"))

    # Prepare lines of output
    out_lines = [
        f"{colorama.Style.BRIGHT}{color}{diag_type}{colorama.Style.RESET_ALL}: {message}",
        f"{colorama.Fore.BLUE} --> {colorama.Style.RESET_ALL}{source_name}:{row_st}:{span.col_st}",
    ]
    left_margin_width = len(str(row_ed))
    for row_idx in range(row_st, row_ed + 1):
        line_text = lines[row_idx - 1]  # zero-based
        line_label = str(row_idx).rjust(left_margin_width)
        # Step 1. the actual source line
        out_lines.append(f"{line_label} |  {line_text}")
        # Step 2. the marker line:
        marker = [" "] * len(line_text)
        # For the first line...
        if row_idx == row_st and row_idx == row_ed:
            # Case 1. Single-line: highlight col_st..col_ed
            c_start = max(1, span.col_st)
            c_end = min(len(line_text), span.col_ed)
            marker[c_start:c_end] = "^" * (c_end - c_start)
        elif row_idx == row_st:
            # Case 2. The first line in a multi-line highlight
            c_start = max(1, span.col_st)
            marker[c_start:] = "^" * (len(line_text) - c_start)
        elif row_idx == row_ed:
            # Case 3. The last line in a multi-line highlight
            c_end = min(len(line_text), span.col_ed)
            marker[:c_end] = "^" * c_end
        else:
            # Case 4. A line in the middle of row_st..row_ed => highlight entire line
            marker = ["^"] * len(line_text)
        out_lines.append(f"{' ' * (left_margin_width)} |  {''.join(marker)}")
    return "\n".join(out_lines)


def excepthook(
    exctype: type[BaseException],
    value: BaseException,
    traceback: TracebackType | None,
) -> Any:
    should_hide_backtrace = os.environ.get("MLC_BACKTRACE", None) is None
    if exctype is DiagnosticError and should_hide_backtrace:
        print("note: run with `MLC_BACKTRACE=1` environment variable to display a backtrace.")
        return
    sys_excepthook(exctype, value, traceback)


sys_excepthook = sys.excepthook
sys.excepthook = excepthook
