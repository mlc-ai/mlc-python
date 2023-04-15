from __future__ import annotations

from enum import Enum
from typing import Any

import ml_dtypes
import numpy as np

from . import _cython as cy


class DataTypeCode(Enum):
    int = 0
    uint = 1
    float = 2
    handle = 3
    bfloat = 4
    complex = 5
    bool = 6
    float8_e4m3fn = 7
    float8_e5m2 = 8


PRESET: dict[Any, Any] = {
    np.dtype(np.bool_): "bool",
    np.dtype(np.int8): "int8",
    np.dtype(np.int16): "int16",
    np.dtype(np.int32): "int32",
    np.dtype(np.int64): "int64",
    np.dtype(np.uint8): "uint8",
    np.dtype(np.uint16): "uint16",
    np.dtype(np.uint32): "uint32",
    np.dtype(np.uint64): "uint64",
    np.dtype(np.float16): "float16",
    np.dtype(np.float32): "float32",
    np.dtype(np.float64): "float64",
    np.dtype(ml_dtypes.bfloat16): "bfloat16",
    np.dtype(ml_dtypes.float8_e4m3fn): "float8_e4m3fn",
    np.dtype(ml_dtypes.float8_e5m2): "float8_e5m2",
}


@cy.register_type("dtype")
class DataType(cy.core.PyAny):
    def __init__(self, dtype: str | np.dtype | DataType) -> None:
        dtype = PRESET.get(dtype, dtype)
        if isinstance(dtype, np.dtype):
            dtype = str(dtype)
        self._mlc_init("__init__", dtype)

    @property
    def _dtype_triple(self) -> tuple[int, int, int]:
        return cy.core.dtype_as_triple(self)

    @property
    def code(self) -> DataTypeCode:
        return DataTypeCode(self._dtype_triple[0])

    @property
    def bits(self) -> int:
        return self._dtype_triple[1]

    @property
    def lanes(self) -> int:
        return self._dtype_triple[2]

    def __eq__(self, other: object) -> bool:
        if isinstance(other, DataType):
            return self._dtype_triple == other._dtype_triple
        return False

    def __ne__(self, other: object) -> bool:
        if isinstance(other, DataType):
            return self._dtype_triple != other._dtype_triple
        return True
