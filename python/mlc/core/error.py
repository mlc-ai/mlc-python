from __future__ import annotations

import sys
from typing import TextIO

from mlc._cython import c_class_core, error_get_info

from .object import Object


@c_class_core("object.Error")
class Error(Object):
    kind: str

    @property
    def _info(self) -> list[str]:
        return error_get_info(self)

    def print_exc(self, file: TextIO | None) -> None:
        if not file:
            file = sys.stdout
        print("Traceback (most recent call last):", file=file)
        info = self._info
        msg, info = info[0], info[1:]
        frame_id = 0
        while info:
            frame_id += 1
            filename, lineno, funcname, info = info[0], int(info[1]), info[2], info[3:]
            print(f'  [{frame_id}] File "{filename}", line {lineno}, in {funcname}', file=file)
        print(f"{self.kind}: {msg}", file=file)
