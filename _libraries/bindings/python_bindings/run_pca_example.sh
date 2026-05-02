#!/bin/bash

# Quick Start Guide for PCA Example with Visualization
# ======================================================

echo "Setting up PCA visualization example..."
echo ""

# Step 1: Build the Python bindings
echo "Step 1: Building Python bindings..."
cd python_bindings
python3 setup.py build_ext --inplace

if [ $? -ne 0 ]; then
    echo "Error: Build failed. Make sure you have:"
    echo "  - Python 3.6+"
    echo "  - pybind11"
    echo "  - Eigen3"
    echo "  - numpy"
    exit 1
fi

# Step 2: Install matplotlib if not present
echo ""
echo "Step 2: Checking for matplotlib..."
python3 -c "import matplotlib" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "matplotlib not found. Installing..."
    pip3 install matplotlib
fi

# Step 3: Run the example
echo ""
echo "Step 3: Running PCA visualization example..."
echo "This will generate several plots showing:"
echo "  - 2D data visualization"
echo "  - Scree plots"
echo "  - Scaling effects"
echo "  - 3D projections"
echo "  - Component loadings"
echo "  - Reconstruction error analysis"
echo ""

python3 examples/example_pca_visualization.py

if [ $? -eq 0 ]; then
    echo ""
    echo "Success! Check the generated PNG files in the current directory."
    echo ""
    echo "Generated files:"
    ls -lh *.png 2>/dev/null | awk '{print "  - " $9}'
else
    echo ""
    echo "Error running example. Make sure ml_core module is built correctly."
fi
