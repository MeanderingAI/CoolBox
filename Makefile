# Build Emscripten JS/WASM bindings (default MODULARIZE=1, override with EMS_MODULARIZE=0)
.PHONY: build-emscripten
build-emscripten:
	@echo "========================================"
	@echo "Building Emscripten JS/WASM bindings (_libraries/emscripten_bindings)"
	@echo "========================================"
	@mkdir -p build
	cd build && cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DEMSCRIPTEN_MODULARIZE=$(or $(EMS_MODULARIZE),ON) .. && $(MAKE) advanced_logging_js
	@echo "✓ Emscripten bindings built! (MODULARIZE=$$(if $(EMS_MODULARIZE),$(EMS_MODULARIZE),ON))"
	@ls -lh _libraries/emscripten_bindings/*.js || true
	@echo ""
# Alias for user convenience
build-service_manager: build-service-manager
# List all built apps
.PHONY: list-apps
list-apps:
	@echo "Available Apps (built in build/_binaries/apps):"
	@find build/_binaries/apps -maxdepth 1 -type f -perm +111 2>/dev/null | xargs -n1 basename | sort || echo "  (none found)"
	@echo ""

# List all built libraries
.PHONY: list-libs
list-libs:
	@echo "Available Libraries (built in build/_libraries/src):"
	@find build/_libraries/src -type f \( -name "*.so" -o -name "*.dylib" -o -name "*.a" \) 2>/dev/null | xargs -n1 basename | sort || echo "  (none found)"
	@echo ""

# List all built services
.PHONY: list-services
list-services:
	@echo "Available Services (built in build/_binaries/services):"
	@find build/_binaries/services -maxdepth 1 -type f -perm +111 2>/dev/null | xargs -n1 basename | sort || echo "  (none found)"
	@echo ""
# List all runnable apps and demos
.PHONY: help-run
help-run:
	@echo "========================================"
	@echo "Runnable Apps (built in build/_binaries/apps):"
	@echo "========================================"
	sh -c '[ -x build/_binaries/apps/service_manager_app ] && echo "  make run-service_manager  | build/_binaries/apps/service_manager_app"'
	sh -c '[ -x build/_binaries/apps/matlab_platform_app ] && echo "  (no make run-matlab_platform_app) | build/_binaries/apps/matlab_platform_app"'
	@echo ""
	@echo "========================================"
	@echo "Runnable Demos (built in build/_binaries/demos):"
	@echo "========================================"
	@find build/_binaries/demos -maxdepth 1 -type f -executable ! -name "*.dylib" ! -name "*.so" 2>/dev/null | while read demo; do \
		name=$$(basename $$demo); \
		echo "  ./$$demo"; \
	done | sort
	@echo ""
	@echo "(Note: Some apps/demos may require building first.)"
# Build all services
.PHONY: build-services
build-services:
	@echo "========================================"
	@echo "Building all services in _binaries/services/"
	@echo "========================================"
	@cd _binaries/services && cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release . && $(MAKE)
	@echo ""
	@echo "✓ All services built!"
	@echo "Service binaries/libraries:"
	@ls -lh _binaries/services/account_service 2>/dev/null || true
	@ls -lh _binaries/services/proxy_service/libproxy_service_lib.a 2>/dev/null || true
	@ls -lh _binaries/services/system_monitor/libsystem_monitor_lib.a 2>/dev/null || true
	@echo ""

# Build all demos


# Build all apps
.PHONY: build-apps
build-apps:
	@echo "========================================"
	@echo "Building all apps in _binaries/apps/"
	@echo "========================================"
	@cd _binaries/apps && cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release . && $(MAKE)
	@echo ""
	@echo "✓ All apps built!"
	@ls -lh _binaries/apps/* || true
	@echo ""

# Run individual services
.PHONY: run-account_service run-metrics_backend_service run-proxy_service
run-account_service:
	_binaries/services/build/account_service
run-metrics_backend_service:
	_binaries/services/build/metrics_backend_service
run-proxy_service:
	_binaries/services/build/proxy_service

# Run individual demos (add more as needed)
.PHONY: run-cache_demo run-data_structures_demo run-concurrent_demo run-ml_demo
run-cache_demo:
	_binaries/demos/cache_demo
run-data_structures_demo:
	_binaries/demos/data_structures_demo
run-concurrent_demo:
	_binaries/demos/concurrent_demo
run-ml_demo:
	_binaries/demos/ml_demo

# Run individual apps (add more as needed)
.PHONY: run-service_manager run-service_breaker run-url_shortener
run-service_manager:
	build/_binaries/apps/service_manager/service_manager
run-service_breaker:
	_binaries/apps/service_breaker/service_breaker
run-url_shortener:
	_binaries/apps/url_shortener/url_shortener

# Help target


.PHONY: all build build-service-manager clean test install examples help



# Default target: show available commands
.PHONY: all
all: help-run

# Build all C++ _libraries
build:
	@echo "========================================"
	@echo "Building ToolBox _Libraries"
	@echo "========================================"
	@mkdir -p build
	@cd build && cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release .. && $(MAKE)
	@echo ""
	@echo "✓ Build complete!"
	@echo ""
	@echo "_Libraries built:"
	@find build/src -name "*.so" -o -name "*.dylib" 2>/dev/null || true
	@echo ""


# Build the service_manager app in _binaries/apps/

# Ensure build directory and CMake configuration
.PHONY: prepare-build-dir
prepare-build-dir:
	@mkdir -p build
	@if [ ! -f build/CMakeCache.txt ]; then \
	  echo "Configuring CMake in ./build..."; \
	  cd build && cmake ..; \
	fi

# Build the service_manager app in _binaries/apps/
build-service-manager: prepare-build-dir
	@echo "========================================"
	@echo "Building service_manager app in _binaries/apps/"
	@echo "========================================"
	@cmake --build build --target service_manager -j8 2>&1 | grep -E "(Built target|error:|warning:|\[)" || echo "Building..."
	@echo ""
	@echo "✓ service_manager build complete!"
	@echo ""
	@ls -lh build/_binaries/apps/service_manager || true
	@echo ""


# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf build
	@cd examples/cpp && $(MAKE) clean 2>/dev/null || true
	@cd libraries/python_bindings && rm -rf build dist *.egg-info 2>/dev/null || true
	@echo "✓ Clean complete!"

# MATLAB Platform Targets
.PHONY: build-matlab-platform run-matlab-platform

build-matlab-platform:
	@echo "========================================"
	@echo "Building MATLAB Platform"
	@echo "========================================"
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | grep -v "^--" || true
	@cmake --build build --target matlab_platform_demo -j8 2>&1 | grep -E "(Built target|error:|warning:|\[)" || echo "Building..."
	@if [ -f build/demos/matlab_platform_demo ]; then \
		echo ""; \
		echo "✓ MATLAB Platform built successfully!"; \
		echo "  Binary: build/demos/matlab_platform_demo"; \
		ls -lh build/demos/matlab_platform_demo | awk '{print "  Size: " $$5}'; \
	else \
		echo ""; \
		echo "✗ Build failed!"; \
		exit 1; \
	fi
	@echo ""

run-matlab-platform: build-matlab-platform
	@echo "========================================"
	@echo "Starting MATLAB Platform"
	@echo "========================================"
	@echo "Port: 9000"
	@echo "URL:  http://localhost:9000"
	@echo ""
	@echo "Press Ctrl+C to stop"
	@echo "========================================"
	@echo ""
	@build/demos/matlab_platform_demo 9000

# Library Build Targets
.PHONY: build-shared-_libraries build-ml-_libraries build-datastructure-_libraries build-network-_libraries


build-shared-_libraries: build
	@echo "✓ All shared _libraries built"
	@find build/src -name "*.so" -o -name "*.dylib" 2>/dev/null || true


build-ml-_libraries:
	@echo "========================================"
	@echo "Building ML _Libraries"
	@echo "========================================"
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | grep -v "^--" || true
	@cmake --build build --target probability_distribution -j8 2>&1 | tail -5
	@echo "✓ ML _libraries built"
	@find build/src -name "*probability*" -o -name "*ml*" 2>/dev/null | grep -E "\.(so|dylib)$$" || true


build-datastructure-_libraries:
	@echo "========================================"
	@echo "Building Data Structure _Libraries"
	@echo "========================================"
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | grep -v "^--" || true
	@cmake --build build --target data_structures -j8 2>&1 | tail -5
	@echo "✓ Data structure _libraries built"
	@find build/src -name "*data_structures*" 2>/dev/null | grep -E "\.(so|dylib)$$" || true


build-network-_libraries:
	@echo "========================================"
	@echo "Building Network _Libraries"
	@echo "========================================"
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | grep -v "^--" || true
	@cmake --build build --target cache_server --target dns_server --target proxy_server -j8 2>&1 | tail -10
	@echo "✓ Network _libraries built"
	@find build/src -name "*server*" -o -name "*network*" 2>/dev/null | grep -E "\.(so|dylib)$$" || true


# Documentation targets
.PHONY: docs docs-clean docs-rebuild

docs:
	@echo "========================================"
	@echo "Generating API Documentation (Doxygen)"
	@echo "========================================"
	@cd gen_docs && cmake -DTOOLBOX_SOURCE_DIR="$(shell pwd)" . && $(MAKE)
	@echo ""
	@echo "✓ Documentation generated. Index files:"
	@echo "  - Monolithic: gen_docs/html/index.html"
	@find gen_docs/html/libs -type f -name index.html | sort | awk '{print "  - " $$1}'
	@echo ""

docs-clean:
	@echo "Cleaning generated documentation..."
	@rm -rf gen_docs/html gen_docs/Doxyfile
	@echo "✓ Docs cleaned!"

docs-rebuild: docs-clean docs

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
		cd build/test && ctest --output-on-failure || ( \
			echo ""; \
			echo "Running tests individually..."; \
			for test in test_* *_tests; do \
				if [ -x "$test" ] && [ -f "$test" ]; then \
					echo ""; \
					echo "Running $test..."; \
					./$test || exit 1; \
				fi; \
			done \
		); \
	fi
	@echo ""
	@echo "✓ All tests passed!"

# Test that _libraries are built correctly
test-libs: build
	@echo "========================================"
	@echo "Testing Library Builds"
	@echo "========================================"
	@echo "Checking for required _libraries..."
	@for lib in json rest_api sql distributed; do \
		if [ -f "build/_libraries/src/$$lib/lib$$lib.so" ] || [ -f "build/_libraries/src/$$lib/lib$$lib.dylib" ]; then \
			echo "✓ lib$$lib found"; \
		else \
			echo "✗ lib$$lib NOT FOUND"; \
			exit 1; \
		fi; \
	done
	@echo ""
	@echo "Testing library symbols..."
	@nm build/_libraries/src/json/libjson.* 2>/dev/null | grep -q "networking::json" && echo "✓ JSON namespace correct" || echo "⚠ JSON namespace check failed"
	@nm build/_libraries/src/rest_api/librest_api.* 2>/dev/null | grep -q "networking::rest_api" && echo "✓ REST API namespace correct" || echo "⚠ REST API namespace check failed"
	@nm build/_libraries/src/distributed/libdistributed.* 2>/dev/null | grep -q "networking::distributed" && echo "✓ Distributed namespace correct" || echo "⚠ Distributed namespace check failed"
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

# Demo Targets
.PHONY: build-demos run-demos run-cache-demo run-ml-demo run-webserver-demo run-concurrent-demo run-html-demo

build-demos:
	@echo "========================================"
	@echo "Building All Demos"
	@echo "========================================"
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | grep -v "^--" || true
	@cmake --build build -j8 2>&1 | grep -E "(demos/.*demo|Built target.*demo|error:|warning:)" || echo "Building demos..."
	@echo ""
	@echo "✓ Demos built successfully!"
	@echo ""
	@echo "Available demos:"
	@find build/demos -maxdepth 1 -type f -executable ! -name "*.dylib" ! -name "*.so" 2>/dev/null | sed 's|build/demos/||' | sort || true
	@echo ""

run-demos:
	@echo "========================================"
	@echo "Available Demos"
	@echo "========================================"
	@echo ""
	@echo "Run any demo with:"
	@find build/demos -maxdepth 1 -type f -executable ! -name "*.dylib" ! -name "*.so" 2>/dev/null | while read demo; do \
		name=$$(basename $$demo); \
		echo "  make run-$$name" | sed 's/_/-/g'; \
	done | sort
	@echo ""
	@echo "Or run directly:"
	@find build/demos -maxdepth 1 -type f -executable ! -name "*.dylib" ! -name "*.so" 2>/dev/null | while read demo; do \
		echo "  ./$$demo"; \
	done | sort
	@echo ""

run-cache-demo: build-demos
	@echo "Running Cache Demo..."
	@echo "========================================"
	@./build/demos/cache_demo

run-ml-demo: build-demos
	@echo "Running ML Demo..."
	@echo "========================================"
	@./build/demos/ml_demo

run-webserver-demo: build-demos
	@echo "Running Webserver Demo..."
	@echo "========================================"
	@echo "Starting webserver on port 8080..."
	@./build/demos/webserver_demo

run-concurrent-demo: build-demos
	@echo "Running Concurrent Demo..."
	@echo "========================================"
	@./build/demos/concurrent_demo

run-html-demo: build-demos
	@echo "Running HTML Demo..."
	@echo "========================================"
	@./build/demos/html_demo

run-data-structures-demo: build-demos
	@echo "Running Data Structures Demo..."
	@echo "========================================"
	@./build/demos/data_structures_demo

# Unit Test Targets
.PHONY: build-tests run-tests run-test-distribution run-test-pca run-test-decision-tree run-test-kalman run-test-svm

build-tests:
	@echo "========================================"
	@echo "Building Unit Tests"
	@echo "========================================"
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | grep -v "^--" || true
	@cmake --build build/test -j8 2>&1 | grep -E "(test/.*test|Built target.*test|error:|warning:)" || echo "Building tests..."
	@echo ""
	@echo "✓ Tests built successfully!"
	@echo ""
	@echo "Available tests:"
	@find build/test -maxdepth 1 -type f -executable -name "test_*" 2>/dev/null | sed 's|build/test/||' | sort || true
	@echo ""

run-tests: build-tests
	@echo "========================================"
	@echo "Running All Unit Tests"
	@echo "========================================"
	@echo ""
	@passed=0; failed=0; \
	for test in build/test/test_*; do \
		if [ -x "$$test" ] && [ -f "$$test" ]; then \
			test_name=$$(basename $$test); \
			echo "Running $$test_name..."; \
			if $$test > /dev/null 2>&1; then \
				echo "  ✓ $$test_name PASSED"; \
				passed=$$((passed + 1)); \
			else \
				echo "  ✗ $$test_name FAILED"; \
				failed=$$((failed + 1)); \
			fi; \
			echo ""; \
		fi; \
	done; \
	echo "========================================"; \
	echo "Test Results:"; \
	echo "  Passed: $$passed"; \
	echo "  Failed: $$failed"; \
	echo "========================================"

run-test-distribution: build-tests
	@echo "Running Distribution Tests..."
	@echo "========================================"
	@./build/test/test_distribution

run-test-pca: build-tests
	@echo "Running PCA Tests..."
	@echo "========================================"
	@./build/test/test_pca

run-test-decision-tree: build-tests
	@echo "Running Decision Tree Tests..."
	@echo "========================================"
	@./build/test/test_decision_tree

run-test-kalman: build-tests
	@echo "Running Kalman Filter Tests..."
	@echo "========================================"
	@./build/test/test_kalman_filter
	@./build/test/test_extended_kalman_filter
	@./build/test/test_unscented_kalman_filter

run-test-svm: build-tests
	@echo "Running SVM Tests..."
	@echo "========================================"
	@./build/test/test_support_vector_machine

run-test-knn: build-tests
	@echo "Running KNN Tests..."
	@echo "========================================"
	@./build/test/test_knn

run-test-hmm: build-tests
	@echo "Running Hidden Markov Model Tests..."
	@echo "========================================"
	@./build/test/test_hidden_markov_model

# Install Python bindings
install-python: build
	@echo "========================================"
	@echo "Installing Python Bindings"
	@echo "========================================"
	@cd libraries/python_bindings && pip install -e .
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
		echo "  _Libraries:"; \
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
	@echo "  make build                      - Build all C++ _libraries"
	@echo "  make test                       - Run C++ tests"
	@echo "  make test-libs                  - Verify library builds and namespaces"
	@echo "  make clean                      - Remove build artifacts"
	@echo "  make rebuild                    - Clean and rebuild everything"
	@echo ""
	@echo "Library Builds:"
	@echo "  make build-shared-_libraries     - Build all shared _libraries"
	@echo "  make build-ml-_libraries         - Build ML/AI _libraries only"
	@echo "  make build-datastructure-_libraries - Build data structure _libraries"
	@echo "  make build-network-_libraries    - Build networking _libraries"
	@echo ""
	@echo "MATLAB Platform:"
	@echo "  make build-matlab-platform      - Build MATLAB-style app launcher"
	@echo "  make run-matlab-platform        - Build and run MATLAB platform (port 9000)"
	@echo ""
	@echo "Service Manager (Hot-Reload):"
	@echo "  make build-service-management   - Build service manager"
	@echo "  make run-service-management     - Run service manager (port 9003)"
	@echo "  make build-binary-management    - Build binary manager"
	@echo "  make run-binary-management      - Run binary manager (port 9006)"
	@echo ""
	@echo "Demos:"
	@echo "  make build-demos                - Build all demo applications"
	@echo "  make run-demos                  - List available demos to run"
	@echo "  make run-cache-demo             - Run cache demo"
	@echo "  make run-ml-demo                - Run machine learning demo"
	@echo "  make run-webserver-demo         - Run webserver demo"
	@echo ""
	@echo "Unit Tests:"
	@echo "  make build-tests                - Build all unit tests"
	@echo "  make run-tests                  - Run all unit tests"
	@echo "  make run-test-distribution      - Run distribution tests"
	@echo "  make run-test-pca               - Run PCA tests"
	@echo "  make run-test-decision-tree     - Run decision tree tests"
	@echo ""
	@echo "Examples:"
	@echo "  make examples                   - Build C++ examples"
	@echo "  make run-examples               - Build and run all examples"
	@echo ""
	@echo "Installation:"
	@echo "  make install-python             - Install Python bindings (pip)"
	@echo "  make install                    - Install C++ libs + Python bindings"
	@echo ""
	@echo "Development:"
	@echo "  make dev                        - Full dev build (clean + build + test)"
	@echo "  make status                     - Check project status"
	@echo "  make quick-build                - Run ./clean&build.sh script"
	@echo ""
	@echo "Project structure:"
	@echo "  _libraries/src/       - C++ source code"
	@echo "  _libraries/include/   - C++ headers"
	@echo "  test/                - C++ tests"
	@echo "  python_bindings/     - Python binding code"
	@echo "  examples/            - Examples (cpp/ and python/)"
	@echo "  demos/               - Service manager demo"
	@echo ""
	@echo "Namespaces:"
	@echo "  networking::json            - JSON library"
	@echo "  networking::rest_api        - REST API servers"
	@echo "  networking::distributed     - Distributed computing"
	@echo "  ml::                        - ML algorithms"
	@echo "  ml::sql                     - SQL operations"
	@echo ""

# Service Manager Targets
.PHONY: build-service-management run-service-management kill-service-management

# Build service manager
build-service-management:
	@echo "========================================"
	@echo "Building Service Manager"
	@echo "========================================"
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | grep -v "^--" || true
	@cmake --build build --target service_manager -j8 2>&1 | grep -E "(Built target|error:|warning:|\[)" || echo "Building..."
	@if [ -f build/apps/service_manager ]; then \
		echo ""; \
		echo "✓ Service Manager built successfully!"; \
		echo "  Binary: build/apps/service_manager"; \
		ls -lh build/apps/service_manager | awk '{print "  Size: " $$5}'; \
	else \
		echo ""; \
		echo "✗ Build failed!"; \
		exit 1; \
	fi
	@echo ""

# Run the service manager
run-service-management: build-service-management
	@echo "========================================"
	@echo "Starting Service Manager"
	@echo "========================================"
	@echo "Port: 9003"
	@echo "URL:  http://localhost:9003/app/manager"
	@echo ""
	@echo "Features:"
	@echo "  • Hot-reload for HTML files"
	@echo "  • Auto-rebuild on C++ changes"
	@echo "  • Service monitoring & console"
	@echo "  • Request logging & CSV export"
	@echo "  • nginx routing configuration"
	@echo "  • Build info & dependencies"
	@echo ""
	@echo "Press Ctrl+C to stop"
	@echo "========================================"
	@echo ""
	@build/demos/service_manager 9003

# Kill any running service manager instances
kill-service-management:
	@echo "Stopping service manager instances..."
	@pkill -f "service_manager 9003" 2>/dev/null || pkill -f "service_manager" 2>/dev/null || echo "No running instances found"
	@echo "✓ Done"

# Binary Manager Targets
.PHONY: build-binary-management run-binary-management kill-binary-management

# Build binary manager
build-binary-management:
	@echo "========================================"
	@echo "Building Binary Manager"
	@echo "========================================"
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | grep -v "^--" || true
	@cmake --build build --target binary_manager -j8 2>&1 | grep -E "(Built target|error:|warning:|\[)" || echo "Building..."
	@if [ -f build/demos/binary_manager ]; then \
		echo ""; \
		echo "✓ Binary Manager built successfully!"; \
		echo "  Binary: build/demos/binary_manager"; \
		ls -lh build/demos/binary_manager | awk '{print "  Size: " $$5}'; \
	else \
		echo ""; \
		echo "✗ Build failed!"; \
		exit 1; \
	fi
	@echo ""

# Run the binary manager
run-binary-management: build-binary-management
	@echo "========================================"
	@echo "Starting Binary Manager"
	@echo "========================================"
	@echo "Port: 9006"
	@echo "URL:  http://localhost:9006"
	@echo ""
	@echo "Features:"
	@echo "  • View all project executables"
	@echo "  • Filter by type (demos/tests/tools)"
	@echo "  • Rebuild individual _binaries"
	@echo "  • View build commands"
	@echo "  • Binary size and timestamps"
	@echo ""
	@echo "Press Ctrl+C to stop"
	@echo "========================================"
	@echo ""
	@build/demos/binary_manager 9006

# Kill any running binary manager instances
kill-binary-management:
	@echo "Stopping binary manager instances..."
	@pkill -f "binary_manager" 2>/dev/null || echo "No running instances found"
	@echo "✓ Done"

# Generic run-app target
.PHONY: run-%
run-%:
	@bin="build/_binaries/apps/$*"; \
	if [ -x "$$bin" ]; then \
		echo "Running $$bin..."; \
		"$$bin"; \
	else \
		echo "App '$*' not found or not executable in build/_binaries/apps."; \
		exit 1; \
	fi

# Pattern rule to run any app in build/_binaries/apps
.PHONY: run-%
run-%:
	@echo "Running app: $*"
	@build/_binaries/apps/$*

# Pattern rule to build any app using CMake
.PHONY: build-%
build-%:
	@echo "Building app: $*"
	@cd build && cmake --build . --target $*
