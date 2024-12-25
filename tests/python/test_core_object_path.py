import mlc


def test_root() -> None:
    root = mlc.ObjectPath.root()
    assert root.kind == -1
    assert str(root) == "{root}"


def test_with_field_0() -> None:
    obj = mlc.ObjectPath.root().with_field("field")
    assert obj.kind == 0
    assert str(obj) == "{root}.field"


def test_with_field_1() -> None:
    obj = mlc.ObjectPath.root()["field"]
    assert obj.kind == 0
    assert str(obj) == "{root}.field"


def test_with_list_index() -> None:
    obj = mlc.ObjectPath.root().with_list_index(1)
    assert obj.kind == 1
    assert str(obj) == "{root}[1]"


def test_with_list_index_1() -> None:
    obj = mlc.ObjectPath.root()[1]
    assert obj.kind == 1
    assert str(obj) == "{root}[1]"


def test_with_dict_key() -> None:
    obj = mlc.ObjectPath.root().with_dict_key("key")
    assert obj.kind == 2
    assert str(obj) == '{root}["key"]'
