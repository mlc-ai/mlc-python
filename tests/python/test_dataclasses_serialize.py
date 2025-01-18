import json
import pickle
from typing import Optional

import mlc


@mlc.dataclasses.py_class("mlc.testing.serialize")
class ObjTest(mlc.PyClass):
    a: int
    b: float
    c: str
    d: bool


@mlc.dataclasses.py_class("mlc.testing.serialize_opt")
class ObjTestOpt(mlc.PyClass):
    a: Optional[int]
    b: Optional[float]
    c: Optional[str]
    d: Optional[bool]


def test_json() -> None:
    obj = ObjTest(1, 2.0, "3", True)
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
    obj = ObjTest(1, 2.0, "3", False)
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
    assert obj_json_dict["values"] == ["3", [0, [1, 1], 2.0, 0, True]]
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
