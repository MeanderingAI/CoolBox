#!/bin/bash

# Build script for ML Core Python bindings

set -e  # Exit on any error

echo "Building ML Core Python Bindings"
echo "================================"

# Check if we're in the right directory
if [ ! -f "setup.py" ]; then
    echo "Error: setup.py not found. Please run this script from the python_bindings directory."
    exit 1
fi

# Check dependencies
echo "Checking dependencies..."

# Check for pybind11
python3 -c "import pybind11" 2>/dev/null || {
    echo "Installing pybind11..."
    pip3 install pybind11[global]
}

# Check for numpy
python3 -c "import numpy" 2>/dev/null || {
    echo "Installing numpy..."
    pip3 install numpy
}

# Check for Eigen3
if [ ! -d "/usr/include/eigen3" ] && [ ! -d "/usr/local/include/eigen3" ] && [ ! -d "../build/eigen-src" ]; then
    echo "Warning: Eigen3 not found in standard locations."
    echo "Make sure Eigen3 is installed or built in the parent project."
fi

echo "Dependencies checked."

# Build the extension
echo "Building Python extension..."

# Method 1: Using setup.py (recommended for development)
echo "Building with setup.py..."
python3 setup.py build_ext --inplace

# Method 2: Using pip install (for production)
# echo "Installing with pip..."
# pip3 install .

echo ""
echo "Build completed successfully!"
echo ""
echo "To test the bindings, run:"
echo "  python3 test_bindings.py"
echo ""
echo "To run examples:"
echo "  python3 examples/decision_tree_example.py"
echo "  python3 examples/svm_example.py"
echo "  python3 examples/hmm_example.py"
echo "  python3 examples/linear_regression_example.py"
echo "  python3 examples/bayesian_network_example.py"
echo ""
echo "To install the package:"
echo "  pip3 install ."