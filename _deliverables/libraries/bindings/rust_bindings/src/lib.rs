use cxx::UniquePtr;
use std::ffi::CStr;

mod stubs;

pub use stubs::*;
pub use stubs::{Toolbar, DockPanel, LayerList, PropertyInspector, FileTree, RadioSelector, CheckboxGroup};

const DEFAULT_ENDPOINT: &str = "local://coolbox";

#[link(name = "coolbox_c_bindings")]
unsafe extern "C" {
    fn coolbox_c_version() -> *const std::ffi::c_char;
    fn coolbox_c_describe() -> *const std::ffi::c_char;
    fn coolbox_c_capability_count() -> usize;
    fn coolbox_c_capability_at(index: usize) -> *const std::ffi::c_char;
    fn coolbox_c_is_ready() -> i32;
}

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

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Client {
    endpoint: String,
}

impl Client {
    pub fn create_default() -> Self {
        Self {
            endpoint: DEFAULT_ENDPOINT.to_string(),
        }
    }

    pub fn for_endpoint(endpoint: impl Into<String>) -> Self {
        let endpoint = endpoint.into();
        Self {
            endpoint: if endpoint.is_empty() {
                DEFAULT_ENDPOINT.to_string()
            } else {
                endpoint
            },
        }
    }

    pub fn endpoint(&self) -> &str {
        &self.endpoint
    }

    pub fn version(&self) -> String {
        let _ = self;
        c_string(unsafe { coolbox_c_version() })
    }

    pub fn describe(&self) -> String {
        let _ = self;
        c_string(unsafe { coolbox_c_describe() })
    }

    pub fn is_ready(&self) -> bool {
        let _ = self;
        unsafe { coolbox_c_is_ready() != 0 }
    }

    pub fn capability_count(&self) -> usize {
        let _ = self;
        unsafe { coolbox_c_capability_count() }
    }

    pub fn capability_at(&self, index: usize) -> String {
        if index >= self.capability_count() {
            return String::new();
        }
        c_string(unsafe { coolbox_c_capability_at(index) })
    }

    pub fn capabilities(&self) -> Vec<String> {
        (0..self.capability_count())
            .map(|index| self.capability_at(index))
            .collect()
    }
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

fn c_string(value: *const std::ffi::c_char) -> String {
    if value.is_null() {
        return String::new();
    }

    unsafe { CStr::from_ptr(value) }
        .to_string_lossy()
        .into_owned()
}
