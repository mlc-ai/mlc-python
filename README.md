MLC-Python
==========

## Installation

```bash
pip install -U mlc-python
```

## Features

TBA

## Development

### Build from Source

```bash
python -m venv .venv
source .venv/bin/activate
python -m pip install --verbose --editable ".[dev]"
pre-commit install
```

### Create Wheels

See `.github/workflows/wheels.yml` for more details. This project uses `cibuildwheel` to build cross-platform wheels.

```bash
export CIBW_BUILD_VERBOSITY=3
export CIBW_BUILD="cp3*-manylinux_x86_64"
python -m pip install pipx
pipx run cibuildwheel==2.20.0 --output-dir wheelhouse
```
