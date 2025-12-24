# ML ToolBox - Quick Installation Reference

## TL;DR - Quick Install

```bash
cd /path/to/ToolBox/python_bindings
./install.sh
```

Or manually:

```bash
pip install -e ".[examples]"
```

---

## Installation Methods

### 1. **Editable Install** (Recommended for Development)

Changes to Python/C++ code take effect after rebuild:

```bash
cd python_bindings
pip install -e .
```

With examples (matplotlib, etc.):

```bash
pip install -e ".[examples]"
```

With dev tools:

```bash
pip install -e ".[dev]"
```

Everything:

```bash
pip install -e ".[all]"
```

### 2. **Normal Install** (For End Users)

```bash
cd python_bindings
pip install .
```

### 3. **From Wheel** (For Distribution)

Build the wheel:

```bash
cd python_bindings
python -m build
```

Install it:

```bash
pip install dist/ml_toolbox-0.2.0-*.whl
```

Share the wheel file with others!

### 4. **From PyPI** (Future - After Publishing)

```bash
pip install ml-toolbox
```

---

## After Installation

Test it works:

```python
import ml_toolbox as ml
from ml_toolbox import deep_learning as dl

print(ml.__version__)  # Should print 0.2.0

# Create an LLM
llm = dl.language_model(vocab_size=10000, context_length=512)
print("✓ Success!")
```

Run examples:

```bash
cd examples
python llm_example.py
python distributed_training_demo.py
```

---

## Prerequisites

Install before building:

**macOS:**
```bash
brew install cmake eigen
pip install pybind11 numpy
```

**Linux:**
```bash
sudo apt-get install cmake libeigen3-dev g++ python3-dev
pip install pybind11 numpy
```

---

## Rebuilding After Changes

If you modify C++ code:

```bash
pip install -e . --force-reinstall --no-cache-dir
```

Or use the build script:

```bash
./build.sh
```

---

## Uninstall

```bash
pip uninstall ml-toolbox
```

---

## Project Structure After Install

```
ml_toolbox/              # Python package
  __init__.py            # Main module
  ml_core.*.so           # Compiled C++ extension
  py.typed               # Type hints marker

examples/                # Example scripts
  llm_example.py
  distributed_training_demo.py
  templates_demo.py

README_INSTALL.md        # Detailed guide
```

---

## Common Issues

**Build fails:**
```bash
# Clean and retry
rm -rf build dist *.egg-info
pip install -e . --no-cache-dir
```

**Import fails:**
```bash
# Check if installed
pip list | grep ml-toolbox

# Reinstall
pip install -e . --force-reinstall
```

**Missing compiler:**
```bash
# Install build tools
# macOS: xcode-select --install
# Linux: sudo apt-get install build-essential
```

---

## Publishing to PyPI (Maintainers Only)

```bash
# Install tools
pip install build twine

# Build
python -m build

# Test on TestPyPI
twine upload --repository testpypi dist/*

# Upload to PyPI
twine upload dist/*
```

---

## Help

For detailed instructions, see [README_INSTALL.md](README_INSTALL.md)

For issues: https://github.com/yourusername/ToolBox/issues
