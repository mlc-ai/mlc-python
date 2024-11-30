<h1 align="center">
  <img src="https://gist.githubusercontent.com/potatomashed/632c58cc8df7df7fdd067aabb34c1ef6/raw/7900472d1f7fce520fef5fad2e47a6f5fb234d08/mlc-python-logo.svg" alt="MLC Logo" style="width:10%; height:auto;">

  MLC-Python
</h1>

* [:inbox_tray: Installation](#inbox_tray-installation)
* [:key: Key Features](#key-key-features)
  + [:building_construction: MLC Dataclass](#building_construction-mlc-dataclass)
  + [:dart: Structure-Aware Tooling](#dart-structure-aware-tooling)
  + [:snake: Text Formats in Python AST](#snake-text-formats-in-python-ast)
  + [:zap: Zero-Copy Interoperability with C++ Plugins](#zap-zero-copy-interoperability-with-c-plugins)
* [:fuelpump: Development](#fuelpump-development)
  + [:gear: Editable Build](#gear-editable-build)
  + [:ferris_wheel: Create Wheels](#ferris_wheel-create-wheels)


MLC is a Python-first toolkit that makes it more ergonomic to build AI compilers, runtimes, and compound AI systems with Pythonic dataclass, rich tooling infra and zero-copy interoperability with C++ plugins.

## :inbox_tray: Installation

```bash
pip install -U mlc-python
```

## :key: Key Features

### :building_construction: MLC Dataclass

MLC dataclass is similar to Python’s native dataclass:

```python
import mlc.dataclasses as mlcd

@mlcd.py_class("demo.MyClass")
class MyClass(mlcd.PyClass):
  a: int
  b: str
  c: float | None

instance = MyClass(12, "test", c=None)
```

**Type safety**. MLC dataclass checks type strictly in Cython and C++.

```python
>>> instance.c = 10; print(instance)
demo.MyClass(a=12, b='test', c=10.0)

>>> instance.c = "wrong type"
TypeError: must be real number, not str

>>> instance.non_exist = 1
AttributeError: 'MyClass' object has no attribute 'non_exist' and no __dict__ for setting new attributes
```

**Serialization**. MLC dataclasses are picklable and JSON-serializable.

```python
>>> MyClass.from_json(instance.json())
demo.MyClass(a=12, b='test', c=None)

>>> import pickle; pickle.loads(pickle.dumps(instance))
demo.MyClass(a=12, b='test', c=None)
```

### :dart: Structure-Aware Tooling

An extra `structure` field are used to specify a dataclass's structure, indicating def site and scoping in an IR.

```python
import mlc.dataclasses as mlcd

@mlcd.py_class
class Expr(mlcd.PyClass):
  def __add__(self, other):
    return Add(a=self, b=other)

@mlcd.py_class(structure="nobind")
class Add(Expr):
  a: Expr
  b: Expr

@mlcd.py_class(structure="var")
class Var(Expr):
  name: str = mlcd.field(structure=None) # excludes `name` from defined structure

@mlcd.py_class(structure="bind")
class Let(Expr):
  rhs: Expr
  lhs: Var = mlcd.field(structure="bind") # `Let.lhs` is the def-site
  body: Expr
```

**Structural equality**. Method eq_s is ready to use to compare the structural equality (alpha equivalence) of two IRs.

```python
"""
L1: let z = x + y; z
L2: let x = y + z; x
L3: let z = x + x; z
"""
>>> x, y, z = Var("x"), Var("y"), Var("z")
>>> L1 = Let(rhs=x + y, lhs=z, body=z)
>>> L2 = Let(rhs=y + z, lhs=x, body=x)
>>> L3 = Let(rhs=x + x, lhs=z, body=z)
>>> L1.eq_s(L2)
True
>>> L1.eq_s(L3, assert_mode=True)
ValueError: Structural equality check failed at {root}.rhs.b: Inconsistent binding. RHS has been bound to a different node while LHS is not bound
```

**Structural hashing**. TBD

### :snake: Text Formats in Python AST

TBD

### :zap: Zero-Copy Interoperability with C++ Plugins

TBD

## :fuelpump: Development

### :gear: Editable Build

```bash
pip install --verbose --editable ".[dev]"
pre-commit install
```

### :ferris_wheel: Create Wheels

This project uses `cibuildwheel` to build cross-platform wheels. See `.github/workflows/wheels.ym` for more details.

```bash
export CIBW_BUILD_VERBOSITY=3
export CIBW_BUILD="cp3*-manylinux_x86_64"
python -m pip install pipx
pipx run cibuildwheel==2.20.0 --output-dir wheelhouse
```
