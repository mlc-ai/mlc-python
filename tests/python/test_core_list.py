from typing import Any

import pytest
from mlc import DataType, Device, List


@pytest.mark.parametrize("index", [-5, 4])
def test_list_index_out_of_bound(index: int) -> None:
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


def test_list_insert_middle() -> None:
    a = List([1, 2, 4, 5])
    a.insert(2, 3)
    assert list(a) == [1, 2, 3, 4, 5]


def test_list_insert_start() -> None:
    a = List([2, 3])
    a.insert(0, 1)
    assert list(a) == [1, 2, 3]


def test_list_insert_end() -> None:
    a = List([1, 2, 3])
    a.insert(len(a), 4)
    assert list(a) == [1, 2, 3, 4]


def test_list_append_single() -> None:
    a = List([1, 2])
    a.append(3)
    assert list(a) == [1, 2, 3]


def test_list_append_multiple() -> None:
    a = List[int]()
    for i in range(3):
        a.append(i)
    assert list(a) == [0, 1, 2]


def test_list_append_various_types() -> None:
    a = List[Any]()
    a.append(1)
    a.append("two")
    a.append(3.0)
    assert list(a) == [1, "two", 3.0]


def test_list_pop_no_arg() -> None:
    a = List([1, 2, 3])
    val = a.pop()
    assert val == 3
    assert list(a) == [1, 2]


def test_list_pop_index() -> None:
    a = List([0, 1, 2, 3])
    val = a.pop(1)
    assert val == 1
    assert list(a) == [0, 2, 3]


def test_list_pop_out_of_bound() -> None:
    a = List([1])
    with pytest.raises(IndexError):
        a.pop(5)


def test_list_clear_empty() -> None:
    a = List[Any]()
    a.clear()
    assert list(a) == []


def test_list_clear_non_empty() -> None:
    a = List([1, 2, 3])
    a.clear()
    assert list(a) == []


def test_list_clear_repeated() -> None:
    a = List([1, 2, 3])
    a.clear()
    a.clear()
    assert list(a) == []
    a = List[int](i * i for i in range(1, 5))
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


def test_list_extend_empty() -> None:
    a = List[int]()
    a.extend([1, 2, 3])
    assert list(a) == [1, 2, 3]


def test_list_extend_non_empty() -> None:
    a = List([1, 2])
    a.extend([3, 4])
    assert list(a) == [1, 2, 3, 4]


def test_list_extend_various_types() -> None:
    a = List[Any]()
    a.extend([1, "two", 3.0])
    assert list(a) == [1, "two", 3.0]


def test_list_extend_generator() -> None:
    a = List([0])
    a.extend(x for x in range(1, 4))
    assert list(a) == [0, 1, 2, 3]
