from __future__ import annotations

from enum import Enum

from . import _cython as cy


class DeviceType(Enum):
    cpu = 1
    cuda = 2
    cuda_host = 3
    opencl = 4
    vulkan = 7
    mps = 8
    vpi = 9
    rocm = 10
    rocm_host = 11
    ext_dev = 21
    cuda_managed = 13
    one_api = 14
    webgpu = 15
    hexagon = 16
    maia = 17


@cy.register_type("Device")
class Device(cy.core.PyAny):
    def __init__(self, device: str | Device) -> None:
        self._mlc_init("__init__", device)

    @property
    def _device_pair(self) -> tuple[int, int]:
        return cy.core.device_as_pair(self)

    @property
    def device_type(self) -> DeviceType:
        return DeviceType(self._device_pair[0])

    @property
    def device_id(self) -> int:
        return self._device_pair[1]

    def __eq__(self, other: object) -> bool:
        if isinstance(other, Device):
            return self._device_pair == other._device_pair
        return False

    def __ne__(self, other: object) -> bool:
        if isinstance(other, Device):
            return self._dtype_triple != other._dtype_triple
        return True
