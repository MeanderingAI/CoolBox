# Synthetic Data Python Bindings

## Overview

The v3.1.2 native extension adds deterministic dataset generators under
`ml_toolbox.synthetic_data`. Every generator returns NumPy arrays and accepts
a `seed` value for reproducible output.

Available generators:

- `make_regression(...) -> (X, y)`
- `make_classification(...) -> (X, labels)`
- `make_blobs(...) -> (X, labels)`

## Install with pip

Install directly from GitHub:

```bash
python -m pip install \
  "git+https://github.com/MeanderingAI/CoolBox.git#subdirectory=_deliverables/libraries/bindings/python_bindings"
```

Or install the current checkout:

```bash
cd CoolBox/_deliverables/libraries/bindings/python_bindings
python -m pip install .
```

## Generate regression data

```bash
python - <<'PY'
from ml_toolbox import synthetic_data

X, y = synthetic_data.make_regression(
    n_samples=100,
    n_features=4,
    noise=0.25,
    bias=2.0,
    seed=42,
)
print("X:", X.shape, X.dtype)
print("y:", y.shape, y.dtype)
print("first target:", y[0])
PY
```

## Generate classification data

```bash
python - <<'PY'
import numpy as np

from ml_toolbox import synthetic_data

X, labels = synthetic_data.make_classification(
    n_samples=120,
    n_features=3,
    n_classes=3,
    class_sep=2.5,
    seed=42,
)
print("X:", X.shape)
print("labels:", np.bincount(labels))
PY
```

## Generate blobs and render them in 3D

```bash
python - <<'PY'
from ml_toolbox import graphics, synthetic_data

X, labels = synthetic_data.make_blobs(
    n_samples=150,
    centers=3,
    n_features=3,
    cluster_std=0.8,
    seed=42,
)

colors = [graphics.RED, graphics.GREEN, graphics.BLUE]
plot = graphics.Graph3D(800, 600, graphics.Graph3DType.SCATTER)
plot.set_title("Synthetic 3D blobs")

for label, color in enumerate(colors):
    points = X[labels == label]
    plot.add_series(
        graphics.DataSeries3D(
            f"cluster {label}",
            points[:, 0].tolist(),
            points[:, 1].tolist(),
            points[:, 2].tolist(),
            color,
        )
    )

plot.render().save_png("synthetic_blobs_3d.png")
print("wrote synthetic_blobs_3d.png")
PY
```

## Input rules

- Sample, feature, class, and center counts must be positive.
- Classification requires at least two classes.
- Classes or centers cannot exceed the number of samples.
- `noise` and `cluster_std` must be non-negative.
- `class_sep` must be positive.
