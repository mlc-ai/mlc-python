from __future__ import annotations

import shutil
from pathlib import Path

from mlc._cython import SYSTEM
from mlc.core import Func

_C_load_dso = Func.get("mlc.ffi.LoadDSO")


def load_dso(path: Path) -> None:
    filename: Path | None = None
    if SYSTEM == "Windows":
        filename = Path.cwd() / "main.dll"
        shutil.copy(str(path), str(filename))
        path = filename
    return _C_load_dso(str(path))
