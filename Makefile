.PHONY: all help build run test install build-% run-% test-% install-% completion clean
clean:
	@echo "Cleaning build artifacts..."
	rm -rf build _binaries/CMakeFiles _binaries/apps/*/CMakeFiles _binaries/services/*/CMakeFiles _binaries/services/*/Makefile _binaries/apps/*/Makefile _binaries/services/*/cmake_install.cmake _binaries/apps/*/cmake_install.cmake _libraries/CMakeFiles _libraries/src/*/CMakeFiles _libraries/src/*/Makefile _libraries/src/*/cmake_install.cmake _test/CMakeFiles _test/*/CMakeFiles _test/*/Makefile _test/*/cmake_install.cmake Testing Temporary
	@echo "Clean complete."

all: help

help:
	@echo "========================================="
	@echo " Dynamic Makefile: Discovered Executables and Libraries"
	@echo "========================================="
	@echo "[APPS] (_binaries/apps):"
	@grep -h -E '^add_executable|^add_library' _binaries/apps/*/CMakeLists.txt 2>/dev/null | sed -E 's/add_executable\(([^ ]+).*/  Executable: \1/;s/add_library\(([^ ]+).*/  Library: \1/' || echo "  (none found)"
	@echo "[DEMOS] (_binaries/demos):"
	@grep -h -E '^add_executable' _binaries/demos/CMakeLists.txt 2>/dev/null | sed -E 's/add_executable\(([^ ]+).*/  Executable: \1/' || echo "  (none found)"
	@echo "[SERVICES] (_binaries/services):"
	@grep -h -E '^add_executable|^add_library' _binaries/services/*/CMakeLists.txt 2>/dev/null | sed -E 's/add_executable\(([^ ]+).*/  Executable: \1/;s/add_library\(([^ ]+).*/  Library: \1/' || echo "  (none found)"
	@echo "[LIBRARIES] (_libraries/src):"
	@find _libraries/src -name CMakeLists.txt -exec grep -h -E '^add_library' {} + 2>/dev/null | sed -E 's/add_library\(([^ ]+).*/  Library: \1/' || echo "  (none found)"
	@echo "========================================="
	@echo "Usage: make build|run|test|install [NAME]"
	@echo "  build      - Build all apps, demos, services, and libraries"
	@echo "  run        - Run all executables (where possible)"
	@echo "  test       - Run all tests (if any)"
	@echo "  install    - Install all libraries (.dylib) to ./lib"
	@echo "  build-NAME - Build a specific executable or library by name"
	@echo "  run-NAME   - Run a specific executable by name"
	@echo "  test-NAME  - Run a specific test by name (if available)"
	@echo "  install-NAME - Install a specific library by name"
	@echo "  completion - Output shell completion script for make targets"
	@echo "========================================="

build:
	@echo "[Makefile] Running CMake configuration (cmake -S . -B build)..."
	cmake -S . -B build
	@echo "Building all apps, demos, services, and libraries..."
	@$(MAKE) -s build-all-apps
	@$(MAKE) -s build-all-demos
	@$(MAKE) -s build-all-services
	@$(MAKE) -s build-all-libraries

build-all-apps:
	@if [ -f build/Makefile ]; then \
		echo "[Makefile] Filtering app targets to only those defined in CMake..."; \
		for f in _binaries/apps/*/CMakeLists.txt; do \
			n=$$(grep -E '^add_executable|^add_library' $$f | sed -E 's/add_executable\(([^ ]+).*/\1/;s/add_library\(([^ ]+).*/\1/'); \
			for t in $$n; do \
				if grep -qE "^$${t}:" build/Makefile; then $(MAKE) -s build-$$t; fi; \
			done; \
		done; \
	else \
		echo "[Makefile] No build/Makefile found. Run 'make build' to configure CMake first."; \
	fi

build-all-demos:
	@if [ -f build/Makefile ]; then \
		echo "[Makefile] Filtering demo targets to only those defined in CMake..."; \
		for n in $$(grep -h -E '^add_executable' _binaries/demos/CMakeLists.txt 2>/dev/null | sed -E 's/add_executable\(([^ ]+).*/\1/'); do \
			if grep -qE "^$${n}:" build/Makefile; then $(MAKE) -s build-$$n; fi; \
		done; \
	else \
		echo "[Makefile] No build/Makefile found. Run 'make build' to configure CMake first."; \
	fi

build-all-services:
	@if [ -f build/Makefile ]; then \
		echo "[Makefile] Filtering service targets to only those defined in CMake..."; \
		for f in _binaries/services/*/CMakeLists.txt; do \
			n=$$(grep -E '^add_executable|^add_library' $$f | sed -E 's/add_executable\(([^ ]+).*/\1/;s/add_library\(([^ ]+).*/\1/'); \
			for t in $$n; do \
				if grep -qE "^$${t}:" build/Makefile; then $(MAKE) -s build-$$t; fi; \
			done; \
		done; \
	else \
		echo "[Makefile] No build/Makefile found. Run 'make build' to configure CMake first."; \
	fi

build-all-libraries:
	@if [ -f build/Makefile ]; then \
		echo "[Makefile] Filtering library targets to only those defined in CMake..."; \
		for f in $$(find _libraries/src -name CMakeLists.txt); do \
			n=$$(grep -E '^add_library' $$f | sed -E 's/add_library\(([^ ]+).*/\1/'); \
			for t in $$n; do \
				if grep -qE "^$${t}:" build/Makefile; then $(MAKE) -s build-$$t; fi; \
			done; \
		done; \
	else \
		echo "[Makefile] No build/Makefile found. Run 'make build' to configure CMake first."; \
	fi

build-%:
	@echo "Building: $*"
	@cd build && cmake --build . --target $* || echo "No CMake target $* found."

run:
	@echo "Running all executables (where possible)..."
	@for n in $$(grep -h -E '^add_executable' _binaries/apps/*/CMakeLists.txt 2>/dev/null | sed -E 's/add_executable\(([^ ]+).*/\1/'); do $(MAKE) -s run-$$n; done
	@for n in $$(grep -h -E '^add_executable' _binaries/demos/CMakeLists.txt 2>/dev/null | sed -E 's/add_executable\(([^ ]+).*/\1/'); do $(MAKE) -s run-$$n; done
	@for n in $$(grep -h -E '^add_executable' _binaries/services/*/CMakeLists.txt 2>/dev/null | sed -E 's/add_executable\(([^ ]+).*/\1/'); do $(MAKE) -s run-$$n; done

run-%:
	@echo "Running: $*"
	if [ ! -x "$*" ]; then \
		$(MAKE) -s build-$*; \
	fi; \
	if [ -x "$*" ]; then \
		./$*; \
	else \
		echo "Executable $* not found in PATH or current directory."; \
	fi

test: build
	@echo "Building tests in _test..."
	cmake -S _test -B build/_test
	cmake --build build/_test
	@echo "Running CTest for all registered tests..."
	@cd build/_test && ctest --output-on-failure

test-%:
	@echo "Testing: $*"
	if [ ! -x "$*" ]; then \
		$(MAKE) -s build-$*; \
	fi; \
	if [ -x "$*" ]; then \
		./$*; \
	else \
		echo "Test executable $* not found in PATH or current directory."; \
	fi

install:
	@echo "Installing all .dylib libraries to ./lib ..."
	@mkdir -p lib
	@find build -name '*.dylib' -exec cp -v {} lib/ \;

install-%:
	@echo "Installing library: $*"
	@mkdir -p lib
	@find build -name '$*.dylib' -exec cp -v {} lib/ \; || echo "No .dylib found for $*"

completion:
	@echo '# bash/zsh completion for make targets in this Makefile'
	@echo 'complete -W "$$(grep -oE "^[a-zA-Z0-9_-]+:|^[a-zA-Z0-9_-]+ +:" Makefile | sed "s/[: ].*//" | sort -u)" make'