from mlc import Func


def test_func_init() -> None:
    func = Func(lambda x: x + 1)
    assert func(1) == 2
    assert str(func).startswith("object.Func@0x")
