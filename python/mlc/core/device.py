from __future__ import annotations

from typing import TYPE_CHECKING

from mlc._cython import DeviceType, PyAny, c_class_core, device_as_pair, device_normalize

if TYPE_CHECKING:
    import torch


@c_class_core("Device")
class Device(PyAny):
    def __init__(self, device: str | Device | torch.device) -> None:
        self._mlc_init(device_normalize(device))

    @property
    def _device_pair(self) -> tuple[int, int]:
        return device_as_pair(self)

    @property
    def device_type(self) -> DeviceType | int:
        code = self._device_pair[0]
        try:
            return DeviceType(code)
        except ValueError:
            return code

    @property
    def device_id(self) -> int:
        return self._device_pair[1]

    def __eq__(self, other: object) -> bool:
        return isinstance(other, Device) and self._device_pair == other._device_pair

    def __ne__(self, other: object) -> bool:
        return isinstance(other, Device) and self._dtype_triple != other._dtype_triple

    def __hash__(self) -> int:
        return hash((Device, *self._device_pair))

    def torch(self) -> torch.device:
        import torch

        return torch.device(str(self))

    @staticmethod
    def register(name: str) -> int:
        from .func import Func

        return Func.get("mlc.base.DeviceTypeRegister")(name)
