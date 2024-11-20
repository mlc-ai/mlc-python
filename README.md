<h1 align="center">
  <img src="https://gist.githubusercontent.com/potatomashed/632c58cc8df7df7fdd067aabb34c1ef6/raw/7900472d1f7fce520fef5fad2e47a6f5fb234d08/mlc-python-logo.svg" alt="MLC Logo" style="width:10%; height:auto;">

  MLC-Python
</h1>

* [:key: Key features](#keykey-features)
* [:inbox_tray: Installation](#inbox_trayinstallation)
  + [:package: Install From PyPI](#packageinstall-from-pypi)
  + [:gear: Build from Source](#gearbuild-from-source)
  + [:ferris_wheel: Create MLC-Python Wheels](#ferris_wheel-create-mlc-python-wheels)

MLC is a Python-first toolkit that makes it more ergonomic to build AI compilers, runtimes, and compound AI systems. It provides Pythonic dataclasses with rich tooling infra, which includes:

- Structure-aware equality and hashing methods;
- Serialization in JSON / pickle;
- Text format printing and parsing in Python syntax.

Additionally, MLC language bindings support:

- Zero-copy bidirectional functioning calling for all MLC dataclasses.

## :key: Key features

TBD

## :inbox_tray: Installation

### :package: Install From PyPI

```bash
pip install -U mlc-python
```

### :gear: Build from Source

```bash
python -m venv .venv
source .venv/bin/activate
python -m pip install --verbose --editable ".[dev]"
pre-commit install
```

### :ferris_wheel: Create MLC-Python Wheels

This project uses `cibuildwheel` to build cross-platform wheels. See `.github/workflows/wheels.ym` for more details.

```bash
export CIBW_BUILD_VERBOSITY=3
export CIBW_BUILD="cp3*-manylinux_x86_64"
python -m pip install pipx
pipx run cibuildwheel==2.20.0 --output-dir wheelhouse
```
