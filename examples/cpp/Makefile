# Makefile for C++ REST API Examples
# Requires: CMake build to be completed first (libraries must be built)

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2
INCLUDES = -I../include
LIBDIRS = -L../build/src/json -L../build/src/rest_api -L../build/src/sql
LIBS = -lrest_api -ljson -lsql -lpthread -lsqlite3

# Build directory
BUILD_DIR = ../build/examples

# Targets
TARGETS = basic_server json_example threaded_server model_server prisma_sql http_protocols

# Default target
all: check_libs $(TARGETS)
	@echo ""
	@echo "✓ All examples compiled successfully!"
	@echo ""
	@echo "Run 'make run-all' to execute all examples"
	@echo "Or run individual examples:"
	@echo "  make run-basic        - Basic server example"
	@echo "  make run-json         - JSON library example"
	@echo "  make run-threaded     - Threaded server example"
	@echo "  make run-model        - Model server example"
	@echo "  make run-prisma       - Prisma SQL example"
	@echo ""

# Check if libraries are built
check_li  make run-protocols    - HTTP protocols comparison"
	@echo "bs:
	@if [ ! -f ../build/src/json/libjson.so ] || [ ! -f ../build/src/rest_api/librest_api.so ] || [ ! -f ../build/src/sql/libsql.so ]; then \
		echo "Error: Libraries not found. Please build the project first:"; \
		echo "  cd .. && ./clean&build.sh"; \
		exit 1; \
	fi
	@mkdir -p $(BUILD_DIR)

# Build targets
basic_server: networking/basic_server_example.cpp
	@echo "Compiling basic_server_example..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(LIBDIRS) $(LIBS) -o $(BUILD_DIR)/$@

json_example: networking/json_example.cpp
	@echo "Compiling json_example..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(LIBDIRS) $(LIBS) -o $(BUILD_DIR)/$@

threaded_server: networking/threaded_server_example.cpp
	@echo "Compiling threaded_server_example..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(LIBDIRS) $(LIBS) -o $(BUILD_DIR)/$@

model_server: ml/model_server_example.cpp
	@echo "Compiling model_server_example..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(LIBDIRS) $(LIBS) -o $(BUILD_DIR)/$@
prisma_sql: sql/prisma_sql_example.cpp
	@echo "Compiling prisma_sql_example..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(LIBDIRS) $(LIBS) -o $(BUILD_DIR)/$@
http_protocols: networking/http_protocols_example.cpp
	@echo "Compiling http_protocols_example..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(LIBDIRS) $(LIBS) -o $(BUILD_DIR)/$@


# Run targets
run-basic: basic_server
	@echo "Running basic_server_example..."
	@echo "================================"
	@cd $(BUILD_DIR) && LD_LIBRARY_PATH=../../src/json:../../src/rest_api:$$LD_LIBRARY_PATH ./basic_server

run-json: json_example
	@echo "Running json_example..."
	@echo "======================="
	@cd $(BUILD_DIR) && LD_LIBRARY_PATH=../../src/json:../../src/rest_api:$$LD_LIBRARY_PATH ./json_example

run-threaded: threaded_server
	@echo "Running threaded_server_example..."
	@echo "==================================="
	@cd $(BUILD_DIR) && LD_LIBRARY_PATH=../../src/json:../../src/rest_api:$$LD_LIBRARY_PATH ./threaded_server
../../src/sql:$$LD_LIBRARY_PATH ./model_server


run-protocols: http_protocols
	@echo "Running http_protocols_example..."
	@echo "=================================="
	@cd $(BUILD_DIR) && LD_LIBRARY_PATH=../../src/json:../../src/rest_api:../../src/sql:$$LD_LIBRARY_PATH ./http_protocols

run-prisma: prisma_sql
	@echo "Running prisma_sql_example..."
	@echo "=============================="
	@cd $(BUILD_DIR) && LD_LIBRARY_PATH=../../src/json:../../src/rest_api:../../src/sql:$$LD_LIBRARY_PATH ./prisma_sql
run-model: model_server
	@echo "Runnin
run-all: all
	@echo ""
	@echo "=========================================="
	@echo "Running all examples..."
	@echo "=========================================="
	@$(MAKE) run-prisma
	@echo ""
	@echo ""
	@$(MAKE) run-basic
	@echo ""
	@$(MAKE) run-prisma
	@echo ""
	@$(MAKE) run-protocols
	@echo ""
	@$(MAKE) run-json
	@echo ""
	@$(MAKE) run-threaded
	@echo ""
	@$(MAKE) run-model
	@echo ""
	@echo "=========================================="
	@echo "All examples completed!"
	@echo "=========================================="

# CleanBuild examples:"
	@echo "  make all             - Build all examples"
	@echo "  make basic_server    - Basic server with GET/POST routes"
	@echo "  make json_example    - JSON library usage"
	@echo "  make threaded_server - Thread pool and async requests"
	@echo "  make model_server    - ML model serving"
	@echo "  make prisma_sql      - Prisma schema and SQL CRUD"
	@echo "  make http_protocols  - HTTP/1.1, HTTP/2, HTTP/3 comparison"
	@echo ""
	@echo "Run examples:"
	@echo "  make run-all       - Run all examples in sequence"
	@echo "  make run-basic     - Run basic_server_example"
	@echo "  make run-json      - Run json_example"
	@echo "  make run-threaded  - Run threaded_server_example"
	@echo "  make run-model     - Run model_server_example"
	@echo "  make run-prisma    - Run prisma_sql_example"
	@echo "  make run-protocols - Run http_protocols_example"
	@echo ""
	@echo "Prerequisites:"
	@echo "  - Libraries must be built first (cd .. && ./clean&build.sh)"
	@echo "  - Requires C++17 compiler (g++, clang++)"
	@echo ""

.PHONY: all check_libs clean help run-basic run-json run-threaded run-model run-prisma run-protocols
	@echo "  make run-json     - Run json_example"
	@echo "  make run-threaded - Run threaded_server_example"
	@echo "  make run-model    - Run model_server_example"
	@echo ""
	@echo "Prerequisites:"
	@echo "  - Libraries must be built first (cd .. && ./clean&build.sh)"
	@echo "  - Requires C++17 compiler (g++, clang++)"
	@echo ""

.PHONY: all check_libs clean help run-basic run-json run-threaded run-model run-all
