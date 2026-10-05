# 3D Plotting Python Extension

## Overview

The v3.1.2 Python extension adds native projected 3D plots under
`ml_toolbox.graphics`. Rendering uses the CoolBox C++ `Canvas`, so the result
can be saved as PNG, JPG, or BMP, or copied into a NumPy RGBA array.

Supported plot types:

- `graphics.Graph3DType.SCATTER`
- `graphics.Graph3DType.LINE`

## Install with pip

Install directly from GitHub:

```bash
python -m pip install \
  "git+https://github.com/MeanderingAI/CoolBox.git#subdirectory=_deliverables/libraries/bindings/python_bindings"
```

Or install from an existing checkout:

```bash
git clone https://github.com/MeanderingAI/CoolBox.git
cd CoolBox/_deliverables/libraries/bindings/python_bindings
python -m pip install .
```

For an editable developer installation:

```bash
python -m pip install -e .
```

## Verify the extension

```bash
python -c "from ml_toolbox import graphics; print(graphics.Graph3DType.SCATTER)"
```

## Create a 3D scatter plot

```bash
python - <<'PY'
from ml_toolbox import graphics

plot = graphics.Graph3D(800, 600, graphics.Graph3DType.SCATTER)
plot.set_title("Native 3D scatter")
plot.set_x_label("X")
plot.set_y_label("Y")
plot.set_z_label("Z")
plot.set_view(40.0, 25.0)
plot.set_point_radius(5)
plot.add_series(
    graphics.DataSeries3D(
        "samples",
        [0.0, 1.0, 2.0, 3.0],
        [0.0, 1.0, 0.5, 2.0],
        [0.0, 0.5, 2.0, 1.5],
        graphics.PURPLE,
    )
)

canvas = plot.render()
if not canvas.save_png("scatter3d.png"):
    raise RuntimeError("Failed to write scatter3d.png")
print("wrote scatter3d.png", canvas.to_numpy().shape)
PY
```

## Create a 3D line plot

```bash
python - <<'PY'
import math

from ml_toolbox import graphics

t = [index * 0.15 for index in range(80)]
x = [math.cos(value) for value in t]
y = [math.sin(value) for value in t]
z = [value / 5.0 for value in t]

plot = graphics.Graph3D(800, 600, graphics.Graph3DType.LINE)
plot.set_title("3D helix")
plot.set_view(45.0, 20.0)
plot.add_series(graphics.DataSeries3D("helix", x, y, z, graphics.BLUE))
plot.render().save_png("helix3d.png")
print("wrote helix3d.png")
PY
```

`set_view(azimuth_degrees, elevation_degrees)` uses an orthographic
projection. Elevation must be between -90 and 90 degrees.
