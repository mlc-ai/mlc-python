import mlc


def test_object_init() -> None:
    obj = mlc.Object()
    assert str(obj).startswith("object.Object@0x")
