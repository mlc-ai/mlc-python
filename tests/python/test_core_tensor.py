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


def test_tensor_serialize() -> None:
    a = mlc.Tensor(np.arange(24, dtype=np.int16).reshape(2, 3, 4))
    b = mlc.Tensor.from_base64(a.base64())
    assert a.ndim == b.ndim
    assert a.shape == b.shape
    assert a.dtype == b.dtype
    assert a.device == b.device
    assert a.strides == b.strides
    assert a.byte_offset == b.byte_offset
    assert a.base64() == b.base64()
