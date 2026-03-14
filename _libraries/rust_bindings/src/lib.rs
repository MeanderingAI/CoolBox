use cxx::UniquePtr;

mod stubs;

pub use stubs::*;

#[cxx::bridge]
mod ffi {
    #[namespace = "coolbox::rust_bindings"]
    unsafe extern "C++" {
        include!("bridge.h");

        type LinearRegressionModel;

        fn fit_linear_regression(
            x_flat: &[f64],
            rows: usize,
            cols: usize,
            y: &[f64],
            method: &str,
            iterations: u32,
            learning_rate: f64,
        ) -> Result<UniquePtr<LinearRegressionModel>>;

        fn predict_linear_regression(
            model: &LinearRegressionModel,
            x_flat: &[f64],
            rows: usize,
            cols: usize,
        ) -> Result<Vec<f64>>;

        fn weights(self: &LinearRegressionModel) -> Vec<f64>;
        fn intercept(self: &LinearRegressionModel) -> f64;
        fn feature_count(self: &LinearRegressionModel) -> usize;
        fn method_name(self: &LinearRegressionModel) -> String;
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FitMethod {
    ClosedForm,
    GradientDescent,
}

impl FitMethod {
    fn as_str(self) -> &'static str {
        match self {
            Self::ClosedForm => "closed_form",
            Self::GradientDescent => "gradient_descent",
        }
    }
}

pub struct LinearRegression {
    inner: UniquePtr<ffi::LinearRegressionModel>,
}

impl LinearRegression {
    pub fn fit(
        x: &[Vec<f64>],
        y: &[f64],
        method: FitMethod,
        iterations: u32,
        learning_rate: f64,
    ) -> Result<Self, String> {
        let (x_flat, rows, cols) = flatten_matrix(x)?;
        if y.len() != rows {
            return Err("target vector length must match the number of feature rows".to_string());
        }

        let inner = ffi::fit_linear_regression(
            &x_flat,
            rows,
            cols,
            y,
            method.as_str(),
            iterations,
            learning_rate,
        )
        .map_err(|err| err.to_string())?;

        Ok(Self { inner })
    }

    pub fn predict(&self, x: &[Vec<f64>]) -> Result<Vec<f64>, String> {
        let (x_flat, rows, cols) = flatten_matrix(x)?;
        let model = self
            .inner
            .as_ref()
            .ok_or_else(|| "native model pointer is null".to_string())?;

        ffi::predict_linear_regression(model, &x_flat, rows, cols).map_err(|err| err.to_string())
    }

    pub fn weights(&self) -> Vec<f64> {
        self.inner
            .as_ref()
            .map(|model| model.weights())
            .unwrap_or_default()
    }

    pub fn intercept(&self) -> f64 {
        self.inner
            .as_ref()
            .map(|model| model.intercept())
            .unwrap_or_default()
    }

    pub fn feature_count(&self) -> usize {
        self.inner
            .as_ref()
            .map(|model| model.feature_count())
            .unwrap_or_default()
    }

    pub fn method_name(&self) -> String {
        self.inner
            .as_ref()
            .map(|model| model.method_name())
            .unwrap_or_default()
    }
}

fn flatten_matrix(x: &[Vec<f64>]) -> Result<(Vec<f64>, usize, usize), String> {
    if x.is_empty() {
        return Err("feature matrix must contain at least one row".to_string());
    }

    let cols = x[0].len();
    if cols == 0 {
        return Err("feature matrix must contain at least one column".to_string());
    }

    let mut flat = Vec::with_capacity(x.len() * cols);
    for row in x {
        if row.len() != cols {
            return Err("feature matrix rows must all have the same length".to_string());
        }
        flat.extend_from_slice(row);
    }

    Ok((flat, x.len(), cols))
}
