# Examples Makefile
# Builds and runs both C++ and Python examples

.PHONY: all cpp python clean help run-all run-cpp run-python status

# Default: build C++ examples
all: cpp

# Build C++ examples
cpp:
	@echo "========================================"
	@echo "Building C++ Examples"
	@echo "========================================"
	@cd cpp && $(MAKE) all
	@echo ""
	@echo "✓ C++ examples built!"

# Check Python examples
python:
	@echo "========================================"
	@echo "Python Examples"
	@echo "========================================"
	@echo "Python examples are ready to run."
	@echo "Make sure Python bindings are installed:"
	@echo "  cd ../python_bindings && pip install -e ."
	@echo ""
	@echo "Available Python examples:"
	@echo "  python/ml/           - ML and deep learning examples"
	@echo "  python/networking/   - REST API and database examples"
	@echo "  python/distributed/  - Distributed computing examples"
	@echo ""

# Clean all examples
clean:
	@echo "Cleaning examples..."
	@cd cpp && $(MAKE) clean
	@echo "✓ Examples cleaned!"

# Run all C++ examples
run-cpp: cpp
	@echo "========================================"
	@echo "Running C++ Examples"
	@echo "========================================"
	@cd cpp && $(MAKE) run-all

# Run Python examples (with checks)
run-python:
	@echo "========================================"
	@echo "Running Python Examples"
	@echo "========================================"
	@if ! python3 -c "import ml_core" 2>/dev/null; then \
		echo "ERROR: ml_core module not found!"; \
		echo "Please install Python bindings first:"; \
		echo "  cd ../python_bindings && pip install -e ."; \
		exit 1; \
	fi
	@echo ""
	@echo "Running ML examples..."
	@echo "----------------------"
	@cd python/ml && python3 deep_learning_example.py
	@echo ""
	@echo "Running networking examples..."
	@echo "------------------------------"
	@cd python/networking && python3 rest_api_example.py
	@echo ""
	@echo "✓ Python examples complete!"

# Run all examples
run-all: run-cpp

# Check examples status
status:
	@echo "========================================"
	@echo "Examples Status"
	@echo "========================================"
	@echo ""
	@echo "C++ Examples:"
	@if [ -d cpp/networking ] && [ -d cpp/ml ] && [ -d cpp/sql ]; then \
		echo "  ✓ Directory structure correct"; \
		echo "    - cpp/networking/ (4 examples)"; \
		echo "    - cpp/ml/ (1 example)"; \
		echo "    - cpp/sql/ (1 example)"; \
	else \
		echo "  ✗ Directory structure incorrect"; \
	fi
	@echo ""
	@if [ -d ../build/examples ]; then \
		echo "  Built executables:"; \
		ls -1 ../build/examples/ 2>/dev/null | sed 's/^/    /' || echo "    (none)"; \
	else \
		echo "  ✗ No built executables (run 'make cpp')"; \
	fi
	@echo ""
	@echo "Python Examples:"
	@if [ -d python/ml ] && [ -d python/networking ] && [ -d python/distributed ]; then \
		echo "  ✓ Directory structure correct"; \
		echo "    - python/ml/ (7 examples)"; \
		echo "    - python/networking/ (2 examples)"; \
		echo "    - python/distributed/ (2 items)"; \
	else \
		echo "  ✗ Directory structure incorrect"; \
	fi
	@echo ""
	@if python3 -c "import ml_core" 2>/dev/null; then \
		echo "  ✓ Python bindings installed"; \
	else \
		echo "  ✗ Python bindings not installed"; \
		echo "    Run: cd ../python_bindings && pip install -e ."; \
	fi
	@echo ""

# List all examples
list:
	@echo "Available Examples:"
	@echo ""
	@echo "C++ Examples (cpp/):"
	@echo "  Networking:"
	@find cpp/networking -name "*.cpp" 2>/dev/null | sed 's|cpp/networking/|    |' || true
	@echo "  ML:"
	@find cpp/ml -name "*.cpp" 2>/dev/null | sed 's|cpp/ml/|    |' || true
	@echo "  SQL:"
	@find cpp/sql -name "*.cpp" 2>/dev/null | sed 's|cpp/sql/|    |' || true
	@echo ""
	@echo "Python Examples (python/):"
	@echo "  ML:"
	@find python/ml -name "*.py" 2>/dev/null | sed 's|python/ml/|    |' || true
	@echo "  Networking:"
	@find python/networking -name "*.py" 2>/dev/null | sed 's|python/networking/|    |' || true
	@echo "  Distributed:"
	@find python/distributed -name "*.py" 2>/dev/null | sed 's|python/distributed/|    |' || true
	@echo ""

# Help
help:
	@echo "Examples Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make all (default)  - Build C++ examples"
	@echo "  make cpp            - Build C++ examples"
	@echo "  make python         - Show Python examples info"
	@echo "  make clean          - Clean built examples"
	@echo ""
	@echo "Running:"
	@echo "  make run-all        - Run all C++ examples"
	@echo "  make run-cpp        - Run C++ examples"
	@echo "  make run-python     - Run Python examples"
	@echo ""
	@echo "Information:"
	@echo "  make status         - Check examples status"
	@echo "  make list           - List all example files"
	@echo ""
	@echo "Directory structure:"
	@echo "  cpp/networking/     - REST API, JSON, HTTP protocols"
	@echo "  cpp/ml/             - Model serving"
	@echo "  cpp/sql/            - Database and Prisma"
	@echo "  python/ml/          - ML and deep learning"
	@echo "  python/networking/  - REST API and database"
	@echo "  python/distributed/ - Distributed computing"
	@echo ""
	@echo "To build libraries first:"
	@echo "  cd .. && make build"
	@echo ""
