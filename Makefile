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
	@echo ""
	@echo "━━━ Discovered Library Targets ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "  [IO]                                      make build_libraries_io"
	@find _libraries/backages/IO -name CMakeLists.txt -exec grep -h -E '^add_library' {} + 2>/dev/null \
		| sed -E 's/add_library\(([^ )]+).*/    make build_\1/' | sort -u || echo "    (none found)"
	@echo ""
	@echo "  [ML]                                      make build_libraries_ml"
	@find _libraries/backages/ML -name CMakeLists.txt -exec grep -h -E '^add_library' {} + 2>/dev/null \
		| sed -E 's/add_library\(([^ )]+).*/    make build_\1/' | sort -u || echo "    (none found)"
	@echo ""
	@echo "  [Security]                                make build_libraries_security"
	@find _libraries/backages/security -name CMakeLists.txt -exec grep -h -E '^add_library' {} + 2>/dev/null \
		| sed -E 's/add_library\(([^ )]+).*/    make build_\1/' | sort -u || echo "    (none found)"
	@echo ""
	@echo "  [Electronics]                             make build_libraries_electronics"
	@find _libraries/backages/ELECTRONICS -name CMakeLists.txt -exec grep -h -E '^add_library' {} + 2>/dev/null \
		| sed -E 's/add_library\(([^ )]+).*/    make build_\1/' | sort -u || echo "    (none found)"
	@echo ""
	@echo "  [Misc]                                    make build_libraries_misc"
	@find _libraries/backages/MISC -name CMakeLists.txt -exec grep -h -E '^add_library' {} + 2>/dev/null \
		| sed -E 's/add_library\(([^ )]+).*/    make build_\1/' | sort -u || echo "    (none found)"
	@echo ""
	@echo "━━━ Discovered Test Targets ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@find _libraries/backages -name CMakeLists.txt -exec grep -h -E '^add_executable.*_tests' {} + 2>/dev/null \
		| sed -E 's/add_executable\(([^ )]+).*/    make test-\1/' | sort -u || echo "    (none found)"
	@echo ""
	@echo "══════════════════════════════════════════════════════════════"
	@echo ""

.PHONY: all help configure build clean test install completion \
        build_libraries build_libraries_io build_libraries_ml \
        build_libraries_security build_libraries_misc build_libraries_electronics

all: help

# ── CMake Configuration (libraries only, skip binaries) ─────────────
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
	rm -rf build lib
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

# Build a specific target by name
build_%: configure
	@echo "Building: $*"
	@cd build && cmake --build . --target $* || echo "No CMake target '$*' found."

# ── Tests ───────────────────────────────────────────────────────────
test: configure
	@echo "Running all CTest suites..."
	@cd build && ctest --output-on-failure || true

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
