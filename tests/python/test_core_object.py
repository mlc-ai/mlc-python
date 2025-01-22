import mlc


def test_object_init() -> None:
    obj = mlc.Object()
    assert str(obj).startswith("object.Object@0x")


def test_object_swap() -> None:
    a = mlc.Object()
    b = mlc.Object()
    a_addr = a._mlc_address
    b_addr = b._mlc_address
    a.swap(b)
    assert a._mlc_address == b_addr
    assert b._mlc_address == a_addr
