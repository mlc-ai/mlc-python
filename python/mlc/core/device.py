from __future__ import annotations

from mlc._cython import DeviceType, PyAny, c_class_core, device_as_pair, device_normalize


@c_class_core("Device")
class Device(PyAny):
    def __init__(self, device: str | Device) -> None:
        self._mlc_init(device_normalize(device))

    @property
    def _device_pair(self) -> tuple[int, int]:
        return device_as_pair(self)

    @property
    def device_type(self) -> DeviceType:
        return DeviceType(self._device_pair[0])

    @property
    def device_id(self) -> int:
        return self._device_pair[1]

    def __eq__(self, other: object) -> bool:
        return isinstance(other, Device) and self._device_pair == other._device_pair

    def __ne__(self, other: object) -> bool:
        return isinstance(other, Device) and self._dtype_triple != other._dtype_triple
