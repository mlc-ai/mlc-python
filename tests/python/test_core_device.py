import mlc
import pytest
import torch
from mlc import Device


@pytest.mark.parametrize("x", ["cpu", "cpu:1", "cuda:0", "mps:1"])
def test_device_init_str(x: str) -> None:
    func = mlc.Func.get("mlc.testing.cxx_device")
    y = func(Device(x))
    z = func(x)
    if ":" not in x:
        x += ":0"
    assert isinstance(y, Device) and y == Device(x) and str(y) == x
    assert isinstance(z, Device) and z == Device(x) and str(z) == x


@pytest.mark.parametrize("x", ["unk"])
def test_device_init_fail(x: str) -> None:
    func = mlc.Func.get("mlc.testing.cxx_device")
    try:
        func(x)
    except ValueError as e:
        assert str(e) == f"Cannot convert to `Device` from string: {x}"
    else:
        assert False


@pytest.mark.parametrize(
    "x, y",
    [
        ("meta:0", torch.device("meta")),
        ("cpu:0", torch.device("cpu")),
        ("cpu:0", torch.device("cpu:0")),
        ("cuda:0", torch.device("cuda")),
        ("cuda:1", torch.device("cuda:1")),
        ("mps:2", torch.device("mps:2")),
    ],
)
def test_device_from_torch(x: str, y: torch.device) -> None:
    assert x == str(Device(y))


@pytest.mark.parametrize(
    "x, y",
    [
        (torch.device("meta:0"), Device("meta")),
        (torch.device("cpu:0"), Device("cpu")),
        (torch.device("cpu:0"), Device("cpu:0")),
        (torch.device("cuda:0"), Device("cuda")),
        (torch.device("cuda:1"), Device("cuda:1")),
        (torch.device("mps:2"), Device("mps:2")),
    ],
)
def test_device_to_torch(x: torch.device, y: Device) -> None:
    assert x == y.torch()


def test_device_register() -> None:
    code = Device.register("my_device")
    device = Device("my_device:10")
    assert device.device_type == code and device.device_id == 10
    assert str(device) == "my_device:10"
