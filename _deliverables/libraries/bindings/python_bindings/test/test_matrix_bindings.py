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


def test_cpu_thread_limit_can_be_configured():
    try:
        matrix.set_cpu_threads(0)
        assert matrix.cpu_threads() >= 1
        for thread_count in (1, 2):
            matrix.set_cpu_threads(thread_count)
            assert matrix.cpu_threads() == thread_count
    finally:
        matrix.set_cpu_threads(0)


@pytest.mark.parametrize(
    "backend", [matrix.MatrixBackend.CPU, matrix.MatrixBackend.EIGEN]
)
def test_odd_rectangular_multiply_respects_thread_budget(backend):
    np = pytest.importorskip("numpy")
    rng = np.random.default_rng(193129173)
    left_values = rng.standard_normal((193, 1025))
    right_values = rng.standard_normal((1025, 683))
    reference = left_values @ right_values
    left = matrix.DenseMatrix(193, 1025, left_values.ravel().tolist())
    right = matrix.DenseMatrix(1025, 683, right_values.ravel().tolist())
    original_backend = matrix.get_backend()
    try:
        matrix.set_backend(backend)
        matrix.set_cpu_threads(1)
        serial_result = np.asarray(left.multiply(right).data).reshape(193, 683)
        matrix.set_cpu_threads(4)
        threaded = left.multiply(right)
        threaded_result = np.asarray(threaded.data).reshape(193, 683)
        assert matrix.get_active_backend() == backend
        assert np.allclose(serial_result, reference, rtol=1e-10, atol=1e-10)
        assert np.allclose(threaded_result, reference, rtol=1e-10, atol=1e-10)
    finally:
        matrix.set_cpu_threads(0)
        matrix.set_backend(original_backend)


def test_auto_backend_resolves_to_an_available_backend():
    matrix.set_backend(matrix.MatrixBackend.AUTO)
    try:
        left = matrix.DenseMatrix.identity(2)
        left.multiply(left)
        active = matrix.get_active_backend()
        assert active != matrix.MatrixBackend.AUTO
        assert matrix.backend_available(active)
    finally:
        matrix.set_backend(matrix.MatrixBackend.CPU)


def test_openblas_availability_and_selection():
    original_backend = matrix.get_backend()
    try:
        assert matrix.backend_name(matrix.MatrixBackend.OPENBLAS) == "openblas"
        if matrix.backend_available(matrix.MatrixBackend.OPENBLAS):
            matrix.set_backend(matrix.MatrixBackend.OPENBLAS)
            assert matrix.get_backend() == matrix.MatrixBackend.OPENBLAS
        else:
            with pytest.raises(ValueError, match="unavailable"):
                matrix.set_backend(matrix.MatrixBackend.OPENBLAS)
            assert matrix.get_backend() == original_backend
    finally:
        matrix.set_backend(original_backend)


@pytest.mark.parametrize("shape", [(1, 3, 5), (7, 19, 13), (129, 137, 131)])
def test_openblas_products(shape):
    if not matrix.backend_available(matrix.MatrixBackend.OPENBLAS):
        pytest.skip("OpenBLAS was not enabled in this build")
    np = pytest.importorskip("numpy")
    m, k, n = shape
    rng = np.random.default_rng(32)
    a, b = rng.standard_normal((m, k)), rng.standard_normal((k, n))
    left = matrix.DenseMatrix(m, k, a.ravel().tolist())
    right = matrix.DenseMatrix(k, n, b.ravel().tolist())
    original_backend = matrix.get_backend()
    try:
        matrix.set_backend(matrix.MatrixBackend.OPENBLAS)
        for threads in (1, 2, 8):
            matrix.set_cpu_threads(threads)
            actual = np.asarray((left @ right).data).reshape(m, n)
            assert matrix.get_active_backend() == matrix.MatrixBackend.OPENBLAS
            assert np.allclose(actual, a @ b, rtol=1e-10, atol=1e-10)
        with pytest.raises(ValueError, match="dimension mismatch"):
            left.multiply(matrix.DenseMatrix(k + 1, n))
        left.transpose()
        assert matrix.get_active_backend() == matrix.MatrixBackend.CPU
        assert matrix.get_backend() == matrix.MatrixBackend.OPENBLAS
    finally:
        matrix.set_cpu_threads(0)
        matrix.set_backend(original_backend)
