from collections.abc import Sequence
from typing import Any

import pytest
from mlc import DataType, Device, List


@pytest.mark.parametrize("index", [-5, 4])
def test_list_index_out_of_bound(index: int) -> None:
    a = List[int](i * i for i in range(4))
    with pytest.raises(IndexError) as e:
        a[index]
    assert str(e.value) == f"list index out of range: {index}"


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


def test_list_eq() -> None:
    a = List([1, 2, 3])
    b = List(a)
    assert a == list(a)
    assert a == tuple(a)
    assert list(a) == a
    assert tuple(a) == a
    assert a == b
    assert b == a
    assert not a.eq_ptr(b)
    assert not b.eq_ptr(a)
    assert a.eq_ptr(a)
    assert a == a  # noqa: PLR0124


def test_list_ne() -> None:
    a = List([1, 2, 3])
    b = [1, 2]
    assert a != b
    assert b != a
    b = List([1, 2, 4])
    assert a != b
    assert b != a
    b = List([1, 2, 3, 4])
    assert a != b
    assert b != a


def test_list_setitem() -> None:
    a = List([1, 4, 9, 16])
    a[1] = 8
    assert tuple(a) == (1, 8, 9, 16)
    a[-2] = 12
    assert tuple(a) == (1, 8, 12, 16)
    a[-4] = 2
    assert tuple(a) == (2, 8, 12, 16)

    with pytest.raises(IndexError) as e:
        a[4] = 20
    assert str(e.value) == "list assignment index out of range: 4"

    with pytest.raises(IndexError) as e:
        a[-5] = 20
    assert str(e.value) == "list assignment index out of range: -5"


@pytest.mark.parametrize(
    "a, b",
    [
        (List([1, 2, 3]), [4, 5, 6]),
        (List([1, 2, 3]), (4, 5, 6)),
        (List([1, 2, 3]), List([4, 5, 6])),
        ([1, 2, 3], List([4, 5, 6])),
        ((1, 2, 3), List([4, 5, 6])),
    ],
)
def test_list_concat(a: Sequence[int], b: Sequence[int]) -> None:
    c = a + b  # type: ignore[operator]
    assert isinstance(c, List)
    assert c == [1, 2, 3, 4, 5, 6]


@pytest.mark.parametrize(
    "seq, i",
    [
        ([1, 2, 3], 0),
        ([1, 2, 3], 1),
        ([1, 2, 3], 2),
        ([1, 2, 3], -1),
        ([1, 2, 3], -2),
        ([1, 2, 3], -3),
    ],
)
def test_list_delitem(seq: list[int], i: int) -> None:
    a = List(seq)
    del a[i]
    del seq[i]
    assert list(a) == list(seq)


@pytest.mark.parametrize(
    "seq, i",
    [
        ([1, 2, 3], 3),
        ([1, 2, 3], -4),
    ],
)
def test_list_delitem_out_of_by(seq: list[int], i: int) -> None:
    a = List(seq)
    with pytest.raises(IndexError) as e:
        del a[i]
    assert str(e.value) == f"list index out of range: {i}"


def test_list_to_py_0() -> None:
    a = List([1, 2, 3]).py()
    assert isinstance(a, list)


def test_list_to_py_1() -> None:
    a = List([{"a": 1}, ["b"], 1, 2.0, "anything"]).py()
    assert isinstance(a, list)
    assert len(a) == 5
    assert isinstance(a[0], dict)
    assert isinstance(a[1], list)
    assert isinstance(a[2], int)
    assert isinstance(a[3], float)
    assert isinstance(a[4], str)
    assert isinstance(a[0], dict)
    # make sure those types are exactly Python's `str`, `int`, `float`
    assert len(a[0]) == 1 and isinstance(a[0], dict)
    assert a[0]["a"] == 1 and type(next(a[0].__iter__())) is str
    assert len(a[1]) == 1 and isinstance(a[1], list)
    assert a[1][0] == "b" and type(a[1][0]) is str
    assert a[2] == 1 and type(a[2]) is int
    assert a[3] == 2.0 and type(a[3]) is float
    assert a[4] == "anything" and type(a[4]) is str
