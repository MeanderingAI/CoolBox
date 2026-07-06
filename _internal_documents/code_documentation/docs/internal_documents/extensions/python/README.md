# Python Extension

## Location

- `_libraries/python_bindings`

## Surface

- Documentation namespace: `coolbox::python`
- `pybind11` extension module exposed through `ml_toolbox.ml_core`
- Covers machine learning algorithms, time series, NLP, distributed helpers, and REST API bindings
- The published Python package name remains `ml-toolbox`

## Installation

From the repository root:

```bash
cd _libraries/python_bindings
pip install -e .
```

To install optional extras:

```bash
cd _libraries/python_bindings
pip install -e ".[examples]"
pip install -e ".[dev]"
```

## Setup And Build

Quick build:

```bash
make build_python_bindings
```

Direct build:

```bash
cd _libraries/python_bindings
python -m build
```

For an in-place extension build during development:

```bash
cd _libraries/python_bindings
python3 setup.py build_ext --inplace
```

## Native Library Setup Notes

- `setup.py` first tries to reuse prebuilt native CoolBox libraries.
- It searches `COOLBOX_LIB_DIR` recursively, so nested build artifact layouts are supported.
- If `charts` or `wave_generator_utils` are unavailable in the staged build tree, the extension falls back to the vendored `graphics.cpp` and `wave_generator.cpp` sources.

Useful overrides:

```bash
COOLBOX_LIB_DIR=/path/to/build python3 setup.py build_ext --inplace
COOLBOX_LIBS=charts,wave_generator_utils python3 setup.py build_ext --inplace
```

## Validation

```bash
cd _libraries/python_bindings
python3 test_bindings.py
```

## Packaging Notes

- Release page: https://github.com/MeanderingAI/CoolBox/releases
- Release workflows publish `python-extension-*` artifacts.
- Docs publishing restores prebuilt wheels, sdists, and reusable binary outputs from those artifacts.