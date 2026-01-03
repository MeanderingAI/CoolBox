# ML Core Python Bindings - Quick Installation Guide

## Quick Start

### 1. Prerequisites

Make sure you have the required dependencies:

```bash
# Install Python development headers (Ubuntu/Debian)
sudo apt-get install python3-dev python3-pip

# Install Eigen3 (Ubuntu/Debian)
sudo apt-get install libeigen3-dev

# Install Python packages
pip3 install numpy pybind11[global]
```

### 2. Build the Bindings

Navigate to the python_bindings directory and run:

```bash
cd python_bindings
./build.sh
```

Or manually:

```bash
python3 setup.py build_ext --inplace
```

### 3. Test the Installation

```bash
python3 test_bindings.py
```

### 4. Run Examples

```bash
python3 examples/decision_tree_example.py
python3 examples/svm_example.py
python3 examples/hmm_example.py
```

## Alternative Installation Methods

### Using CMake

```bash
cd python_bindings
mkdir build && cd build
cmake ..
make
```

### Install as Package

```bash
pip3 install .
```

## Troubleshooting

### Common Issues

1. **"pybind11 not found"**
   ```bash
   pip3 install pybind11[global]
   ```

2. **"Eigen3 not found"**
   ```bash
   sudo apt-get install libeigen3-dev
   # or specify EIGEN3_INCLUDE_DIR in CMake
   ```

3. **"ImportError: ml_core not found"**
   - Make sure you're running from the python_bindings directory
   - Check that the build completed successfully
   - Try rebuilding: `python3 setup.py build_ext --inplace`

4. **Compilation errors**
   - Ensure you have a C++17 compatible compiler
   - Check that all source files are accessible
   - Verify CMake version >= 3.12

### Platform-Specific Notes

**Ubuntu/Debian:**
```bash
sudo apt-get install python3-dev libeigen3-dev cmake
```

**macOS (with Homebrew):**
```bash
brew install eigen pybind11 cmake
```

**Windows:**
- Use Visual Studio with C++17 support
- Install Eigen3 and pybind11 via vcpkg or manually
- Ensure Python development libraries are available

## Project Structure

```
python_bindings/
├── py_ml_core.cpp          # Main binding definitions
├── setup.py                # Python build script
├── CMakeLists.txt          # CMake build script
├── requirements.txt        # Python dependencies
├── README.md               # Detailed documentation
├── build.sh               # Build automation script
├── test_bindings.py       # Comprehensive test suite
└── examples/              # Usage examples
    ├── decision_tree_example.py
    ├── svm_example.py
    ├── hmm_example.py
    ├── linear_regression_example.py
    └── bayesian_network_example.py
```