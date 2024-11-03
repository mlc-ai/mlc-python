from mlc import Dict


def test_dict_init_len() -> None:
    a = Dict({i: i * i for i in range(1, 5)})
    assert len(a) == 4
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


def test_dict_key_error() -> None:
    a = Dict({i: i * i for i in range(1, 5)})
    try:
        a[5]
    except KeyError as e:
        assert str(e) == "'5'"
    else:
        assert False


def test_dict_str() -> None:
    a = Dict({i: i * i for i in range(1, 5)})
    assert "{" + ", ".join(sorted(str(a)[1:-1].split(", "))) + "}" == "{1: 1, 2: 4, 3: 9, 4: 16}"
