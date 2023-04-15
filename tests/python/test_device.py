import mlc
import pytest
from mlc import Device


@pytest.mark.parametrize("x", ["cpu", "cpu:1", "cuda:0", "mps:1"])
def test_device_init_str(x: str) -> None:
    func = mlc.get_global_func("mlc.testing.cxx_device")
    y = func(Device(x))
    z = func(x)
    if ":" not in x:
        x += ":0"
    assert isinstance(y, Device) and y == Device(x) and str(y) == x
    assert isinstance(z, Device) and z == Device(x) and str(z) == x


@pytest.mark.parametrize("x", ["unk"])
def test_device_init_fail(x: str) -> None:
    func = mlc.get_global_func("mlc.testing.cxx_device")
    try:
        func(x)
    except ValueError as e:
        assert str(e) == f"Cannot convert to `Device` from string: {x}"
    else:
        assert False
