<h1 align="center">
  <img src="https://gist.githubusercontent.com/potatomashed/632c58cc8df7df7fdd067aabb34c1ef6/raw/7900472d1f7fce520fef5fad2e47a6f5fb234d08/mlc-python-logo.svg" alt="MLC Logo" style="width:10%; height:auto;">

  MLC-Python
</h1>

* [:inbox_tray: Installation](#inbox_tray-installation)
* [:key: Key Features](#key-key-features)
  + [:building_construction: MLC Dataclass](#building_construction-mlc-dataclass)
  + [:dart: Structure-Aware Tooling](#dart-structure-aware-tooling)
  + [:snake: Text Formats in Python](#snake-text-formats-in-python)
  + [:zap: Zero-Copy Interoperability with C++ Plugins](#zap-zero-copy-interoperability-with-c-plugins)
* [:fuelpump: Development](#fuelpump-development)
  + [:gear: Editable Build](#gear-editable-build)
  + [:ferris_wheel: Create Wheels](#ferris_wheel-create-wheels)


MLC is a Python-first toolkit that streamlines the development of AI compilers, runtimes, and compound AI systems with its Pythonic dataclasses, structure-aware tooling, and Python-based text formats.

Beyond pure Python, MLC natively supports zero-copy interoperation with C++ plugins, and enables a smooth engineering practice transitioning from Python to hybrid or Python-free development.

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

**Type safety**. MLC dataclass enforces strict type checking using Cython and C++.

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

<details><summary> Define a toy IR with `structure`. </summary>

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

</details>

**Structural equality**. Member method `eq_s` compares the structural equality (alpha equivalence) of two IRs represented by MLC's structured dataclass.

```python
>>> x, y, z = Var("x"), Var("y"), Var("z")
>>> L1 = Let(rhs=x + y, lhs=z, body=z)  # let z = x + y; z
>>> L2 = Let(rhs=y + z, lhs=x, body=x)  # let x = y + z; x
>>> L3 = Let(rhs=x + x, lhs=z, body=z)  # let z = x + x; z
>>> L1.eq_s(L2)
True
>>> L1.eq_s(L3, assert_mode=True)
ValueError: Structural equality check failed at {root}.rhs.b: Inconsistent binding. RHS has been bound to a different node while LHS is not bound
```

**Structural hashing**. The structure of MLC dataclasses can be hashed via `hash_s`, which guarantees if two dataclasses are alpha-equivalent, they will share the same structural hash:

```python
>>> L1_hash, L2_hash, L3_hash = L1.hash_s(), L2.hash_s(), L3.hash_s()
>>> assert L1_hash == L2_hash
>>> assert L1_hash != L3_hash
```

### :snake: Text Formats in Python

**IR Printer.** By defining an `__ir_print__` method, which converts an IR node to MLC's Python-style AST, MLC's `IRPrinter` handles variable scoping, renaming and syntax highlighting automatically for a text format based on Python syntax.

<details><summary>Defining Python-based text format on a toy IR using `__ir_print__`.</summary>

```python
import mlc.dataclasses as mlcd
import mlc.printer as mlcp
from mlc.printer import ast as mlt

@mlcd.py_class
class Expr(mlcd.PyClass): ...

@mlcd.py_class
class Stmt(mlcd.PyClass): ...

@mlcd.py_class
class Var(Expr):
  name: str
  def __ir_print__(self, printer: mlcp.IRPrinter, path: mlcp.ObjectPath) -> mlt.Node:
    if not printer.var_is_defined(obj=self):
      printer.var_def(obj=self, frame=printer.frames[-1], name=self.name)
    return printer.var_get(obj=self)

@mlcd.py_class
class Add(Expr):
  lhs: Expr
  rhs: Expr
  def __ir_print__(self, printer: mlcp.IRPrinter, path: mlcp.ObjectPath) -> mlt.Node:
    lhs: mlt.Expr = printer(obj=self.lhs, path=path["a"])
    rhs: mlt.Expr = printer(obj=self.rhs, path=path["b"])
    return lhs + rhs

@mlcd.py_class
class Assign(Stmt):
  lhs: Var
  rhs: Expr
  def __ir_print__(self, printer: mlcp.IRPrinter, path: mlcp.ObjectPath) -> mlt.Node:
    rhs: mlt.Expr = printer(obj=self.rhs, path=path["b"])
    printer.var_def(obj=self.lhs, frame=printer.frames[-1], name=self.lhs.name)
    lhs: mlt.Expr = printer(obj=self.lhs, path=path["a"])
    return mlt.Assign(lhs=lhs, rhs=rhs)

@mlcd.py_class
class Func(mlcd.PyClass):
  name: str
  args: list[Var]
  stmts: list[Stmt]
  ret: Var
  def __ir_print__(self, printer: mlcp.IRPrinter, path: mlcp.ObjectPath) -> mlt.Node:
    with printer.with_frame(mlcp.DefaultFrame()):
      for arg in self.args:
        printer.var_def(obj=arg, frame=printer.frames[-1], name=arg.name)
      args: list[mlt.Expr] = [printer(obj=arg, path=path["args"][i]) for i, arg in enumerate(self.args)]
      stmts: list[mlt.Expr] = [printer(obj=stmt, path=path["stmts"][i]) for i, stmt in enumerate(self.stmts)]
      ret_stmt = mlt.Return(printer(obj=self.ret, path=path["ret"]))
      return mlt.Function(
        name=mlt.Id(self.name),
        args=[mlt.Assign(lhs=arg, rhs=None) for arg in args],
        decorators=[],
        return_type=None,
        body=[*stmts, ret_stmt],
      )

# An example IR:
a, b, c, d, e = Var("a"), Var("b"), Var("c"), Var("d"), Var("e")
f = Func(
  name="f",
  args=[a, b, c],
  stmts=[
    Assign(lhs=d, rhs=Add(a, b)),  # d = a + b
    Assign(lhs=e, rhs=Add(d, c)),  # e = d + c
  ],
  ret=e,
)
```

</details>

Two printer APIs are provided for Python-based text format:
- `mlc.printer.to_python` that converts an IR fragment to Python text, and
- `mlc.printer.print_python` that further renders the text with proper syntax highlighting.

```python
>>> print(mlcp.to_python(f))  # Stringify to Python
def f(a, b, c):
  d = a + b
  e = d + c
  return e
>>> mlcp.print_python(f)  # Syntax highlighting
```

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
