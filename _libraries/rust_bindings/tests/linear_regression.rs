use coolbox_rs::{FitMethod, LinearRegression};

#[test]
fn closed_form_fit_and_predict() {
    let x = vec![vec![1.0], vec![2.0], vec![3.0], vec![4.0]];
    let y = vec![3.0, 5.0, 7.0, 9.0];

    let model = LinearRegression::fit(&x, &y, FitMethod::ClosedForm, 500, 0.01)
        .expect("fit should succeed");

    assert_eq!(model.feature_count(), 1);
    assert_eq!(model.method_name(), "closed_form");
    assert!((model.intercept() - 1.0).abs() < 1e-6);
    assert!((model.weights()[0] - 2.0).abs() < 1e-6);

    let predictions = model.predict(&x).expect("predict should succeed");
    assert_eq!(predictions.len(), y.len());

    for (prediction, target) in predictions.iter().zip(y.iter()) {
        assert!((prediction - target).abs() < 1e-6);
    }
}

#[test]
fn rejects_ragged_matrices() {
    let x = vec![vec![1.0, 2.0], vec![3.0]];
    let y = vec![1.0, 2.0];

    let err = LinearRegression::fit(&x, &y, FitMethod::ClosedForm, 10, 0.01)
        .err()
        .expect("ragged matrix should fail");

    assert!(err.contains("same length"));
}
