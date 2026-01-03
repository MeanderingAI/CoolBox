# Python Examples

This directory contains Python examples demonstrating the ToolBox library features.

## Directory Structure

- **ml/** - Machine learning examples
  - `deep_learning_example.py` - Neural network training
  - `llm_example.py` - Large language model usage
  - `multiclass_classification.py` - Multi-class classification
  - `nlp_example.py` - Natural language processing
  - `time_series_example.py` - Time series analysis
  - `training_visualization.py` - Training metrics visualization
  - `simple_training_viz.py` - Simple visualization utilities

- **networking/** - Network and database examples
  - `rest_api_example.py` - REST API client usage
  - `prisma_sql_example.py` - Prisma schema and SQL operations

- **distributed/** - Distributed computing examples
  - `distributed_training_demo.py` - Distributed ML training patterns
  - `deep_learning_example.cpp` - C++ deep learning example
  - `distributed_data_parallel.png` - Visualization

## Installation

First, install the Python bindings:

```bash
cd ../../python_bindings
pip install .
```

## Running Examples

### ML Examples

```bash
cd ml
python deep_learning_example.py
python llm_example.py
python multiclass_classification.py
```

### Networking Examples

```bash
cd networking
python rest_api_example.py
python prisma_sql_example.py
```

### Distributed Examples

```bash
cd distributed
python distributed_training_demo.py
```

## Requirements

- Python 3.7+
- NumPy
- matplotlib (for visualizations)
- ml-toolbox Python package (installed from python_bindings/)
