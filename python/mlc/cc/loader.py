import shutil
from pathlib import Path

from mlc._cython import SYSTEM
from mlc.func import Func

_C_load_dso = Func.get("mlc.ffi.LoadDSO")


def load_dso(path: Path) -> None:
    if SYSTEM == "Windows":
        from mlc.config import libdir

        filename = libdir() / path.name
        shutil.move(str(path), str(filename))
        path = filename
    return _C_load_dso(str(path))
