# CoolBox v3.1.2 Release Notes

CoolBox v3.1.2 expands the native Python extension with projected 3D plotting
and deterministic synthetic dataset generation.

## Highlights

- Native `graphics.Graph3D` line and scatter plots.
- Configurable azimuth, elevation, labels, colors, and point radius.
- PNG, JPG, BMP, and NumPy output through the existing native `Canvas`.
- Native regression, classification, and Gaussian-blob dataset generators.
- Reproducible dataset generation through explicit random seeds.
- Package-level imports through `ml_toolbox.graphics` and
  `ml_toolbox.synthetic_data`.

## Guides

- [3D plotting Python extension](./README_3D_PLOTS.md)
- [Synthetic data Python bindings](./README_SYNTHETIC_DATA.md)

Both features are compiled into `ml_toolbox.ml_core`; they do not require
Matplotlib or scikit-learn at runtime.
