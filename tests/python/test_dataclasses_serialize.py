import json
import pickle

import mlc


@mlc.dataclasses.py_class("mlc.testing.serialize")
class ObjTest(mlc.PyClass):
    a: int
    b: float
    c: str


def test_json() -> None:
    obj = ObjTest(1, 2.0, "3")
    obj_json = obj.json()
    obj_json_dict = json.loads(obj_json)
    assert obj_json_dict["type_keys"] == ["mlc.testing.serialize", "int"]
    assert obj_json_dict["values"] == ["3", [0, [1, 1], 2.0, 0]]
    obj_from_json = ObjTest.from_json(obj_json)
    assert obj.a == obj_from_json.a
    assert obj.b == obj_from_json.b
    assert obj.c == obj_from_json.c


def test_pickle() -> None:
    obj = ObjTest(1, 2.0, "3")
    obj_pickle = pickle.dumps(obj)
    obj_from_pickle = pickle.loads(obj_pickle)
    assert obj.a == obj_from_pickle.a
    assert obj.b == obj_from_pickle.b
    assert obj.c == obj_from_pickle.c
