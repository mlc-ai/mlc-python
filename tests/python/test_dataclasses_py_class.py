from typing import Optional

import mlc


@mlc.py_class("mlc.testing.py_class_base")
class Base(mlc.PyClass):
    base_a: int
    base_b: str


@mlc.py_class("mlc.testing.py_class_derived")
class Derived(Base):
    derived_a: float
    derived_b: Optional[str]


def test_base() -> None:
    base = Base(1, "a")
    base_str = "mlc.testing.py_class_base(base_a=1, base_b='a')"
    assert base.base_a == 1
    assert base.base_b == "a"
    assert str(base) == base_str
    assert repr(base) == base_str


def test_derived() -> None:
    derived = Derived(1.0, "b", 2, "c")
    derived_str = "mlc.testing.py_class_derived(base_a=1, base_b='b', derived_a=2.0, derived_b='c')"
    assert derived.base_a == 1
    assert derived.base_b == "b"
    assert derived.derived_a == 2.0
    assert derived.derived_b == "c"
    assert str(derived) == derived_str
    assert repr(derived) == derived_str


def test_repr_in_list() -> None:
    target = mlc.List[Base](
        [
            Base(1, "a"),
            Derived(1.0, "b", 2, "c"),
        ],
    )
    target_str_0 = "mlc.testing.py_class_base(base_a=1, base_b='a')"
    target_str_1 = (
        "mlc.testing.py_class_derived(base_a=1, base_b='b', derived_a=2.0, derived_b='c')"
    )
    target_str = f"[{target_str_0}, {target_str_1}]"
    assert str(target) == target_str
    assert repr(target) == target_str
