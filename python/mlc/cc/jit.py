from __future__ import annotations

import tempfile
from collections.abc import Mapping, Sequence
from pathlib import Path
from types import MappingProxyType

from mlc._cython import DSO_SUFFIX, SYSTEM

from .compiler import DEFAULT_OPTIONS, create_shared
from .loader import load_dso


def jit_load(
    sources: str | Path | Sequence[Path | str],
    options: Mapping[str, Sequence[str]] = MappingProxyType(DEFAULT_OPTIONS),
) -> None:
    if isinstance(options, Sequence):
        options: dict[str, Sequence[str]] = {SYSTEM: sources}  # type: ignore[no-redef]
    with tempfile.TemporaryDirectory() as temp_dir_str:
        output = Path(temp_dir_str) / f"jit{DSO_SUFFIX}"
        create_shared(
            sources=sources,
            output=output,
            options=options,
        )
        load_dso(output)
