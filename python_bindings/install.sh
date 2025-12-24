#!/bin/bash
# Quick installation script for ML ToolBox

set -e  # Exit on error

echo "========================================="
echo "  ML ToolBox - Installation Script"
echo "========================================="
echo ""

# Check Python version
python_version=$(python3 --version 2>&1 | awk '{print $2}')
echo "✓ Python version: $python_version"

# Check if pip is available
if ! command -v pip3 &> /dev/null; then
    echo "❌ pip3 not found. Please install pip first."
    exit 1
fi
echo "✓ pip3 found"

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "⚠ CMake not found. Installing via pip..."
    pip3 install cmake
else
    cmake_version=$(cmake --version | head -n1 | awk '{print $3}')
    echo "✓ CMake version: $cmake_version"
fi

# Check for C++ compiler
if command -v g++ &> /dev/null; then
    gpp_version=$(g++ --version | head -n1)
    echo "✓ G++ found: $gpp_version"
elif command -v clang++ &> /dev/null; then
    clang_version=$(clang++ --version | head -n1)
    echo "✓ Clang++ found: $clang_version"
else
    echo "❌ No C++ compiler found (g++ or clang++)"
    echo "   Please install a C++17 compatible compiler first"
    exit 1
fi

echo ""
echo "Installing ML ToolBox..."
echo ""

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Parse command line arguments
INSTALL_TYPE="${1:-editable}"
EXTRAS="${2:-}"

case "$INSTALL_TYPE" in
    "editable"|"dev"|"e")
        echo "Installing in EDITABLE mode (for development)..."
        if [ -n "$EXTRAS" ]; then
            pip3 install -e ".[$EXTRAS]"
        else
            pip3 install -e ".[examples]"
        fi
        ;;
    "normal"|"prod"|"n")
        echo "Installing in PRODUCTION mode..."
        if [ -n "$EXTRAS" ]; then
            pip3 install ".[$EXTRAS]"
        else
            pip3 install ".[examples]"
        fi
        ;;
    "wheel"|"w")
        echo "Building WHEEL package..."
        pip3 install build
        python3 -m build
        echo ""
        echo "Wheel created in dist/ directory"
        echo "Install with: pip3 install dist/ml_toolbox-*.whl"
        exit 0
        ;;
    *)
        echo "Usage: $0 [editable|normal|wheel] [extras]"
        echo ""
        echo "Install types:"
        echo "  editable (default) - Install in development mode (-e)"
        echo "  normal             - Install normally"
        echo "  wheel              - Build a wheel file"
        echo ""
        echo "Extras (optional):"
        echo "  examples  - Include matplotlib and other example dependencies"
        echo "  dev       - Include development tools (pytest, black, mypy)"
        echo "  all       - Include all optional dependencies"
        echo ""
        echo "Examples:"
        echo "  $0                    # Editable install with examples"
        echo "  $0 editable all       # Editable with all extras"
        echo "  $0 normal             # Normal install"
        echo "  $0 wheel              # Build wheel"
        exit 1
        ;;
esac

echo ""
echo "========================================="
echo "  Installation Complete!"
echo "========================================="
echo ""
echo "Verify the installation:"
echo "  python3 -c 'import ml_toolbox; print(ml_toolbox.__version__)'"
echo ""
echo "Run examples:"
echo "  python3 examples/llm_example.py"
echo "  python3 examples/distributed_training_demo.py"
echo ""
echo "Documentation:"
echo "  See README_INSTALL.md for detailed instructions"
echo ""
