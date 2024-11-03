MLC-Python
==========

🛠️ MLC is a Python-first toolkit for building ML compilers, runtimes, and compound AI systems. It enables you to define nested data structures (like compiler IRs) as roundtrippable text formats in Python syntax, with structural comparison for unit-testing and zero-copy C++ interop when needed.

## 🔑 Key features

### 🐍 `mlc.ast`: Text formats in Python Syntax

TBD

### 🏗️ `mlc.dataclasses`: Cross-Language Dataclasses

TBD

### ⚡ `mlc.Func`: Zero-Copy Cross-Language Function Calling

TBD

### 🎯 Structural Testing for Nested Dataclasses

TBD

## 📥 Installation

### 📦 Install From PyPI

```bash
pip install -U mlc-python
```

### ⚙️ Build from Source

```bash
python -m venv .venv
source .venv/bin/activate
python -m pip install --verbose --editable ".[dev]"
pre-commit install
```

### 🎡 Create MLC-Python Wheels

This project uses `cibuildwheel` to build cross-platform wheels. See `.github/workflows/wheels.ym` for more details.

```bash
export CIBW_BUILD_VERBOSITY=3
export CIBW_BUILD="cp3*-manylinux_x86_64"
python -m pip install pipx
pipx run cibuildwheel==2.20.0 --output-dir wheelhouse
```
