from mlc.testing import ReflectionTestObj  # type: ignore[import-not-found]


def test_mutable() -> None:
    obj = ReflectionTestObj("hello", 42)
    assert obj.x_mutable == "hello"
    assert obj.y_immutable == 42
    obj.x_mutable = "world"
    assert obj.x_mutable == "world"


def test_immutable() -> None:
    obj = ReflectionTestObj("hello", 42)
    assert obj.y_immutable == 42
    try:
        obj.y_immutable = 43
        assert False
    except AttributeError:
        pass


def test_method() -> None:
    obj = ReflectionTestObj("hello", 42)
    assert obj.YPlusOne() == 43
