use coolbox_rs::{CpuOptimization, DenseMatrix, DenseVector, MatrixBackend};
use std::sync::Mutex;

static MATRIX_BACKEND_LOCK: Mutex<()> = Mutex::new(());

#[test]
fn cpu_thread_budget() {
    let _guard = MATRIX_BACKEND_LOCK.lock().unwrap();
    coolbox_rs::set_cpu_threads(0);
    assert!(coolbox_rs::cpu_threads() >= 1);
    for backend in [MatrixBackend::Cpu, MatrixBackend::Eigen] {
        backend.set().unwrap();
        let values: Vec<f64> = (0..513 * 513).map(|i| (i % 7) as f64 - 3.0).collect();
        let matrix = DenseMatrix::from_data(513, 513, &values).unwrap();
        coolbox_rs::set_cpu_threads(1);
        let serial = matrix.multiply(&matrix, CpuOptimization::Auto).unwrap().data().unwrap();
        coolbox_rs::set_cpu_threads(3);
        assert_eq!(coolbox_rs::cpu_threads(), 3);
        let parallel = matrix.multiply(&matrix, CpuOptimization::Auto).unwrap().data().unwrap();
        for (a, b) in serial.iter().zip(parallel.iter()) {
            assert!((a - b).abs() < 1e-10);
        }
    }
    MatrixBackend::Cpu.set().unwrap();
    coolbox_rs::set_cpu_threads(0);
}

#[test]
fn dense_matrix_operations() {
    let _guard = MATRIX_BACKEND_LOCK.lock().unwrap();
    MatrixBackend::Eigen.set().unwrap();
    let left = DenseMatrix::from_data(2, 2, &[1.0, 2.0, 3.0, 4.0]).unwrap();
    let right = DenseMatrix::from_data(2, 2, &[5.0, 6.0, 7.0, 8.0]).unwrap();

    assert_eq!(
        left.multiply(&right, CpuOptimization::Scalar)
            .unwrap()
            .data()
            .unwrap(),
        vec![19.0, 22.0, 43.0, 50.0]
    );
    assert_eq!(
        left.transpose().unwrap().data().unwrap(),
        vec![1.0, 3.0, 2.0, 4.0]
    );
    assert_eq!(MatrixBackend::active().unwrap(), MatrixBackend::Eigen);
    MatrixBackend::Cpu.set().unwrap();
}

#[test]
fn matrix_backend_metadata() {
    let _guard = MATRIX_BACKEND_LOCK.lock().unwrap();
    assert!(MatrixBackend::Eigen.is_available());
    assert_eq!(MatrixBackend::Eigen.name(), "eigen");
    assert!(!MatrixBackend::OpenCl.is_available());
    assert!(MatrixBackend::OpenCl.set().is_err());
    assert_eq!(MatrixBackend::OpenBlas.name(), "openblas");
    if !MatrixBackend::OpenBlas.is_available() {
        assert!(MatrixBackend::OpenBlas.set().is_err());
    }
    MatrixBackend::Eigen.set().unwrap();
    assert_eq!(MatrixBackend::current().unwrap(), MatrixBackend::Eigen);
    MatrixBackend::Cpu.set().unwrap();
}

#[test]
fn dense_vector_operations() {
    let _guard = MATRIX_BACKEND_LOCK.lock().unwrap();
    MatrixBackend::Eigen.set().unwrap();
    let left = DenseVector::from_data(&[1.0, 2.0, 3.0]).unwrap();
    let right = DenseVector::from_data(&[4.0, 5.0, 6.0]).unwrap();

    assert_eq!(left.dot(&right, CpuOptimization::Scalar).unwrap(), 32.0);
    assert_eq!(
        left.add(&right, CpuOptimization::Auto)
            .unwrap()
            .data()
            .unwrap(),
        vec![5.0, 7.0, 9.0]
    );
    MatrixBackend::Cpu.set().unwrap();
}
