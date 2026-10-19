use std::ffi::{c_char, c_double, c_int, c_uint, c_void, CStr};
use std::ptr::NonNull;

const ERROR_CAPACITY: usize = 1024;

unsafe extern "C" {
    fn coolbox_boost_tree_regressor_new(
        num_estimators: c_uint,
        learning_rate: c_double,
        max_depth: c_uint,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_boost_tree_regressor_free(handle: *mut c_void);
    fn coolbox_boost_tree_regressor_fit(
        handle: *mut c_void,
        features: *const c_double,
        rows: usize,
        columns: usize,
        targets: *const c_double,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_boost_tree_regressor_predict(
        handle: *mut c_void,
        sample: *const c_double,
        columns: usize,
        prediction: *mut c_double,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_boost_tree_regressor_predict_batch(
        handle: *mut c_void,
        features: *const c_double,
        rows: usize,
        columns: usize,
        predictions: *mut c_double,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_boost_tree_classifier_new(
        num_estimators: c_uint,
        learning_rate: c_double,
        max_depth: c_uint,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_boost_tree_classifier_free(handle: *mut c_void);
    fn coolbox_boost_tree_classifier_fit(
        handle: *mut c_void,
        features: *const c_double,
        rows: usize,
        columns: usize,
        labels: *const c_int,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_boost_tree_classifier_predict(
        handle: *mut c_void,
        sample: *const c_double,
        columns: usize,
        prediction: *mut c_int,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_boost_tree_classifier_predict_proba(
        handle: *mut c_void,
        sample: *const c_double,
        columns: usize,
        probabilities: *mut c_double,
        capacity: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_boost_tree_classifier_class_count(
        handle: *mut c_void,
        error: *mut c_char,
        error_capacity: usize,
    ) -> usize;
    fn coolbox_boost_tree_classifier_copy_classes(
        handle: *mut c_void,
        classes: *mut c_int,
        capacity: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
}

struct BoostHandle {
    pointer: NonNull<c_void>,
    free: unsafe extern "C" fn(*mut c_void),
}

impl BoostHandle {
    fn new(
        pointer: *mut c_void,
        free: unsafe extern "C" fn(*mut c_void),
        kind: &str,
        error: &[c_char],
    ) -> Result<Self, String> {
        let Some(pointer) = NonNull::new(pointer) else {
            return Err(native_error(error, &format!("failed to create {kind}")));
        };
        Ok(Self { pointer, free })
    }

    fn as_ptr(&self) -> *mut c_void {
        self.pointer.as_ptr()
    }
}

impl Drop for BoostHandle {
    fn drop(&mut self) {
        unsafe { (self.free)(self.pointer.as_ptr()) };
    }
}

fn native_error(error: &[c_char], fallback: &str) -> String {
    let message = unsafe { CStr::from_ptr(error.as_ptr()) }.to_string_lossy();
    if message.is_empty() {
        fallback.to_string()
    } else {
        message.into_owned()
    }
}

fn check(status: c_int, error: &[c_char], fallback: &str) -> Result<(), String> {
    if status != 0 {
        Ok(())
    } else {
        Err(native_error(error, fallback))
    }
}

fn flatten_matrix(rows: &[Vec<f64>]) -> Result<(Vec<f64>, usize, usize), String> {
    let columns = rows.first().map(Vec::len).unwrap_or(0);
    if rows.is_empty() || columns == 0 || rows.iter().any(|row| row.len() != columns) {
        return Err("feature matrix must be non-empty and rectangular".to_string());
    }
    let mut flat = Vec::with_capacity(rows.len() * columns);
    for row in rows {
        flat.extend_from_slice(row);
    }
    Ok((flat, rows.len(), columns))
}

fn validate_parameters(parameters: BoostTreeParameters) -> Result<(), String> {
    if parameters.num_estimators == 0
        || !parameters.learning_rate.is_finite()
        || parameters.learning_rate <= 0.0
        || parameters.max_depth > i32::MAX as u32
    {
        return Err("boosted-tree parameters require positive estimators and learning rate and a valid depth".to_string());
    }
    Ok(())
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct BoostTreeParameters {
    pub num_estimators: u32,
    pub learning_rate: f64,
    pub max_depth: u32,
}

impl Default for BoostTreeParameters {
    fn default() -> Self {
        Self {
            num_estimators: 100,
            learning_rate: 0.1,
            max_depth: 3,
        }
    }
}

/// Gradient-boosted decision-tree regression model.
pub struct BoostTree {
    handle: BoostHandle,
    pub params: BoostTreeParameters,
}

impl BoostTree {
    pub fn new(params: BoostTreeParameters) -> Result<Self, String> {
        validate_parameters(params)?;
        let mut error = [0; ERROR_CAPACITY];
        let pointer = unsafe {
            coolbox_boost_tree_regressor_new(
                params.num_estimators,
                params.learning_rate,
                params.max_depth,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Ok(Self {
            handle: BoostHandle::new(
                pointer,
                coolbox_boost_tree_regressor_free,
                "boosted-tree regressor",
                &error,
            )?,
            params,
        })
    }

    pub fn fit(&mut self, features: &[Vec<f64>], targets: &[f64]) -> Result<(), String> {
        let (flat, rows, columns) = flatten_matrix(features)?;
        if targets.len() != rows {
            return Err("target count must match feature rows".to_string());
        }
        let mut error = [0; ERROR_CAPACITY];
        let status = unsafe {
            coolbox_boost_tree_regressor_fit(
                self.handle.as_ptr(),
                flat.as_ptr(),
                rows,
                columns,
                targets.as_ptr(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(status, &error, "failed to fit boosted-tree regressor")
    }

    pub fn predict(&self, sample: &[f64]) -> Result<f64, String> {
        if sample.is_empty() {
            return Err("prediction sample must not be empty".to_string());
        }
        let mut prediction = 0.0;
        let mut error = [0; ERROR_CAPACITY];
        let status = unsafe {
            coolbox_boost_tree_regressor_predict(
                self.handle.as_ptr(),
                sample.as_ptr(),
                sample.len(),
                &mut prediction,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(status, &error, "failed to predict with boosted-tree regressor")?;
        Ok(prediction)
    }

    pub fn predict_batch(&self, features: &[Vec<f64>]) -> Result<Vec<f64>, String> {
        let (flat, rows, columns) = flatten_matrix(features)?;
        let mut predictions = vec![0.0; rows];
        let mut error = [0; ERROR_CAPACITY];
        let status = unsafe {
            coolbox_boost_tree_regressor_predict_batch(
                self.handle.as_ptr(),
                flat.as_ptr(),
                rows,
                columns,
                predictions.as_mut_ptr(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(status, &error, "failed to predict with boosted-tree regressor")?;
        Ok(predictions)
    }
}

/// Gradient-boosted decision-tree classifier with arbitrary i32 labels.
pub struct BoostTreeClassifier {
    handle: BoostHandle,
    pub params: BoostTreeParameters,
}

impl BoostTreeClassifier {
    pub fn new(params: BoostTreeParameters) -> Result<Self, String> {
        validate_parameters(params)?;
        let mut error = [0; ERROR_CAPACITY];
        let pointer = unsafe {
            coolbox_boost_tree_classifier_new(
                params.num_estimators,
                params.learning_rate,
                params.max_depth,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Ok(Self {
            handle: BoostHandle::new(
                pointer,
                coolbox_boost_tree_classifier_free,
                "boosted-tree classifier",
                &error,
            )?,
            params,
        })
    }

    pub fn fit(&mut self, features: &[Vec<f64>], labels: &[i32]) -> Result<(), String> {
        let (flat, rows, columns) = flatten_matrix(features)?;
        if labels.len() != rows {
            return Err("label count must match feature rows".to_string());
        }
        let mut error = [0; ERROR_CAPACITY];
        let status = unsafe {
            coolbox_boost_tree_classifier_fit(
                self.handle.as_ptr(),
                flat.as_ptr(),
                rows,
                columns,
                labels.as_ptr(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(status, &error, "failed to fit boosted-tree classifier")
    }

    pub fn predict(&self, sample: &[f64]) -> Result<i32, String> {
        if sample.is_empty() {
            return Err("prediction sample must not be empty".to_string());
        }
        let mut prediction = 0;
        let mut error = [0; ERROR_CAPACITY];
        let status = unsafe {
            coolbox_boost_tree_classifier_predict(
                self.handle.as_ptr(),
                sample.as_ptr(),
                sample.len(),
                &mut prediction,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(status, &error, "failed to predict with boosted-tree classifier")?;
        Ok(prediction)
    }

    pub fn predict_proba(&self, sample: &[f64]) -> Result<Vec<f64>, String> {
        if sample.is_empty() {
            return Err("prediction sample must not be empty".to_string());
        }
        let mut error = [0; ERROR_CAPACITY];
        let count = unsafe {
            coolbox_boost_tree_classifier_class_count(
                self.handle.as_ptr(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        if count == 0 {
            return Err(native_error(&error, "failed to get boosted-tree classes"));
        }
        let mut probabilities = vec![0.0; count];
        let status = unsafe {
            coolbox_boost_tree_classifier_predict_proba(
                self.handle.as_ptr(),
                sample.as_ptr(),
                sample.len(),
                probabilities.as_mut_ptr(),
                probabilities.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(status, &error, "failed to predict class probabilities")?;
        Ok(probabilities)
    }

    pub fn classes(&self) -> Result<Vec<i32>, String> {
        let mut error = [0; ERROR_CAPACITY];
        let count = unsafe {
            coolbox_boost_tree_classifier_class_count(
                self.handle.as_ptr(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        if count == 0 {
            return Err(native_error(&error, "failed to get boosted-tree classes"));
        }
        let mut classes = vec![0; count];
        let status = unsafe {
            coolbox_boost_tree_classifier_copy_classes(
                self.handle.as_ptr(),
                classes.as_mut_ptr(),
                classes.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(status, &error, "failed to copy boosted-tree classes")?;
        Ok(classes)
    }
}

#[cfg(test)]
mod tests {
    use super::{BoostTree, BoostTreeClassifier, BoostTreeParameters};

    #[test]
    fn boosted_tree_regression_fit_and_batch_prediction() {
        let mut model = BoostTree::new(BoostTreeParameters {
            num_estimators: 10,
            learning_rate: 0.1,
            max_depth: 0,
        })
        .unwrap();
        let features = vec![vec![0.0], vec![1.0], vec![2.0], vec![3.0]];
        model.fit(&features, &[1.0, 2.0, 3.0, 4.0]).unwrap();
        let predictions = model.predict_batch(&features).unwrap();
        assert_eq!(predictions.len(), 4);
        assert!(predictions.iter().all(|prediction| (*prediction - 2.5).abs() < 1e-9));
        assert!((model.predict(&[1.5]).unwrap() - 2.5).abs() < 1e-9);
    }

    #[test]
    fn boosted_tree_classifier_predicts_arbitrary_labels_and_probabilities() {
        let mut model = BoostTreeClassifier::new(BoostTreeParameters {
            num_estimators: 20,
            learning_rate: 0.2,
            max_depth: 1,
        })
        .unwrap();
        model
            .fit(
                &[vec![0.0], vec![0.1], vec![1.0], vec![1.1]],
                &[-7, -7, 42, 42],
            )
            .unwrap();
        assert_eq!(model.classes().unwrap(), vec![-7, 42]);
        assert_eq!(model.predict(&[0.0]).unwrap(), -7);
        assert_eq!(model.predict(&[1.1]).unwrap(), 42);
        let probabilities = model.predict_proba(&[1.1]).unwrap();
        assert_eq!(probabilities.len(), 2);
        assert!(probabilities.iter().all(|value| (0.0..=1.0).contains(value)));
        assert!((probabilities.iter().sum::<f64>() - 1.0).abs() < 1e-12);
    }

    #[test]
    fn boosted_tree_rejects_bad_configuration() {
        let result = BoostTree::new(BoostTreeParameters {
            num_estimators: 0,
            ..BoostTreeParameters::default()
        });
        assert!(result.is_err());
    }
}
