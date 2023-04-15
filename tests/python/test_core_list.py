import pytest
from mlc import DataType, Device, List


def test_list_init_len_iter() -> None:
    a = List(i * i for i in range(1, 5))
    assert len(a) == 4
    assert a[0] == 1
    assert a[1] == 4
    assert a[2] == 9
    assert a[3] == 16
    assert a[-1] == 16
    assert a[-2] == 9
    assert a[-3] == 4
    assert a[-4] == 1
    assert list(a) == [1, 4, 9, 16]


@pytest.mark.parametrize("index", [-5, 4])
def test_list_index_out_of_bound(index: int) -> None:
    a: List[int]
    a = List[int](i * i for i in range(4))
    try:
        a[index]
    except IndexError as e:
        assert str(e) == f"list index out of range: {index}"


def test_list_init_with_list() -> None:
    a = List[int]([1, 4, 9, 16])
    b = List(a)
    assert list(a) == list(b)


def test_list_str() -> None:
    a = List((1, "a", DataType("int32"), Device("cpu:0")))
    assert str(a) == '[1, "a", int32, cpu:0]'
