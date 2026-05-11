# ToolBox Examples

This directory contains examples demonstrating various ToolBox library features organized by category.

## Directory Structure

### C++ Examples (`cpp/`)
C++ examples demonstrating core library features:

- **networking/** - REST API, JSON, and HTTP protocols
  - HTTP/1.1, HTTP/2, HTTP/3 servers
  - JSON parsing and building
  - Thread pool and async handling
  
- **ml/** - Machine learning
  - Model serving via REST API
  
- **sql/** - Database operations
  - Prisma schema parsing
  - SQL query generation

See [cpp/README.md](cpp/README.md) for build instructions.

### Python Examples (`python/`)
Python examples using the Python bindings:

- **ml/** - Machine learning examples
  - Deep learning training
  - LLM usage
  - Classification and NLP
  - Time series analysis
  
- **networking/** - Network and database
  - REST API client usage
  - Prisma/SQL operations
  
- **distributed/** - Distributed computing
  - Data parallel training
  - Parameter server patterns
  - Federated learning

See [python/README.md](python/README.md) for setup instructions.

## Quick Start

### C++ Examples

```bash
# Build the main library
./clean&build.sh

# Build and run C++ examples
cd examples/cpp
make all
make run-all
```

### Python Examples

```bash
# Install Python bindings
cd python_bindings
pip install .

# Run Python examples
cd ../examples/python/ml
python deep_learning_example.py
```

## Namespace Organization

The library uses the following namespace structure:

- `networking::json` - JSON library
- `networking::rest_api` - REST API servers
- `networking::distributed` - Distributed computing
- `ml::` - Machine learning algorithms
- `ml::sql` - SQL/database operations
- `ml::deep_learning` - Neural networks

## Additional Resources

- **schema.prisma** - Example Prisma schema file
- **time_series_example.py.bak** - Backup files (can be deleted)
- **training_visualization.py.bak\*** - Backup files (can be deleted)
