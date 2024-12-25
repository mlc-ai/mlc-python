from __future__ import annotations

import sys
import typing

if typing.TYPE_CHECKING:
    import pygments  # type: ignore[import-untyped]


def cprint(printable: str, style: str | None = None) -> None:
    """Print Python code with Pygments highlight.

    Parameters
    ----------
    style : str, optional

        Pygmentize printing style, auto-detected if None.

    Notes
    -----

    The style parameter follows the Pygments style names or Style objects. Three
    built-in styles are extended: "light", "dark" and "ansi". By default, "light"
    will be used for notebook environment and terminal style will be "ansi" for
    better style consistency. As an fallback when the optional Pygment library is
    not installed, plain text will be printed with a one-time warning to suggest
    installing the Pygment library. Other Pygment styles can be found in
    https://pygments.org/styles/
    """
    is_in_notebook = "ipykernel" in sys.modules  # in notebook env (support html display).

    pygment_style = _get_pygments_style(style, is_in_notebook)

    if pygment_style is None:
        print(printable)
        return

    # pylint: disable=import-outside-toplevel
    from pygments import highlight  # type: ignore[import-untyped]
    from pygments.formatters import (  # type: ignore[import-untyped]
        HtmlFormatter,
        Terminal256Formatter,
    )
    from pygments.lexers.python import Python3Lexer  # type: ignore[import-untyped]

    if is_in_notebook:
        from IPython import display  # type: ignore[import-not-found]

        formatter = HtmlFormatter(style=pygment_style)
        formatter.noclasses = True  # inline styles
        html = highlight(printable, Python3Lexer(), formatter)
        display.display(display.HTML(html))
    else:
        print(highlight(printable, Python3Lexer(), Terminal256Formatter(style=pygment_style)))


def _get_pygments_style(
    style: str | None,
    is_in_notebook: bool,
) -> pygments.style.Style | str | None:
    from pygments.style import Style  # type: ignore[import-untyped]
    from pygments.token import (  # type: ignore[import-untyped]
        Comment,
        Keyword,
        Name,
        Number,
        Operator,
        String,
    )

    class JupyterLight(Style):
        """A Jupyter-Notebook-like Pygments style configuration (aka. "light")"""

        background_color = ""
        styles: typing.ClassVar = {
            Keyword: "bold #008000",
            Keyword.Type: "nobold #008000",
            Name.Function: "#0000FF",
            Name.Class: "bold #0000FF",
            Name.Decorator: "#AA22FF",
            String: "#BA2121",
            Number: "#008000",
            Operator: "bold #AA22FF",
            Operator.Word: "bold #008000",
            Comment: "italic #007979",
        }

    class VSCDark(Style):
        """A VSCode-Dark-like Pygments style configuration (aka. "dark")"""

        background_color = ""
        styles: typing.ClassVar = {
            Keyword: "bold #c586c0",
            Keyword.Type: "#82aaff",
            Keyword.Namespace: "#4ec9b0",
            Name.Class: "bold #569cd6",
            Name.Function: "bold #dcdcaa",
            Name.Decorator: "italic #fe4ef3",
            String: "#ce9178",
            Number: "#b5cea8",
            Operator: "#bbbbbb",
            Operator.Word: "#569cd6",
            Comment: "italic #6a9956",
        }

    class AnsiTerminalDefault(Style):
        """The default style for terminal display with ANSI colors (aka. "ansi")"""

        background_color = ""
        styles: typing.ClassVar = {
            Keyword: "bold ansigreen",
            Keyword.Type: "nobold ansigreen",
            Name.Class: "bold ansiblue",
            Name.Function: "bold ansiblue",
            Name.Decorator: "italic ansibrightmagenta",
            String: "ansiyellow",
            Number: "ansibrightgreen",
            Operator: "bold ansimagenta",
            Operator.Word: "bold ansigreen",
            Comment: "italic ansibrightblack",
        }

    if style == "light":
        return JupyterLight
    elif style == "dark":
        return VSCDark
    elif style == "ansi":
        return AnsiTerminalDefault
    if style is not None:
        return style
    if is_in_notebook:
        return JupyterLight
    return AnsiTerminalDefault
