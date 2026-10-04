import numpy as np
import pytest

from ml_toolbox import graphics, synthetic_data


def test_graph3d_scatter_renders_to_numpy():
    graph = graphics.Graph3D(240, 180, graphics.Graph3DType.SCATTER)
    graph.set_title("Clusters")
    graph.set_x_label("feature 1")
    graph.set_y_label("feature 2")
    graph.set_z_label("feature 3")
    graph.set_view(35.0, 20.0)
    graph.add_series(
        graphics.DataSeries3D(
            "samples",
            [0.0, 1.0, 2.0],
            [1.0, 0.0, 2.0],
            [2.0, 1.0, 0.0],
            graphics.PURPLE,
        )
    )

    pixels = graph.render().to_numpy()
    assert pixels.shape == (180, 240, 4)
    assert np.any(pixels[:, :, :3] != 255)


def test_graph3d_line_validates_coordinates():
    graph = graphics.Graph3D(graph_type=graphics.Graph3DType.LINE)
    with pytest.raises(ValueError):
        graph.add_series(
            graphics.DataSeries3D("bad", [0.0, 1.0], [0.0], [0.0, 1.0])
        )


@pytest.mark.parametrize(
    ("factory", "kwargs", "expected_shape"),
    [
        (
            synthetic_data.make_regression,
            {"n_samples": 20, "n_features": 4, "noise": 0.1, "seed": 7},
            ((20, 4), (20,)),
        ),
        (
            synthetic_data.make_classification,
            {"n_samples": 24, "n_features": 3, "n_classes": 3, "seed": 7},
            ((24, 3), (24,)),
        ),
        (
            synthetic_data.make_blobs,
            {"n_samples": 30, "n_features": 3, "centers": 3, "seed": 7},
            ((30, 3), (30,)),
        ),
    ],
)
def test_synthetic_generators_are_deterministic(factory, kwargs, expected_shape):
    first_features, first_targets = factory(**kwargs)
    second_features, second_targets = factory(**kwargs)

    assert first_features.shape == expected_shape[0]
    assert first_targets.shape == expected_shape[1]
    np.testing.assert_array_equal(first_features, second_features)
    np.testing.assert_array_equal(first_targets, second_targets)


def test_synthetic_generators_validate_arguments():
    with pytest.raises(ValueError):
        synthetic_data.make_regression(n_samples=0)
    with pytest.raises(ValueError):
        synthetic_data.make_classification(n_samples=2, n_classes=3)
    with pytest.raises(ValueError):
        synthetic_data.make_blobs(cluster_std=-1.0)
