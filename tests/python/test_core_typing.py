import typing

import mlc
import pytest

get_func: mlc.Func = mlc.Func.get("mlc.testing.nested_type_checking_list")


@pytest.mark.parametrize(
    ("type_ann", "type_str", "args"),
    [
        (list[str], "list[str]", ("str",)),
        (list, "list[Any]", ("Any",)),
        (dict[str, int], "dict[str, int]", ("str", "int")),
        (dict, "dict[Any, Any]", ("Any", "Any")),
        (typing.List[str], "list[str]", ("str",)),  # noqa: UP006
        (typing.List, "list[Any]", ("Any",)),  # noqa: UP006
        (typing.Dict[str, int], "dict[str, int]", ("str", "int")),  # noqa: UP006
        (typing.Dict, "dict[Any, Any]", ("Any", "Any")),  # noqa: UP006
        (list[typing.Any], "list[Any]", ("Any",)),
        (list[list[int]], "list[list[int]]", ("list[int]",)),
        (dict[str, typing.Any], "dict[str, Any]", ("str", "Any")),
        (dict[typing.Any, str], "dict[Any, str]", ("Any", "str")),
        (dict[str, list[int]], "dict[str, list[int]]", ("str", "list[int]")),
    ],
)
def test_creation(type_ann: type, type_str: str, args: tuple[str, ...]) -> None:
    a = mlc.typing.from_py(type_ann)
    assert str(a) == type_str
    a_args = tuple(str(arg) for arg in a.args())
    assert a_args == args


@pytest.mark.parametrize(
    ("type_ann", "x"),
    [
        (list[str], ["hello", "world", "python"]),
        (list, [1, "two", 3.0, [4, 5], {"six": 6}]),
        (dict[str, int], {"one": 1, "two": 2, "three": 3}),
        (dict, {"a": 1, "b": "two", "c": [3, 4], "d": {"nested": 1}}),
        (list[typing.Any], [1, "two", 3.0, [4, 5], {"six": 6}]),
        (list[list[int]], [[1, 2, 3], [4, 5, 6], [7, 8, 9]]),
        (dict[str, typing.Any], {"a": 1, "b": "two", "c": [3, 4], "d": {"nested": 1}}),
        (dict[typing.Any, str], {1: "one", "two": "two", 3.0: "three", (4, 5): "four_five"}),
        (dict[str, list[int]], {"a": [1, 2, 3], "b": [4, 5, 6], "c": [7, 8, 9]}),
    ],
)
def test_cast(type_ann: type, x: typing.Any) -> None:
    def _translate(x: typing.Any) -> typing.Any:
        if isinstance(x, mlc.List):
            return [_translate(v) for v in x]
        if isinstance(x, mlc.Dict):
            ret: dict = {}
            for k, v in x.items():
                kk = _translate(k)
                if isinstance(kk, list):
                    kk = tuple(kk)
                ret[kk] = _translate(v)
            return ret
        if isinstance(x, mlc.Str):
            return str(x)
        if isinstance(x, (int, float)):
            return x
        raise ValueError(f"Unexpected type: {type(x)}")

    def _json_compare(x: typing.Any, y: typing.Any) -> bool:
        if type(x) is not type(y):
            raise ValueError(f"Type mismatch: {type(x)} != {type(y)}")
        if isinstance(x, list):
            if len(x) != len(y):
                raise ValueError(f"Length mismatch: {len(x)} != {len(y)}")
            return all(_json_compare(x[i], y[i]) for i in range(len(x)))
        if isinstance(x, dict):
            if len(x) != len(y):
                return False
            for k in x.keys():
                if k in y and _json_compare(x[k], y[k]):
                    continue
                raise ValueError(f"Key mismatch: {k} not in {y} or values differ")
            return True
        if isinstance(x, (str, int, float)):
            if x != y:
                raise ValueError(f"Value mismatch: {x} != {y}")
            return x == y
        raise ValueError(f"Unexpected type: {type(x)}")

    a = mlc.typing.from_py(type_ann)
    y = a.cast(x)
    y = _translate(y)
    assert _json_compare(x, y)


@pytest.mark.parametrize(
    ("type_ann", "x"),
    [
        (list[str], ["hello", 42, "python"]),
        (list, "str"),
        (dict[str, int], {"one": 1, "two": "2", "three": 3}),
        (dict, 1),
        (list[typing.Any], 2.0),
        (list[list[int]], [[1, 2, 3], [4, "five", 6], [7, 8, 9]]),
        (dict[str, typing.Any], {1: "one", "b": "two"}),
        (dict[typing.Any, str], {1: "one", "two": 2, 3.0: "three"}),
        (dict[str, list[int]], {"a": [1, 2, 3], "b": [4, "five", 6], "c": [7, 8, 9]}),
    ],
)
def test_cast_fail(type_ann: type, x: typing.Any) -> None:
    a = mlc.typing.from_py(type_ann)
    with pytest.raises(TypeError):
        a.cast(x)


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
