use coolbox_rs::{CpuOptimization, DenseMatrix, DenseVector, MatrixBackend};
use std::sync::Mutex;

static MATRIX_BACKEND_LOCK: Mutex<()> = Mutex::new(());

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
