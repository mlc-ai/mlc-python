from __future__ import annotations

from typing import TYPE_CHECKING

import numpy as np

from mlc._cython import (
    DataTypeCode,
    PyAny,
    c_class_core,
    dtype_as_triple,
    dtype_from_triple,
    dtype_normalize,
)

if TYPE_CHECKING:
    import torch


@c_class_core("dtype")
class DataType(PyAny):
    def __init__(self, dtype: str | np.dtype | type[torch.dtype] | DataType) -> None:
        self._mlc_init(dtype_normalize(dtype))

    @property
    def _dtype_triple(self) -> tuple[int, int, int]:
        return dtype_as_triple(self)

    @staticmethod
    def from_triple(code: DataTypeCode | int, bits: int, lanes: int) -> DataType:
        if isinstance(code, DataTypeCode):
            code = code.value
        return dtype_from_triple(code, bits, lanes)

    @property
    def code(self) -> DataTypeCode | int:
        code = self._dtype_triple[0]
        try:
            return DataTypeCode(code)
        except ValueError:
            return code

    @property
    def bits(self) -> int:
        return self._dtype_triple[1]

    @property
    def lanes(self) -> int:
        return self._dtype_triple[2]

    def __eq__(self, other: object) -> bool:
        if isinstance(other, str):
            other = DataType(other)
        return isinstance(other, DataType) and self._dtype_triple == other._dtype_triple

    def __ne__(self, other: object) -> bool:
        if isinstance(other, str):
            other = DataType(other)
        return isinstance(other, DataType) and self._dtype_triple != other._dtype_triple

    def __hash__(self) -> int:
        return hash((DataType, *self._dtype_triple))

    def torch(self) -> torch.dtype:
        import torch

        if (ret := getattr(torch, str(self), None)) is not None:
            if isinstance(ret, torch.dtype):
                return ret
        raise ValueError(f"Cannot convert to `torch.dtype` from: {self}")

    def numpy(self) -> np.dtype:
        return np.dtype(str(self))

    @staticmethod
    def register(name: str, bits: int) -> int:
        from .func import Func

        return Func.get("mlc.base.DataTypeRegister")(name, bits)
