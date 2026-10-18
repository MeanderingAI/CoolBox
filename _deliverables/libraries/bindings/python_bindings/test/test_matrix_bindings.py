import pytest

try:
    from ml_toolbox import matrix
except ImportError as error:
    pytest.skip(f"CoolBox Python extension is not built: {error}", allow_module_level=True)


@pytest.mark.parametrize(
    "backend", [matrix.MatrixBackend.CPU, matrix.MatrixBackend.EIGEN]
)
def test_dense_matrix_operations(backend):
    matrix.set_backend(backend)
    try:
        left = matrix.DenseMatrix.from_rows([[1.0, 2.0], [3.0, 4.0]])
        right = matrix.DenseMatrix.from_rows([[5.0, 6.0], [7.0, 8.0]])

        assert left.shape == (2, 2)
        assert (left + right).data == [6.0, 8.0, 10.0, 12.0]
        assert (left - right).data == [-4.0, -4.0, -4.0, -4.0]
        assert (left @ right).data == [19.0, 22.0, 43.0, 50.0]
        assert left.transpose().data == [1.0, 3.0, 2.0, 4.0]
        assert left.rowwise_add(matrix.DenseVector([10.0, 20.0])).data == [
            11.0, 22.0, 13.0, 24.0
        ]
        assert left.norm() == pytest.approx(30.0**0.5)
        assert matrix.get_backend() == backend
        assert matrix.get_active_backend() == backend
    finally:
        matrix.set_backend(matrix.MatrixBackend.CPU)


def test_matrix_backend_metadata():
    assert matrix.backend_name(matrix.MatrixBackend.EIGEN) == "eigen"
    assert matrix.backend_available(matrix.MatrixBackend.EIGEN)
    assert not matrix.backend_available(matrix.MatrixBackend.OPENCL)
    with pytest.raises(ValueError, match="unavailable"):
        matrix.set_backend(matrix.MatrixBackend.OPENCL)


@pytest.mark.parametrize(
    "backend", [matrix.MatrixBackend.CPU, matrix.MatrixBackend.EIGEN]
)
def test_dense_vector_optimized_helpers(backend):
    matrix.set_backend(backend)
    first = matrix.DenseVector([1.0, 2.0, 3.0])
    second = matrix.DenseVector([4.0, 5.0, 6.0])

    try:
        assert first.dot(second, matrix.CpuOptimization.SCALAR) == pytest.approx(32.0)
        assert first.add(second).data == [5.0, 7.0, 9.0]
    finally:
        matrix.set_backend(matrix.MatrixBackend.CPU)


def test_matrix_dimension_errors_are_reported():
    first = matrix.DenseMatrix.from_rows([[1.0, 2.0]])
    other = matrix.DenseMatrix.from_rows([[1.0, 2.0]])

    with pytest.raises(ValueError, match="dimension mismatch"):
        first.multiply(other)
