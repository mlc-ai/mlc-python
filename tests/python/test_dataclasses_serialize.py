import json
import pickle
from typing import Any, Optional

import mlc


@mlc.dataclasses.py_class("mlc.testing.serialize", init=False)
class ObjTest(mlc.PyClass):
    a: int
    b: float
    c: str
    d: bool

    def __init__(self, b: float, c: str, a: int, d: bool) -> None:
        self.a = a
        self.b = b
        self.c = c
        self.d = d


@mlc.dataclasses.py_class("mlc.testing.serialize_opt")
class ObjTestOpt(mlc.PyClass):
    a: Optional[int]
    b: Optional[float]
    c: Optional[str]
    d: Optional[bool]


@mlc.dataclasses.py_class("mlc.testing.AnyContainer")
class AnyContainer(mlc.PyClass):
    field: Any


def test_json() -> None:
    obj = ObjTest(a=1, b=2.0, c="3", d=True)
    obj_json = obj.json()
    obj_json_dict = json.loads(obj_json)
    assert obj_json_dict["type_keys"] == ["mlc.testing.serialize", "int"]
    assert obj_json_dict["values"] == ["3", [0, [1, 1], 2.0, 0, True]]
    obj_from_json: ObjTest = ObjTest.from_json(obj_json)
    assert obj.a == obj_from_json.a
    assert obj.b == obj_from_json.b
    assert obj.c == obj_from_json.c
    assert obj.d == obj_from_json.d


def test_pickle() -> None:
    obj = ObjTest(a=1, b=2.0, c="3", d=True)
    obj_pickle = pickle.dumps(obj)
    obj_from_pickle = pickle.loads(obj_pickle)
    assert obj.a == obj_from_pickle.a
    assert obj.b == obj_from_pickle.b
    assert obj.c == obj_from_pickle.c
    assert obj.d == obj_from_pickle.d


def test_json_opt_0() -> None:
    obj = ObjTestOpt(1, 2.0, "3", True)
    obj_json = obj.json()
    obj_json_dict = json.loads(obj_json)
    assert obj_json_dict["type_keys"] == ["mlc.testing.serialize_opt", "int"]
    assert obj_json_dict["values"] == [
        "3",  # values[0] = literal: "3"
        [
            # values[1].type_index = 0 ===> ObjTestOpt
            0,
            # values[1].0
            [
                1,  # type_index = 1 ===> int
                1,  # value = 1
            ],
            # values[1].1 = literal: 2.0
            2.0,
            # values[1].2 = values[0]
            0,
            # values[1].3 = literal: True
            True,
        ],
    ]
    obj_from_json: ObjTestOpt = ObjTestOpt.from_json(obj_json)
    assert obj.a == obj_from_json.a
    assert obj.b == obj_from_json.b
    assert obj.c == obj_from_json.c
    assert obj.d == obj_from_json.d


def test_json_opt_1() -> None:
    obj = ObjTestOpt(None, None, None, None)
    obj_json = obj.json()
    obj_json_dict = json.loads(obj_json)
    assert obj_json_dict["type_keys"] == ["mlc.testing.serialize_opt"]
    assert obj_json_dict["values"] == [[0, None, None, None, None]]
    obj_from_json: ObjTestOpt = ObjTestOpt.from_json(obj_json)
    assert obj.a == obj_from_json.a
    assert obj.b == obj_from_json.b
    assert obj.c == obj_from_json.c
    assert obj.d == obj_from_json.d


def test_json_dag() -> None:
    lst = mlc.List([1, 2.0, "3", True])
    dct = mlc.Dict({"a": 1, "b": 2.0, "c": "3", "d": True, "v": lst})
    big_lst = mlc.List([lst, dct, lst, dct])
    obj_1 = AnyContainer([big_lst, big_lst])
    obj_2: AnyContainer = AnyContainer.from_json(obj_1.json())
    assert obj_2.field[0].is_(obj_2.field[1])
    assert obj_2.field[0] == big_lst
    big_lst = obj_2.field[0]
    assert big_lst[0].is_(big_lst[2])  # type: ignore[attr-defined]
    assert big_lst[1].is_(big_lst[3])  # type: ignore[attr-defined]
    assert big_lst[0] == lst
    assert big_lst[1] == dct
    lst, dct = big_lst[:2]  # type: ignore[assignment]
    assert dct["v"].is_(lst)  # type: ignore[attr-defined]
