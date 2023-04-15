from mlc import Object


def test_object_init() -> None:
    obj = Object()
    assert str(obj).startswith("object.Object@0")
