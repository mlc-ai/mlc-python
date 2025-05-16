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


mlc.Opaque.register(MyType)


@mlc.dataclasses.py_class(structure="bind")
class Wrapper(mlc.dataclasses.PyClass):
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


@mlc.Func.register("Opaque.eq_s.test_core_opaque.MyType")
def _eq_s_MyType(a: MyType, b: MyType) -> bool:
    return isinstance(a, MyType) and isinstance(b, MyType) and a.a == b.a


@mlc.Func.register("Opaque.hash_s.test_core_opaque.MyType")
def _hash_s_MyType(a: MyType) -> int:
    assert isinstance(a, MyType)
    return hash((MyType, a.a))


def test_opaque_dataclass_eq_s() -> None:
    a1 = Wrapper(field=MyType(a=10))
    a2 = Wrapper(field=MyType(a=10))
    a1.eq_s(a2, assert_mode=True)


def test_opaque_dataclass_eq_s_fail() -> None:
    a1 = Wrapper(field=MyType(a=10))
    a2 = Wrapper(field=MyType(a=20))
    with pytest.raises(ValueError) as exc_info:
        a1.eq_s(a2, assert_mode=True)
    assert str(exc_info.value).startswith("Structural equality check failed at {root}.field")


def test_opaque_dataclass_hash_s() -> None:
    a1 = Wrapper(field=MyType(a=10))
    assert isinstance(a1.hash_s(), int)
