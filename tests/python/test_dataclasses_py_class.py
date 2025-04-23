from typing import Optional

import mlc
import mlc.dataclasses as mlcd
import pytest


@mlcd.py_class("mlc.testing.py_class_base")
class Base(mlcd.PyClass):
    base_a: int
    base_b: str

    @mlcd.vtable_method(is_static=True)
    @staticmethod
    def static_method(a: int, b: int) -> int:
        return a + b

    @mlcd.vtable_method(is_static=False)
    def fake_repr(self) -> str:
        return str(self.base_a) + self.base_b


@mlcd.py_class("mlc.testing.py_class_derived")
class Derived(Base):
    derived_a: float
    derived_b: Optional[str]


@mlcd.py_class("mlc.testing.py_class_base_with_default")
class BaseWithDefault(mlcd.PyClass):
    base_a: int
    base_b: list[int] = mlcd.field(default_factory=list)


@mlcd.py_class("mlc.testing.py_class_derived_with_default")
class DerivedWithDefault(BaseWithDefault):
    derived_a: Optional[int] = None
    derived_b: Optional[str] = "1234"


@mlcd.py_class("mlc.testing.DerivedDerived")
class DerivedDerived(DerivedWithDefault):
    derived_derived_a: str


@mlcd.py_class("mlc.testing.py_class_derived_with_default_interleaved")
class DerivedWithDefaultInterleaved(BaseWithDefault):
    derived_a: int
    derived_b: Optional[str] = "1234"


@mlcd.py_class("mlc.testing.py_class_post_init")
class PostInit(mlcd.PyClass):
    a: int
    b: str

    def __post_init__(self) -> None:
        self.b = self.b.upper()


@mlcd.py_class("mlc.testing.py_class_frozen", frozen=True)
class Frozen(mlcd.PyClass):
    a: int
    b: str


@mlcd.py_class
class ContainerFields(mlcd.PyClass):
    a: list[int]
    b: dict[int, int]


@mlcd.py_class(frozen=True)
class FrozenContainerFields(mlcd.PyClass):
    a: list[int]
    b: dict[int, int]


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


def test_default_in_derived() -> None:
    derived = DerivedWithDefault(12)
    derived_str = "mlc.testing.py_class_derived_with_default(base_a=12, base_b=[], derived_a=None, derived_b='1234')"
    assert derived.base_a == 12
    assert isinstance(derived.base_b, mlc.List) and len(derived.base_b) == 0
    assert derived.derived_a is None
    assert derived.derived_b == "1234"
    assert str(derived) == derived_str
    assert repr(derived) == derived_str


def test_default_in_derived_interleaved() -> None:
    derived = DerivedWithDefaultInterleaved(12, 34)
    assert derived.base_a == 12
    assert isinstance(derived.base_b, mlc.List) and len(derived.base_b) == 0
    assert derived.derived_a == 34
    assert derived.derived_b == "1234"


def test_post_init() -> None:
    post_init = PostInit(1, "a")
    assert post_init.a == 1
    assert post_init.b == "A"
    assert str(post_init) == "mlc.testing.py_class_post_init(a=1, b='A')"
    assert repr(post_init) == "mlc.testing.py_class_post_init(a=1, b='A')"


def test_frozen_set_fail() -> None:
    frozen = Frozen(1, "a")
    with pytest.raises(AttributeError) as e:
        frozen.a = 2
    # depends on Python version, there are a few possible error messages
    assert str(e.value) in [
        "property 'a' of 'Frozen' object has no setter",
        "can't set attribute",
    ]
    assert frozen.a == 1
    assert frozen.b == "a"


def test_frozen_force_set() -> None:
    frozen = Frozen(1, "a")
    frozen._mlc_setattr("a", 2)
    assert frozen.a == 2
    assert frozen.b == "a"

    frozen._mlc_setattr("b", "b")
    assert frozen.a == 2
    assert frozen.b == "b"


def test_derived_derived() -> None:
    # __init__(base_a, derived_derived_a, base_b, derived_a, derived_b)
    obj = DerivedDerived(1, "a", [1, 2], 2, "b")
    assert obj.base_a == 1
    assert obj.derived_derived_a == "a"
    assert isinstance(obj.base_b, mlc.List) and len(obj.base_b) == 2
    assert obj.base_b[0] == 1
    assert obj.base_b[1] == 2
    assert obj.derived_a == 2
    assert obj.derived_b == "b"
    assert str(obj) == (
        "mlc.testing.DerivedDerived(base_a=1, base_b=[1, 2], derived_a=2, derived_b='b', derived_derived_a='a')"
    )


def test_container_fields() -> None:
    obj_0 = ContainerFields([1, 2], {1: 2})
    assert obj_0.a == [1, 2]
    assert obj_0.b == {1: 2}

    obj_1 = ContainerFields(a=obj_0.a, b=obj_0.b)
    assert obj_1.a == obj_0.a
    assert obj_1.b == obj_0.b


def test_frozen_container_fields() -> None:
    obj = FrozenContainerFields([1, 2], {1: 2})
    assert obj.a == [1, 2]
    assert obj.b == {1: 2}

    assert obj.a.frozen
    assert obj.b.frozen

    e: pytest.ExceptionInfo
    with pytest.raises(AttributeError) as e:
        obj.a = [2, 3]
    assert str(e.value) in [
        "property 'a' of 'FrozenContainerFields' object has no setter",
        "can't set attribute",
    ]

    with pytest.raises(AttributeError) as e:
        obj.b = {2: 3}
    assert str(e.value) in [
        "property 'b' of 'FrozenContainerFields' object has no setter",
        "can't set attribute",
    ]

    with pytest.raises(RuntimeError) as e:
        obj.a[0] = 3
    assert str(e.value) == "Cannot modify a frozen list"

    with pytest.raises(RuntimeError) as e:
        obj.b[0] = 3
    assert str(e.value) == "Cannot modify a frozen dict"
