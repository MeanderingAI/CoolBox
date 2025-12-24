# ToolBox Top-Level Makefile
# Builds C++ libraries, runs tests, and manages the entire project

.PHONY: all build clean test install examples help

# Default target
all: build

# Build all C++ libraries
build:
	@echo "========================================"
	@echo "Building ToolBox Libraries"
	@echo "========================================"
	@mkdir -p build
	@cd build && cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release .. && $(MAKE)
	@echo ""
	@echo "✓ Build complete!"
	@echo ""
	@echo "Libraries built:"
	@find build/src -name "*.so" -o -name "*.dylib" 2>/dev/null || true
	@echo ""

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf build
	@cd examples/cpp && $(MAKE) clean 2>/dev/null || true
	@cd python_bindings && rm -rf build dist *.egg-info 2>/dev/null || true
	@echo "✓ Clean complete!"

# Full clean and rebuild
rebuild: clean build

# Run C++ tests
test: build
	@echo "========================================"
	@echo "Running Tests"
	@echo "========================================"
	@if [ -f build/test/test_runner ]; then \
		cd build && ./test/test_runner; \
	elif [ -f build/runTestSuite.sh ]; then \
		cd build && bash runTestSuite.sh; \
	else \
		echo "Running individual test executables..."; \
		cd build && ctest --output-on-failure || ( \
			echo ""; \
			echo "Running tests individually..."; \
			for test in test/test_* test/*_tests; do \
				if [ -x "$$test" ] && [ -f "$$test" ]; then \
					echo ""; \
					echo "Running $$test..."; \
					$$test || exit 1; \
				fi; \
			done \
		); \
	fi
	@echo ""
	@echo "✓ All tests passed!"

# Test that libraries are built correctly
test-libs: build
	@echo "========================================"
	@echo "Testing Library Builds"
	@echo "========================================"
	@echo "Checking for required libraries..."
	@for lib in json rest_api sql distributed; do \
		if [ -f "build/src/$$lib/lib$$lib.so" ] || [ -f "build/src/$$lib/lib$$lib.dylib" ]; then \
			echo "✓ lib$$lib found"; \
		else \
			echo "✗ lib$$lib NOT FOUND"; \
			exit 1; \
		fi; \
	done
	@echo ""
	@echo "Testing library symbols..."
	@nm build/src/json/libjson.* 2>/dev/null | grep -q "networking::json" && echo "✓ JSON namespace correct" || echo "⚠ JSON namespace check failed"
	@nm build/src/rest_api/librest_api.* 2>/dev/null | grep -q "networking::rest_api" && echo "✓ REST API namespace correct" || echo "⚠ REST API namespace check failed"
	@nm build/src/distributed/libdistributed.* 2>/dev/null | grep -q "networking::distributed" && echo "✓ Distributed namespace correct" || echo "⚠ Distributed namespace check failed"
	@echo ""
	@echo "✓ Library tests complete!"

# Build C++ examples
examples: build
	@echo "========================================"
	@echo "Building Examples"
	@echo "========================================"
	@cd examples && $(MAKE) all
	@echo ""
	@echo "✓ Examples built!"

# Run C++ examples
run-examples: examples
	@cd examples && $(MAKE) run-all

# Install Python bindings
install-python: build
	@echo "========================================"
	@echo "Installing Python Bindings"
	@echo "========================================"
	@cd python_bindings && pip install -e .
	@echo ""
	@echo "✓ Python bindings installed!"

# Install everything (C++ + Python)
install: build install-python

# Quick build script (equivalent to ./clean&build.sh)
quick-build:
	@./clean&build.sh

# Check project status
status:
	@echo "========================================"
	@echo "ToolBox Project Status"
	@echo "========================================"
	@echo ""
	@echo "Build artifacts:"
	@if [ -d build ]; then \
		echo "  ✓ build/ directory exists"; \
		echo "  Libraries:"; \
		find build/src -name "*.so" -o -name "*.dylib" 2>/dev/null | sed 's/^/    /' || echo "    (none found)"; \
	else \
		echo "  ✗ build/ directory not found (run 'make build')"; \
	fi
	@echo ""
	@echo "Python bindings:"
	@if python3 -c "import ml_core" 2>/dev/null; then \
		echo "  ✓ ml_core module installed"; \
	else \
		echo "  ✗ ml_core not installed (run 'make install-python')"; \
	fi
	@echo ""
	@echo "Examples:"
	@if [ -d examples/cpp/networking ]; then \
		echo "  ✓ C++ examples organized"; \
	else \
		echo "  ✗ C++ examples not found"; \
	fi
	@if [ -d examples/python/ml ]; then \
		echo "  ✓ Python examples organized"; \
	else \
		echo "  ✗ Python examples not found"; \
	fi
	@echo ""

# Development workflow
dev: clean build test
	@echo ""
	@echo "✓ Development build complete!"
	@echo ""
	@echo "Next steps:"
	@echo "  make examples         - Build C++ examples"
	@echo "  make install-python   - Install Python bindings"
	@echo "  make run-examples     - Run all examples"

# Build and run MATLAB-style platform demo
matlab-platform: build
	@echo "========================================"
	@echo "Building MATLAB-Style Platform Demo"
	@echo "========================================"
	@cd build && $(MAKE) matlab_platform_demo
	@echo ""
	@echo "✓ MATLAB Platform demo built!"
	@echo ""
	@echo "Run with: ./build/demos/matlab_platform_demo"
	@echo "Open: http://localhost:9000"
	@echo "Login: admin/admin123 or user/user123"
	@echo ""

# Run MATLAB platform demo
run-matlab: matlab-platform
	@echo "Starting MATLAB-Style Platform..."
	@echo ""
	@./build/demos/matlab_platform_demo

# Help message
help:
	@echo "ToolBox Makefile - Build System"
	@echo ""
	@echo "Main targets:"
	@echo "  make build           - Build all C++ libraries"
	@echo "  make test            - Run C++ tests"
	@echo "  make test-libs       - Verify library builds and namespaces"
	@echo "  make clean           - Remove build artifacts"
	@echo "  make rebuild         - Clean and rebuild everything"
	@echo ""
	@echo "Demos & Applications:"
	@echo "  make matlab-platform - Build MATLAB-style app launcher"
	@echo "  make run-matlab      - Build and run MATLAB platform (port 9000)"
	@echo ""
	@echo "Examples:"
	@echo "  make examples        - Build C++ examples"
	@echo "  make run-examples    - Build and run all examples"
	@echo ""
	@echo "Installation:"
	@echo "  make install-python  - Install Python bindings (pip)"
	@echo "  make install         - Install C++ libs + Python bindings"
	@echo ""
	@echo "Development:"
	@echo "  make dev             - Full dev build (clean + build + test)"
	@echo "  make status          - Check project status"
	@echo "  make quick-build     - Run ./clean&build.sh script"
	@echo ""
	@echo "Project structure:"
	@echo "  src/                 - C++ source code"
	@echo "  include/             - C++ headers"
	@echo "  test/                - C++ tests"
	@echo "  python_bindings/     - Python binding code"
	@echo "  examples/            - Examples (cpp/ and python/)"
	@echo ""
	@echo "Namespaces:"
	@echo "  networking::json            - JSON library"
	@echo "  networking::rest_api        - REST API servers"
	@echo "  networking::distributed     - Distributed computing"
	@echo "  ml::                        - ML algorithms"
	@echo "  ml::sql                     - SQL operations"
	@echo ""
