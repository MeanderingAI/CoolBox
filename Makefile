# Set Python executable
PYTHON := $(if $(wildcard .venv/bin/python),$(CURDIR)/.venv/bin/python,python3)

# Emscripten SDK location (override with EMSDK=/your/path)
EMSDK ?= $(HOME)/emsdk

# Ensure pybind11 is installed before building Python bindings
install_pybind11:
	@echo "Checking for setuptools and pybind11..."
	@$(PYTHON) -c "import setuptools" 2>/dev/null || (echo "Installing setuptools..." && $(PYTHON) -m pip install --break-system-packages setuptools)
	@$(PYTHON) -c "import pybind11" 2>/dev/null || (echo "Installing pybind11..." && $(PYTHON) -m pip install --break-system-packages pybind11)

# Install Emscripten SDK and ensure emcmake is on PATH
install_emcmake:
	@if command -v emcmake >/dev/null 2>&1; then \
		echo "emcmake is already available:"; \
		emcc --version | head -1; \
	elif [ -f "$(EMSDK)/emsdk_env.sh" ]; then \
		echo "emsdk found at $(EMSDK), activating..."; \
		. "$(EMSDK)/emsdk_env.sh" 2>/dev/null; \
	else \
		echo "Installing Emscripten SDK to $(EMSDK) ..."; \
		git clone https://github.com/emscripten-core/emsdk.git "$(EMSDK)" && \
		cd "$(EMSDK)" && ./emsdk install latest && ./emsdk activate latest; \
		echo ""; \
		echo "✓ Emscripten SDK installed to $(EMSDK)"; \
		echo "  To use, first run:"; \
		echo "    source $(EMSDK)/emsdk_env.sh"; \
		echo "  Then:"; \
		echo "    make build-emscripten"; \
	fi
# Set global DYLD_LIBRARY_PATH for all run/install commands
DYLD_LIBRARY_PATH := $(CURDIR)/lib:$(DYLD_LIBRARY_PATH)

.DEFAULT_GOAL := help

help:
	@echo ""
	@echo "╔══════════════════════════════════════════════════════════════╗"
	@echo "║                    CoolBox Build System                     ║"
	@echo "╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "━━━ Build Commands ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "  make build                       Configure CMake & show targets"
	@echo "  make build_libraries             Build all libraries"
	@echo "  make build_libraries_io          Build IO libraries only"
	@echo "  make build_libraries_ml          Build ML libraries only"
	@echo "  make build_libraries_security    Build Security libraries only"
	@echo "  make build_libraries_misc        Build Misc libraries only"
	@echo "  make build_libraries_graphics    Build Graphics libraries only"
	@echo "  make build_libraries_electronics Build Electronics libraries only"
	@echo "  make build_NAME                  Build a specific target by name"
	@echo ""
	@echo "━━━ Test Commands ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "  make test               Run all registered CTest suites"
	@echo "  make test-NAME          Run a specific test by name"
	@echo ""
	@echo "━━━ Install / Utility ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "  make install            Install all .dylib libraries to ./lib"
	@echo "  make install-NAME       Install a specific library"
	@echo "  make configure          Run CMake configuration"
	@echo "  make clean              Remove all build artifacts"
	@echo "  make completion         Output shell completion script"
	@echo "  make install_tutorial_editor_deps Install tutorial editor Python dependencies"
	@echo "  make launch_editor      Launch the tutorial editor app"
	@echo "  make build_c_bindings   Build the plain C bindings"
	@echo "  make build_java_bindings Build the plain Java bindings"
	@echo ""
	@echo "━━━ Discovered Library Targets ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "  [IO]                                      make build_libraries_io"
	@echo "    (skipped: requires Unix find/grep/sed; run in WSL/Git-Bash to enable)"
	@echo ""
	@echo "  [ML]                                      make build_libraries_ml"
	@echo "    (skipped: requires Unix find/grep/sed; run in WSL/Git-Bash to enable)"
	@echo ""
	@echo "  [Security]                                make build_libraries_security"
	@echo "    (skipped: requires Unix find/grep/sed; run in WSL/Git-Bash to enable)"
	@echo ""
	@echo "  [Graphics]                                make build_libraries_graphics"
	@echo "    (skipped: requires Unix find/grep/sed; run in WSL/Git-Bash to enable)"
	@echo ""
	@echo "  [Electronics]                             make build_libraries_electronics"
	@echo "    (skipped: requires Unix find/grep/sed; run in WSL/Git-Bash to enable)"
	@echo ""
	@echo "  [Misc]                                    make build_libraries_misc"
	@echo "    (skipped: requires Unix find/grep/sed; run in WSL/Git-Bash to enable)"
	@echo ""
	@echo "━━━ Discovered Test Targets ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "    (skipped: requires Unix find/grep/sed; run in WSL/Git-Bash to enable)"
	@echo ""
	@echo "══════════════════════════════════════════════════════════════"
	@echo ""

.PHONY: all help configure build build_all clean test install completion \
        build_libraries build_libraries_io build_libraries_ml \
        build_libraries_security build_libraries_misc build_libraries_electronics \
        build_libraries_graphics \
        build-emscripten build_js_bindings clean_js_bindings install_js_bindings \
		build_c_bindings \
		install_tutorial_editor_deps \
		build_java_bindings \
	build_python_bindings clean_python_bindings install_python_bindings install_pybind11 \
	document_r_bindings build_r_bindings install_r_bindings site_r_bindings \
		build_rust_bindings test_rust_bindings build_docs_portal launch_editor

all: build_all

# ── Build All (every CMake target: libraries + tests) ───────────────
build_all: configure
	@echo "[Makefile] Building ALL targets..."
	@cmake --build build
	@echo "[Makefile] Build complete."

# ── CMake Configuration (libraries only, skip binaries) ─────────────
# Configure CMake (libraries only). To enable/disable SQL backage,
# pass -DBUILD_IO_SQL=ON/OFF on the command line when running cmake.
configure:
	@if [ ! -f build/Makefile ]; then \
		echo "[Makefile] Running CMake configuration (libraries only)..."; \
		cmake -S . -B build -DBUILD_BINARIES=OFF; \
	else \
		echo "[Makefile] Build already configured (build/Makefile exists)."; \
	fi

build: configure
	@$(MAKE) --no-print-directory help

clean:
	@echo "Cleaning build artifacts..."
	rm -rf build build_lsp lib .site .local-cpp-docs .r-library .r-makevars.local .documentation
	@echo "Clean complete."

# ── Library Builds ──────────────────────────────────────────────────
build_libraries: configure
	@echo "[Makefile] Building all libraries..."
	@for f in $$(find _libraries/backages -name CMakeLists.txt); do \
		for t in $$(grep -E '^add_library' $$f 2>/dev/null | grep -v 'INTERFACE' | sed -E 's/add_library\(([^ ]+).*/\1/'); do \
			echo "  → $$t"; \
			cd build && cmake --build . --target $$t 2>&1 | tail -3 || true; cd ..; \
		done; \
	done
	@echo ""
	@echo "  (Header-only / INTERFACE libraries need no build step)"

# Category-specific library builds
define BUILD_LIBS_IN
	@echo "[Makefile] Building $(1) libraries..."
	@for f in $$(find _libraries/backages/$(1) -name CMakeLists.txt 2>/dev/null); do \
		for t in $$(grep -E '^add_library' $$f 2>/dev/null | grep -v 'INTERFACE' | sed -E 's/add_library\(([^ ]+).*/\1/'); do \
			echo "  → $$t"; \
			cd build && cmake --build . --target $$t || true; cd ..; \
		done; \
	done
	@echo ""
	@echo "  (Header-only / INTERFACE libraries need no build step)"
endef

build_libraries_io: configure
	$(call BUILD_LIBS_IN,IO)

build_libraries_ml: configure
	$(call BUILD_LIBS_IN,ML)

build_libraries_security: configure
	$(call BUILD_LIBS_IN,security)

build_libraries_misc: configure
	$(call BUILD_LIBS_IN,MISC)

build_libraries_electronics: configure
	$(call BUILD_LIBS_IN,ELECTRONICS)

build_libraries_graphics: configure
	$(call BUILD_LIBS_IN,GRAPHICS)

# Build a specific target by name
build_%: configure
	@echo "Building: $*"
	@cd build && cmake --build . --target $* || echo "No CMake target '$*' found."

# ── Tests ───────────────────────────────────────────────────────────
test: configure
	@echo "Building all test executables..."
	@for f in $$(find _libraries/backages -name CMakeLists.txt); do \
		for t in $$(grep -E 'add_executable.*_tests' $$f 2>/dev/null | sed -E 's/.*add_executable\(([^ ]+).*/\1/'); do \
			echo "  → building $$t"; \
			cd build && cmake --build . --target $$t 2>&1 | tail -3 || true; cd ..; \
		done; \
	done
	@echo ""
	@echo "Running all CTest suites (library tests only)..."
	@cd build && ctest --output-on-failure -R "Tests$$" || true

test-%: configure
	@echo "Building & running test: $*"
	@cd build && cmake --build . --target $* && ./$* || \
		(find . -name "$*" -type f -perm +111 -exec {} \;)

# ── Install ─────────────────────────────────────────────────────────
install:
	@echo "Installing all .dylib libraries to ./lib ..."
	@mkdir -p lib
	@find build -name '*.dylib' -exec cp -v {} lib/ \;

install-%:
	@echo "Installing library: $*"
	@mkdir -p lib
	@find build -name "lib$*.dylib" -o -name "$*.dylib" 2>/dev/null | head -1 | xargs -I{} cp -v {} lib/ || echo "No .dylib found for $*"

# ── Completion ──────────────────────────────────────────────────────
completion:
	@echo '# bash/zsh completion for make targets in this Makefile'
	@echo 'complete -W "$$(grep -oE "^[a-zA-Z0-9_-]+:" Makefile | sed "s/://" | sort -u)" make'

# ── Python Bindings ───────────────────────────────────────────────
build_python_bindings: install_pybind11
	@echo "Building Python bindings..."
	@cd _libraries/python_bindings && $(PYTHON) setup.py build

clean_python_bindings:
	@echo "Cleaning Python bindings build artifacts..."
	@cd _libraries/python_bindings && $(PYTHON) setup.py clean --all

install_python_bindings:
	@echo "Installing Python bindings..."
	@cd _libraries/python_bindings && $(PYTHON) setup.py install

install_tutorial_editor_deps:
	@echo "Checking tutorial editor dependencies..."
	@$(PYTHON) -c "import tkinterdnd2" 2>/dev/null || (echo "Installing tkinterdnd2..." && $(PYTHON) -m pip install tkinterdnd2)

launch_editor: install_tutorial_editor_deps
	@echo "Launching tutorial editor..."
	@$(PYTHON) apps/tutorial_editor/tutorial_editor.py

build_c_bindings:
	@echo "Building plain C bindings..."
	@cmake -S _libraries/c_bindings -B _libraries/c_bindings/build -DBUILD_TESTING=ON
	@cmake --build _libraries/c_bindings/build --config Release
	@ctest --test-dir _libraries/c_bindings/build --output-on-failure

build_java_bindings:
	@echo "Building plain Java bindings..."
	@mvn -f _libraries/java_bindings/pom.xml test package javadoc:javadoc

# ── Emscripten / JavaScript Bindings ────────────────────────────────
build_js_bindings: install_emcmake
	@echo "========================================"
	@echo "Building Emscripten JS/WASM bindings"
	@echo "========================================"
	@mkdir -p build-emscripten
	@if command -v emcmake >/dev/null 2>&1; then \
		cd build-emscripten && emcmake cmake -G "Unix Makefiles" \
			-DCMAKE_BUILD_TYPE=Release \
			-DEMSCRIPTEN_MODULARIZE=$(or $(EMS_MODULARIZE),ON) \
			../_libraries/emscripten_bindings && \
			$(MAKE); \
	elif [ -f "$(EMSDK)/emsdk_env.sh" ]; then \
		cd build-emscripten && . "$(EMSDK)/emsdk_env.sh" && emcmake cmake -G "Unix Makefiles" \
			-DCMAKE_BUILD_TYPE=Release \
			-DEMSCRIPTEN_MODULARIZE=$(or $(EMS_MODULARIZE),ON) \
			../_libraries/emscripten_bindings && \
			$(MAKE); \
	else \
		echo "Error: emcmake not found and no emsdk at $(EMSDK)."; \
		echo "  Run: make install_emcmake"; \
		echo "  Then: source $(EMSDK)/emsdk_env.sh"; \
		exit 1; \
	fi
	@echo "✓ Emscripten bindings built! (MODULARIZE=$(or $(EMS_MODULARIZE),ON))"
	@ls -lh build-emscripten/*.js 2>/dev/null || echo "(no .js files found)"
	@echo ""

clean_js_bindings:
	@echo "Cleaning Emscripten build artifacts..."
	rm -rf build-emscripten
	@echo "Clean complete."

install_js_bindings:
	@echo "Installing JS bindings to ./lib/js ..."
	@mkdir -p lib/js
	@find build-emscripten -name '*.js' -exec cp -v {} lib/js/ \; 2>/dev/null || echo "No .js files found. Run 'make build_js_bindings' first."
	@find build-emscripten -name '*.wasm' -exec cp -v {} lib/js/ \; 2>/dev/null || true

# ── R Bindings / Docs ───────────────────────────────────────────────
document_r_bindings:
	@echo "Generating roxygen2 docs for R bindings..."
	@Rscript -e "devtools::document('_libraries/r_bindings/coolboxr')"

build_r_bindings:
	@echo "Building R package bundle..."
	@R CMD build _libraries/r_bindings/coolboxr

install_r_bindings:
	@echo "Installing R bindings package..."
	@R CMD INSTALL _libraries/r_bindings/coolboxr

site_r_bindings:
	@echo "Building pkgdown site for R bindings..."
	@Rscript -e "pkgdown::build_site('_libraries/r_bindings/coolboxr')"

# ── Rust Bindings ───────────────────────────────────────────────────
build_rust_bindings:
	@echo "Building Rust bindings..."
	@cargo build --manifest-path _libraries/rust_bindings/Cargo.toml --release

test_rust_bindings:
	@echo "Testing Rust bindings..."
	@cargo test --manifest-path _libraries/rust_bindings/Cargo.toml --release

# ── Static Site Generation ─────────────────────────────────────────
# Build the full documentation site via a single entry-point script.
# The script builds tutorials into build/documentation/, generates the
# docs hub portal under .site/, and populates references and tag pages.
build_site:
	@bash ./_scripts/build_documentation.sh "$(CURDIR)"
