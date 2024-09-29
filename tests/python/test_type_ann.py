import typing

import mlc
import pytest
from mlc._cython import TypeAnn
from mlc._cython import testing_cast as mlc_cast


@pytest.mark.parametrize(
    ("type_ann", "type_str"),
    [
        (list[str], "list[str]"),
        (list, "list[Any]"),
        (dict[str, int], "dict[str, int]"),
        (dict, "dict[Any, Any]"),
        (typing.List[str], "list[str]"),  # noqa: UP006
        (typing.List, "list[Any]"),  # noqa: UP006
        (typing.Dict[str, int], "dict[str, int]"),  # noqa: UP006
        (typing.Dict, "dict[Any, Any]"),  # noqa: UP006
        (list[typing.Any], "list[Any]"),
        (list[list[int]], "list[list[int]]"),
        (dict[str, typing.Any], "dict[str, Any]"),
        (dict[typing.Any, str], "dict[Any, str]"),
        (dict[str, list[int]], "dict[str, list[int]]"),
    ],
)
def test_creation(type_ann: str, type_str: str) -> None:
    a = TypeAnn(type_ann)
    assert str(a) == type_str


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
def test_cast(type_ann: str, x: typing.Any) -> None:
    def _translate(x: typing.Any) -> typing.Any:
        if isinstance(x, mlc.List):
            return [_translate(v) for v in x]
        if isinstance(x, mlc.Dict):
            ret: dict = {}
            for k, v in x.items():
                k = _translate(k)  # noqa: PLW2901
                v = _translate(v)  # noqa: PLW2901
                if isinstance(k, list):
                    k = tuple(k)  # noqa: PLW2901
                ret[k] = v
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

    ann = TypeAnn(type_ann)
    y = mlc_cast(x, ann)
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
def test_cast_fail(type_ann: str, x: typing.Any) -> None:
    ann = TypeAnn(type_ann)
    with pytest.raises(TypeError):
        mlc_cast(x, ann)
