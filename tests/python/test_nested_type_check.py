import mlc
import pytest

get_func: mlc.Func = mlc.Func.get("mlc.testing.nested_type_checking_list")


def test_list() -> None:
    func = get_func("list")
    func([1, "two", [3], {"four": 4}])


def test_list_fail() -> None:
    func = get_func("list")
    with pytest.raises(TypeError) as excinfo:
        func({"a": 1, "b": 2})
    assert (
        "Mismatched type on argument #0 when calling: `(0: list[Any]) -> list[Any]`. "
        "Expected `list[Any]` but got `object.Dict`" == str(excinfo.value)
    )


def test_list_any() -> None:
    func = get_func("list[Any]")
    func([1, "two", [3], {"four": 4}])


def test_list_list_int() -> None:
    func = get_func("list[list[int]]")
    func([[1, 2], [3, 4, 5], [6]])


def test_list_list_int_fail() -> None:
    func = get_func("list[list[int]]")
    with pytest.raises(TypeError) as excinfo:
        func([[1, 2], [3, 4], 5])
    assert (
        "Mismatched type on argument #0 when calling: `(0: list[list[int]]) -> list[list[int]]`. "
        "Let input be `A: list[list[int]]`. Type mismatch on `A[2]`: "
        "Cannot convert from type `int` to `object.ListObj[Any] *`" == str(excinfo.value)
    )


def test_dict() -> None:
    func = get_func("dict")
    func({"a": 1, "b": "two", "c": [3]})


def test_dict_fail() -> None:
    func = get_func("dict")
    with pytest.raises(TypeError) as excinfo:
        func(["a", 1, "b", 2])
    assert (
        "Mismatched type on argument #0 when calling: `(0: dict[Any, Any]) -> dict[Any, Any]`. "
        "Expected `dict[Any, Any]` but got `object.List`" == str(excinfo.value)
    )


def test_dict_str_any() -> None:
    func = get_func("dict[str, Any]")
    func({"a": 1, "b": "two", "c": [3]})


def test_dict_str_any_fail() -> None:
    func = get_func("dict[str, Any]")
    with pytest.raises(TypeError) as excinfo:
        func({"a": 1, "b": "two", 1: 3})
    assert (
        "Mismatched type on argument #0 when calling: `(0: dict[str, Any]) -> dict[str, Any]`. "
        "Let input be `A_0: dict[str, Any]`, `A_1: str in A_0.keys()`. "
        "Type mismatch on `A_1`: Cannot convert from type `int` to `object.StrObj *`"
        == str(excinfo.value)
    )


def test_dict_any_str() -> None:
    func = get_func("dict[Any, str]")
    func({1: "one", "two": "dos", (3, 4): "three-four"})


def test_dict_any_str_fail() -> None:
    func = get_func("dict[Any, str]")
    with pytest.raises(TypeError) as excinfo:
        func({1: "one", "two": "dos", (3, 4): [5, 6]})
    assert (
        "Mismatched type on argument #0 when calling: `(0: dict[Any, str]) -> dict[Any, str]`. "
        "Let input be `A: dict[Any, str]`. "
        "Type mismatch on `A[[3, 4]]`: Cannot convert from type `object.List` to `object.StrObj *`"
        == str(excinfo.value)
    )


def test_dict_any_any() -> None:
    func = get_func("dict[Any, Any]")
    func({"a": 1, "b": "two", "c": [3]})


def test_dict_str_list_int() -> None:
    func = get_func("dict[str, list[int]]")
    func({"evens": [2, 4, 6], "odds": [1, 3, 5]})


def test_dict_str_list_int_fail() -> None:
    func = get_func("dict[str, list[int]]")
    with pytest.raises(TypeError) as excinfo:
        func({"evens": [2, 4, 6], "odds": [1, 3, [5]]})
    assert (
        "Mismatched type on argument #0 when calling: `(0: dict[str, list[int]]) -> dict[str, list[int]]`. "
        "Let input be `A: dict[str, list[int]]`. "
        'Type mismatch on `A["odds"][2]`: Cannot convert from type `object.List` to `int`'
        == str(excinfo.value)
    )
