import mlc
import pytest


class MyType:
    def __init__(self, a: int) -> None:
        self.a = a


class MyTypeNotRegistered:
    def __init__(self, a: int) -> None:
        self.a = a


mlc.Opaque.register(MyType)


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
