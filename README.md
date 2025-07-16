<h1 align="center">
  <img src="https://gist.githubusercontent.com/potatomashed/632c58cc8df7df7fdd067aabb34c1ef6/raw/7900472d1f7fce520fef5fad2e47a6f5fb234d08/mlc-python-logo.svg" alt="MLC Logo" style="width:10%; height:auto;">

  MLC-Python
</h1>
<p align="center"> Python-first Development for AI Compilers. </p>

* [:inbox_tray: Installation](#inbox_tray-installation)
* [:key: Key Features](#key-key-features)
  + [:building_construction: Define IRs with MLC Dataclasses](#building_construction-define-irs-with-mlc-dataclasses)
  + [:snake: Design Python-based Text Formats for IRs](#snake-design-python-based-text-formats-for-irs)
  + [:dart: Test IRs with MLC Structure-Aware Tooling](#dart-test-irs-with-mlc-structure-aware-tooling)
  + [:zap: Migrate Gradually to C++ with MLC Plugins](#zap-migrate-gradually-to-c-with-mlc-plugins)
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

### :building_construction: Define IRs with MLC Dataclasses

MLC provides Pythonic dataclasses:

```python
import mlc.dataclasses as mlcd

@mlcd.py_class("demo.MyClass")
class MyClass:
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
>>> mlc.json_loads(mlc.json_dumps(instance))
demo.MyClass(a=12, b='test', c=None)

>>> pickle.loads(pickle.dumps(instance))
demo.MyClass(a=12, b='test', c=None)
```

### :snake: Design Python-based Text Formats for IRs

**Printer.** MLC looks up method `__ir_print__` to convert IR nodes to Python AST:

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

### :dart: Test IRs with MLC Structure-Aware Tooling

By annotating IR definitions with `structure`, MLC supports structural equality and structural hashing to detect structural equivalence between IRs:

<details><summary> Define a toy IR with `structure`. </summary>

```python
import mlc
import mlc.dataclasses as mlcd

@mlcd.py_class
class Expr:
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
>>> mlc.eq_s(L1, L2)
True
>>> mlc.eq_s(L1, L3, assert_mode=True)
ValueError: Structural equality check failed at {root}.rhs.b: Inconsistent binding. RHS has been bound to a different node while LHS is not bound
```

**Structural hashing**. The structure of MLC dataclasses can be hashed via `hash_s`, which guarantees if two dataclasses are alpha-equivalent, they will share the same structural hash:

```python
>>> L1_hash, L2_hash, L3_hash = mlc.hash_s(L1), mlc.hash_s(L2), mlc.hash_s(L3)
>>> assert L1_hash == L2_hash
>>> assert L1_hash != L3_hash
```

### :zap: Migrate Gradually to C++ with MLC Plugins

(🚧 Under construction)

MLC seamlessly supports zero-copy bidirectional interoperabilty with C++ plugins with no extra dependency. By gradually migrating classes and methods one at a time, a pure Python prototype can be transitioned to hybrid or Python-free development.

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
pipx run cibuildwheel==2.22.0 --output-dir wheelhouse
```
