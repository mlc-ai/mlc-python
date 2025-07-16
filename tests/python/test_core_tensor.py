import mlc
import numpy as np
import pytest
import torch


@pytest.fixture
def cxx_func() -> mlc.Func:
    return mlc.Func.get("mlc.testing.cxx_obj")


def test_tensor_from_numpy(cxx_func: mlc.Func) -> None:
    a = np.arange(24, dtype=np.int16).reshape(2, 3, 4)

    b = cxx_func(a)
    assert b.dtype == mlc.DataType("int16")
    assert b.device == mlc.Device("cpu")
    assert b.shape == (2, 3, 4)
    assert b.strides is None
    assert b.byte_offset == 0
    assert str(b) == "<mlc.Tensor int16[2, 3, 4] @ cpu:0>"

    b = mlc.Tensor(a)
    assert b.dtype == mlc.DataType("int16")
    assert b.device == mlc.Device("cpu")
    assert b.shape == (2, 3, 4)
    assert b.strides is None
    assert b.byte_offset == 0
    assert str(b) == "<mlc.Tensor int16[2, 3, 4] @ cpu:0>"

    assert np.array_equal(a, b.numpy())


def test_opaque_from_torch(cxx_func: mlc.Func) -> None:
    a = torch.arange(24, dtype=torch.int16).reshape(2, 3, 4)

    b = cxx_func(a)
    assert b.dtype == mlc.DataType("int16")
    assert b.device == mlc.Device("cpu")
    assert b.shape == (2, 3, 4)
    assert b.strides is None
    assert b.byte_offset == 0
    assert str(b) == "<mlc.Tensor int16[2, 3, 4] @ cpu:0>"

    b = mlc.Tensor(a)
    assert b.dtype == mlc.DataType("int16")
    assert b.device == mlc.Device("cpu")
    assert b.shape == (2, 3, 4)
    assert b.strides is None
    assert b.byte_offset == 0
    assert str(b) == "<mlc.Tensor int16[2, 3, 4] @ cpu:0>"

    assert torch.equal(a, b.torch())


@pytest.mark.skipif(not torch.cuda.is_available(), reason="CUDA is not available")
def test_opaque_from_torch_cuda(cxx_func: mlc.Func) -> None:
    a = torch.arange(24, dtype=torch.int16).reshape(2, 3, 4).cuda()

    b = cxx_func(a)
    assert b.dtype == mlc.DataType("int16")
    assert b.device == mlc.Device("cuda")
    assert b.shape == (2, 3, 4)
    assert b.strides is None
    assert b.byte_offset == 0
    assert str(b) == "<mlc.Tensor int16[2, 3, 4] @ cuda:0>"

    b = mlc.Tensor(a)
    assert b.dtype == mlc.DataType("int16")
    assert b.device == mlc.Device("cuda")
    assert b.shape == (2, 3, 4)
    assert b.strides is None
    assert b.byte_offset == 0
    assert str(b) == "<mlc.Tensor int16[2, 3, 4] @ cuda:0>"

    assert torch.equal(a, b.torch())


def test_tensor_base64_int16() -> None:
    a = mlc.Tensor(np.arange(24, dtype=np.int16).reshape(2, 3, 4))
    assert (
        a.base64()
        == "P6G0lvBAXt0DAAAAABABAAIAAAAAAAAAAwAAAAAAAAAEAAAAAAAAAAAAAQACAAMABAAFAAYABwAIAAkACgALAAwADQAOAA8AEAARABIAEwAUABUAFgAXAA=="
    )
    b = mlc.Tensor.from_base64(a.base64())
    assert a.ndim == b.ndim
    assert a.shape == b.shape
    assert a.dtype == b.dtype
    assert a.device == b.device
    assert a.strides == b.strides
    assert a.byte_offset == b.byte_offset
    assert a.base64() == b.base64()

    assert np.array_equal(a.numpy(), b.numpy())
    assert torch.equal(a.torch(), b.torch())


def test_tensor_base64_float16() -> None:
    a = mlc.Tensor(np.array([3.0, 10.0, 20.0, 30.0, 35.50], dtype=np.float16))
    assert a.base64() == "P6G0lvBAXt0BAAAAAhABAAUAAAAAAAAAAEIASQBNgE9wUA=="
    b = mlc.Tensor.from_base64(a.base64())
    assert a.ndim == b.ndim
    assert a.shape == b.shape
    assert a.dtype == b.dtype
    assert a.device == b.device
    assert a.strides == b.strides
    assert a.byte_offset == b.byte_offset
    assert a.base64() == b.base64()

    assert np.array_equal(a.numpy(), b.numpy())
    assert torch.equal(a.torch(), b.torch())


def test_torch_strides() -> None:
    a = torch.empty(4, 1, 6, 1, 10, dtype=torch.int16)
    a = torch.from_dlpack(torch.to_dlpack(a))
    b = mlc.Tensor(a)
    assert b.strides is None


def test_tensor_serialize() -> None:
    a = mlc.Tensor(np.arange(24, dtype=np.int16).reshape(2, 3, 4))
    a_json = mlc.json_dumps(mlc.List([a, a]))
    b = mlc.json_loads(a_json)
    assert isinstance(b, mlc.List)
    assert len(b) == 2
    assert isinstance(b[0], mlc.Tensor)
    assert isinstance(b[1], mlc.Tensor)
    assert mlc.eq_ptr(b[0], b[1])
    assert np.array_equal(a.numpy(), b[0].numpy())
