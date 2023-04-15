from typing import Optional

import mlc
import mlc.dataclasses as mlcd


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
