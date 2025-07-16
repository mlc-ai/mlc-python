import copy
import json
from typing import Any

import mlc
import pytest


class MyTypeNotRegistered:
    def __init__(self, a: int) -> None:
        self.a = a


class MyType:
    def __init__(self, a: int) -> None:
        self.a = a

    def __call__(self, x: int) -> int:
        return x + self.a

    def __eq__(self, other: Any) -> bool:
        return isinstance(self, MyType) and isinstance(other, MyType) and self.a == other.a

    def __hash__(self) -> int:
        assert isinstance(self, MyType)
        return hash((MyType, self.a))


mlc.Opaque.register(MyType)


@mlc.dataclasses.py_class(structure="bind")
class Wrapper:
    field: Any = mlc.dataclasses.field(structure="nobind")


def test_opaque_init() -> None:
    a = MyType(a=10)
    opaque = mlc.Opaque(a)
    assert str(opaque) == "<Opaque `test_core_opaque.MyType`>"


def test_opaque_init_error() -> None:
    a = MyTypeNotRegistered(a=10)
    with pytest.raises(TypeError) as e:
        mlc.Opaque(a)
    assert (
        str(e.value)
        == "MLC does not recognize type: <class 'test_core_opaque.MyTypeNotRegistered'>. "
        "If it's intentional, please register using `mlc.Opaque.register(type)`."
    )


def test_opaque_ffi() -> None:
    func = mlc.Func.get("mlc.testing.cxx_obj")
    a = func(MyType(a=10))
    assert isinstance(a, MyType)
    assert a.a == 10


def test_opaque_ffi_error() -> None:
    func = mlc.Func.get("mlc.testing.cxx_obj")
    with pytest.raises(TypeError) as e:
        func(MyTypeNotRegistered(a=10))
    assert (
        str(e.value)
        == "MLC does not recognize type: <class 'test_core_opaque.MyTypeNotRegistered'>"
    )


def test_opaque_dataclass() -> None:
    a = MyType(a=10)
    wrapper = Wrapper(field=a)
    assert isinstance(wrapper.field, MyType)
    assert wrapper.field.a == 10


def test_opaque_dataclass_eq_s() -> None:
    a1 = Wrapper(field=MyType(a=10))
    a2 = Wrapper(field=MyType(a=10))
    mlc.eq_s(a1, a2, assert_mode=True)


def test_opaque_dataclass_eq_s_fail() -> None:
    a1 = Wrapper(field=MyType(a=10))
    a2 = Wrapper(field=MyType(a=20))
    with pytest.raises(ValueError) as exc_info:
        mlc.eq_s(a1, a2, assert_mode=True)
    assert str(exc_info.value).startswith("Structural equality check failed at {root}.field")


def test_opaque_dataclass_hash_s() -> None:
    a1 = Wrapper(field=MyType(a=10))
    assert isinstance(mlc.hash_s(a1), int)


def test_opaque_serialize() -> None:
    obj_1 = Wrapper(field=MyType(a=10))
    json_str = mlc.json_dumps(obj_1)
    js = json.loads(json_str)
    assert js["opaques"] == '[{"py/object": "test_core_opaque.MyType", "a": 10}]'
    assert js["values"] == [[0, 0], [1, 0]]
    assert js["type_keys"] == ["mlc.core.Opaque", "test_core_opaque.Wrapper"]
    obj_2 = mlc.json_loads(json_str)
    assert isinstance(obj_2.field, MyType)
    assert obj_2.field.a == 10


def test_opaque_serialize_with_alias() -> None:
    a1 = MyType(a=10)
    a2 = MyType(a=20)
    a3 = MyType(a=30)
    obj_1 = Wrapper(field=[a1, a2, a3, a3, a2, a1])
    obj_2 = mlc.json_loads(mlc.json_dumps(obj_1))
    assert obj_2.field[0] is obj_2.field[5]
    assert obj_2.field[1] is obj_2.field[4]
    assert obj_2.field[2] is obj_2.field[3]
    assert obj_2.field[0].a == 10
    assert obj_2.field[1].a == 20
    assert obj_2.field[2].a == 30
    assert obj_2.field[3].a == 30
    assert obj_2.field[4].a == 20
    assert obj_2.field[5].a == 10


def test_opaque_deepcopy() -> None:
    a = MyType(a=10)
    obj_1 = Wrapper(field=a)
    obj_2 = copy.deepcopy(obj_1)
    assert isinstance(obj_2.field, MyType)
    assert obj_2.field.a == 10
    assert obj_1 is not obj_2
    assert obj_1.field is not obj_2.field
