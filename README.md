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

**Printer.** MLC converts an IR node to Python AST by looking up the `__ir_print__` method.

**[[Example](https://github.com/mlc-ai/mlc-python/blob/main/python/mlc/testing/toy_ir/ir.py)]**. Copy the toy IR definition to REPL and then create a `Func` node below:

```python
>>> a, b, c, d, e = Var("a"), Var("b"), Var("c"), Var("d"), Var("e")
>>> f = Func("f", [a, b, c],
  stmts=[
    Assign(lhs=d, rhs=Add(a, b)),  # d = a + b
    Assign(lhs=e, rhs=Add(d, c)),  # e = d + c
  ],
  ret=e)
```

- Method `mlc.printer.to_python` converts an IR node to Python-based text;

```python
>>> print(mlcp.to_python(f))  # Stringify to Python
def f(a, b, c):
  d = a + b
  e = d + c
  return e
```

- Method `mlc.printer.print_python` further renders the text with proper syntax highlighting. [[Screenshot](https://raw.githubusercontent.com/gist/potatomashed/5a9b20edbdde1b9a91a360baa6bce9ff/raw/3c68031eaba0620a93add270f8ad7ed2c8724a78/mlc-python-printer.svg)]

```python
>>> mlcp.print_python(f)  # Syntax highlighting
```

**AST Parser.** MLC has a concise set of APIs for implementing parser with Python's AST module, including:
- Inspection API that obtains source code of a Python class or function and the variables they capture;
- Variable management APIs that help with proper scoping;
- AST fragment evaluation APIs;
- Error rendering APIs.

**[[Example](https://github.com/mlc-ai/mlc-python/blob/main/python/mlc/testing/toy_ir/parser.py)]**. With MLC APIs, a parser can be implemented with 100 lines of code for the Python text format above defined by `__ir_printer__`.

### :zap: Zero-Copy Interoperability with C++ Plugins

🚧 Under construction.

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
