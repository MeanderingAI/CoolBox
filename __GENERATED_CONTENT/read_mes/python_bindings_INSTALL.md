# Installing ML ToolBox via pip

This guide explains how to install the ML ToolBox package using pip.

## Installation Methods

### Method 1: Install from Local Directory (Development)

If you're developing or testing locally:

```bash
cd /path/to/ToolBox/python_bindings
pip install -e .
```

The `-e` flag installs in "editable" mode, so changes to the code are immediately available.

### Method 2: Install from Local Directory (Production)

For a regular installation from local files:

```bash
cd /path/to/ToolBox/python_bindings
pip install .
```

### Method 3: Build a Wheel and Install

To create a distributable wheel file:

```bash
cd /path/to/ToolBox/python_bindings

# Build the wheel
python -m build

# Install the wheel
pip install dist/ml_toolbox-0.2.0-*.whl
```

### Method 4: Install with Optional Dependencies

Install with example dependencies (matplotlib, scikit-learn):

```bash
pip install -e ".[examples]"
```

Install with development tools:

```bash
pip install -e ".[dev]"
```

Install everything:

```bash
pip install -e ".[all]"
```

## Prerequisites

Before installing, ensure you have:

1. **C++ Compiler**: GCC 7+, Clang 5+, or MSVC 2017+
2. **CMake**: Version 3.12 or higher
3. **Python**: Version 3.7 or higher
4. **pip**: Latest version recommended

Install prerequisites:

```bash
# macOS
brew install cmake eigen

# Ubuntu/Debian
sudo apt-get install cmake libeigen3-dev g++

# Install Python build tools
pip install --upgrade pip setuptools wheel build
```

## Verifying Installation

After installation, verify it works:

```python
import ml_toolbox as ml
from ml_toolbox import deep_learning as dl

# Check version
print(ml.__version__)

# Create a simple model
model = dl.binary_classifier(input_dim=10)
print("✓ ML ToolBox installed successfully!")
```

## Using the Package

After installation, you can import it anywhere:

```python
import ml_toolbox as ml
from ml_toolbox import deep_learning as dl
from ml_toolbox import distributed

# Create an LLM
llm = dl.language_model(
    vocab_size=10000,
    context_length=512,
    embed_dim=256,
    num_heads=4,
    num_layers=4
)

# Use distributed training
trainer = distributed.DataParallelTrainer(num_workers=4)
```

## Uninstalling

To remove the package:

```bash
pip uninstall ml-toolbox
```

## Publishing to PyPI (For Maintainers)

To publish to PyPI:

```bash
# Install twine
pip install twine

# Build the distribution
python -m build

# Upload to PyPI (requires account)
twine upload dist/*

# Or upload to TestPyPI first
twine upload --repository testpypi dist/*
```

Then users can install with:

```bash
pip install ml-toolbox
```

## Troubleshooting

### Build Errors

If you encounter build errors:

1. **Check compiler**: Ensure you have a C++17 compatible compiler
   ```bash
   g++ --version  # Should be 7.0+
   clang++ --version  # Should be 5.0+
   ```

2. **Check CMake**: 
   ```bash
   cmake --version  # Should be 3.12+
   ```

3. **Check Eigen**: Make sure Eigen is installed or will be downloaded by CMake

4. **Clean build**:
   ```bash
   rm -rf build dist *.egg-info
   pip install -e . --no-cache-dir
   ```

### Import Errors

If `import ml_toolbox` fails:

1. Check installation:
   ```bash
   pip list | grep ml-toolbox
   ```

2. Check the extension was built:
   ```bash
   python -c "import ml_toolbox; print(ml_toolbox.__file__)"
   ```

3. Rebuild:
   ```bash
   pip install -e . --force-reinstall --no-cache-dir
   ```

### Linking Errors

If you get linking errors related to Eigen or other libraries:

1. **Set include paths** (if Eigen is in a non-standard location):
   ```bash
   export CPLUS_INCLUDE_PATH=/usr/local/include/eigen3:$CPLUS_INCLUDE_PATH
   pip install -e .
   ```

2. **Use system Eigen**:
   ```bash
   # macOS
   brew install eigen
   
   # Linux
   sudo apt-get install libeigen3-dev
   ```

## Development Workflow

For active development:

```bash
# Clone the repository
git clone https://github.com/yourusername/ToolBox.git
cd ToolBox/python_bindings

# Create a virtual environment
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate

# Install in editable mode with dev dependencies
pip install -e ".[dev,examples]"

# Make changes to C++ code, then rebuild
pip install -e . --force-reinstall --no-cache-dir

# Run tests
python test_bindings.py

# Run examples
python examples/llm_example.py
```

## Platform-Specific Notes

### macOS
- Use Homebrew for dependencies: `brew install cmake eigen`
- May need Xcode Command Line Tools: `xcode-select --install`

### Linux
- Use package manager: `apt-get install cmake libeigen3-dev g++`
- Ensure you have Python development headers: `apt-get install python3-dev`

### Windows
- Install Visual Studio 2017 or later with C++ tools
- Install CMake from cmake.org
- May need to use CMake directly instead of setuptools

## Examples

After installation, try the examples:

```bash
# Copy examples to your working directory
cp -r /path/to/ToolBox/examples .

# Run examples
python examples/llm_example.py
python examples/distributed_training_demo.py
```

## Support

For issues and questions:
- GitHub Issues: https://github.com/yourusername/ToolBox/issues
- Documentation: See README.md and inline docstrings
