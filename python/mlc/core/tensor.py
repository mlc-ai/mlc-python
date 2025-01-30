from __future__ import annotations

from typing import TYPE_CHECKING, Any

import numpy as np

from mlc._cython import (
    Ptr,
    c_class_core,
    tensor_byte_offset,
    tensor_data,
    tensor_device,
    tensor_dtype,
    tensor_init,
    tensor_ndim,
    tensor_shape,
    tensor_strides,
    tensor_to_dlpack,
)

from .func import Func
from .object import Object

if TYPE_CHECKING:
    import torch

    from mlc.core import DataType, Device


@c_class_core("mlc.core.Tensor")
class Tensor(Object):
    def __init__(self, tensor: Any) -> None:
        tensor_init(self, tensor)

    @property
    def data(self) -> Ptr:
        return tensor_data(self)

    @property
    def device(self) -> Device:
        return tensor_device(self)

    @property
    def dtype(self) -> DataType:
        return tensor_dtype(self)

    @property
    def ndim(self) -> int:
        return tensor_ndim(self)

    @property
    def shape(self) -> tuple[int, ...]:
        return tensor_shape(self)

    @property
    def strides(self) -> tuple[int, ...] | None:
        return tensor_strides(self)

    @property
    def byte_offset(self) -> int:
        return tensor_byte_offset(self)

    def base64(self) -> str:
        return TensorToBase64(self)

    @staticmethod
    def from_base64(base64: str) -> Tensor:
        return TensorFromBase64(base64)

    def __dlpack__(self) -> Any:
        return tensor_to_dlpack(self)

    def __dlpack_device__(self) -> tuple[int, int]:
        return self.device._device_pair

    def numpy(self) -> np.ndarray:
        return np.from_dlpack(self)

    def torch(self) -> torch.Tensor:
        import torch

        return torch.from_dlpack(self)


TensorToBase64 = Func.get("mlc.core.TensorToBase64")
TensorFromBase64 = Func.get("mlc.core.TensorFromBase64")
