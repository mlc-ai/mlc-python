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
