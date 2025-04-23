from collections.abc import Callable

import pytest
from mlc import Dict


def test_dict_init_len() -> None:
    a = Dict({i: i * i for i in range(1, 5)})
    assert len(a) == 4
    assert sorted(list(a)) == [1, 2, 3, 4]
    assert a[1] == 1
    assert a[2] == 4
    assert a[3] == 9
    assert a[4] == 16


def test_dict_iter() -> None:
    a = Dict({i: i * i for i in range(1, 5)})
    assert sorted(a) == [1, 2, 3, 4]
    assert sorted(a.keys()) == [1, 2, 3, 4]
    assert sorted(a.values()) == [1, 4, 9, 16]
    assert sorted(a.items()) == [(1, 1), (2, 4), (3, 9), (4, 16)]


def test_dict_set_item() -> None:
    a = Dict({i: i * i for i in range(1, 5)})
    a[1] = -1
    a[2] = -4
    assert a[1] == -1
    assert a[2] == -4
    assert dict(a) == {1: -1, 2: -4, 3: 9, 4: 16}


def test_dict_del_item() -> None:
    a = Dict({i: i * i for i in range(1, 5)})
    del a[1]
    del a[2]
    assert dict(a) == {3: 9, 4: 16}


def test_dict_pop() -> None:
    a = Dict({i: i * i for i in range(1, 5)})
    assert a.pop(1) == 1
    assert dict(a) == {2: 4, 3: 9, 4: 16}
    assert a.pop(2) == 4
    assert dict(a) == {3: 9, 4: 16}
    with pytest.raises(KeyError):
        a.pop(5)
    assert a.pop("not found", "default") == "default"  # type: ignore[arg-type]


def test_dict_key_error() -> None:
    a = Dict({i: i * i for i in range(1, 5)})
    try:
        a[5]
    except KeyError as e:
        assert str(e) == "'5'"
    else:
        assert False


def test_dict_get() -> None:
    a = Dict({i: i * i for i in range(1, 5)})
    assert a.get(1) == 1
    assert a.get(2) == 4
    assert a.get(3) == 9
    assert a.get(4) == 16
    assert a.get(5) is None


def test_dict_str() -> None:
    a = Dict({i: i * i for i in range(1, 5)})
    assert "{" + ", ".join(sorted(str(a)[1:-1].split(", "))) + "}" == "{1: 1, 2: 4, 3: 9, 4: 16}"


def test_dict_setdefault() -> None:
    a = Dict({i: i * i for i in range(1, 5)})
    assert a.setdefault(1, -1) == 1
    assert a.setdefault(2, -4) == 4
    assert a.setdefault(5, 25) == 25
    assert dict(a) == {1: 1, 2: 4, 3: 9, 4: 16, 5: 25}


def test_dict_eq() -> None:
    a = Dict({i: i * i for i in range(1, 5)})
    b = {i: i * i for i in range(1, 5)}
    assert a == b
    assert b == a
    assert a == Dict(b)
    assert Dict(b) == a
    assert not a.eq_ptr(Dict(b))
    assert not Dict(b).eq_ptr(a)
    assert a == a  # noqa: PLR0124
    assert a.eq_ptr(a)


def test_dict_ne_0() -> None:
    a = Dict({i: i * i for i in range(1, 5)})
    b = {i: i * i for i in range(1, 6)}
    assert a != b
    assert b != a


def test_dict_ne_1() -> None:
    a = Dict({i: i * i for i in range(1, 6)})
    b = {i: i * i for i in range(1, 5)}
    assert a != b
    assert b != a


def test_dict_to_py_0() -> None:
    a = Dict({i: i * i for i in range(1, 5)}).py()
    assert isinstance(a, dict)
    assert len(a) == 4
    assert isinstance(a[1], int) and a[1] == 1
    assert isinstance(a[2], int) and a[2] == 4
    assert isinstance(a[3], int) and a[3] == 9
    assert isinstance(a[4], int) and a[4] == 16


def test_dict_to_py_1() -> None:
    a = Dict(
        {
            "a": {
                "b": [2],
                "c": 3.0,
            },
            1: "one",
            None: "e",
        }
    ).py()
    assert len(a) == 3 and set(a.keys()) == {"a", 1, None}
    assert isinstance(a["a"], dict) and len(a["a"]) == 2
    assert isinstance(a["a"]["b"], list) and len(a["a"]["b"]) == 1 and a["a"]["b"][0] == 2
    assert isinstance(a["a"]["c"], float) and a["a"]["c"] == 3.0
    assert isinstance(a[1], str) and a[1] == "one"
    assert isinstance(a[None], str) and a[None] == "e"


@pytest.mark.parametrize(
    "callable",
    [
        lambda a: a.__setitem__(0, 0),
        lambda a: a.__delitem__(1),
        lambda a: a.pop(2),
        lambda a: a.clear(),
        lambda a: a.setdefault(1, 5),
    ],
)
def test_dict_freeze(callable: Callable[[Dict], None]) -> None:
    a = Dict({i: i * i for i in range(1, 5)})
    assert a.frozen == False
    a.freeze()
    assert a.frozen == True
    with pytest.raises(RuntimeError) as e:
        callable(a)
    assert str(e.value) == "Cannot modify a frozen dict"
