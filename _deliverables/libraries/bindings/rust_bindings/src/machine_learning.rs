use std::ffi::{c_char, c_double, c_int, c_void, CStr, CString};
use std::marker::PhantomData;
use std::ptr::NonNull;

const ERROR_CAPACITY: usize = 1024;

unsafe extern "C" {
    fn coolbox_tree_classifier_new(criterion: c_int) -> *mut c_void;
    fn coolbox_tree_classifier_free(handle: *mut c_void);
    fn coolbox_tree_classifier_fit(
        handle: *mut c_void,
        features: *const c_int,
        rows: usize,
        columns: usize,
        labels: *const c_int,
        max_depth: c_int,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_tree_classifier_predict(
        handle: *mut c_void,
        sample: *const c_int,
        columns: usize,
        prediction: *mut c_int,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_tree_regressor_new() -> *mut c_void;
    fn coolbox_tree_regressor_free(handle: *mut c_void);
    fn coolbox_tree_regressor_fit(
        handle: *mut c_void,
        features: *const c_double,
        rows: usize,
        columns: usize,
        targets: *const c_double,
        max_depth: c_int,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_tree_regressor_predict(
        handle: *mut c_void,
        sample: *const c_double,
        columns: usize,
        prediction: *mut c_double,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;

    fn coolbox_nlp_lowercase(text: *const c_char) -> *mut c_char;
    fn coolbox_nlp_remove_punctuation(text: *const c_char) -> *mut c_char;
    fn coolbox_nlp_remove_numbers(text: *const c_char) -> *mut c_char;
    fn coolbox_nlp_normalize_whitespace(text: *const c_char) -> *mut c_char;
    fn coolbox_nlp_free_string(value: *mut c_char);
    fn coolbox_nlp_tokenize(text: *const c_char, mode: c_int) -> *mut c_void;
    fn coolbox_nlp_token_count(handle: *const c_void) -> usize;
    fn coolbox_nlp_token_at(handle: *const c_void, index: usize) -> *mut c_char;
    fn coolbox_nlp_tokens_free(handle: *mut c_void);

    fn coolbox_series_new(values: *const c_double, count: usize) -> *mut c_void;
    fn coolbox_series_free(handle: *mut c_void);
    fn coolbox_series_size(handle: *const c_void) -> usize;
    fn coolbox_series_copy_values(
        handle: *const c_void,
        destination: *mut c_double,
        capacity: usize,
    ) -> c_int;
    fn coolbox_series_statistic(
        handle: *const c_void,
        statistic: c_int,
        result: *mut c_double,
    ) -> c_int;
    fn coolbox_series_autocorrelation_size(handle: *const c_void, max_lag: usize) -> usize;
    fn coolbox_series_transform(
        handle: *mut c_void,
        operation: c_int,
        integer_argument: usize,
        first_argument: c_double,
        second_argument: c_double,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_series_copy_autocorrelation(
        handle: *const c_void,
        max_lag: usize,
        destination: *mut c_double,
        capacity: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;

    fn coolbox_forecaster_new(
        kind: c_int,
        first_order: usize,
        second_order: usize,
        third_order: usize,
        alpha: c_double,
        beta: c_double,
        gamma: c_double,
    ) -> *mut c_void;
    fn coolbox_forecaster_free(handle: *mut c_void);
    fn coolbox_forecaster_fit(
        handle: *mut c_void,
        values: *const c_double,
        count: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_forecaster_forecast(
        handle: *mut c_void,
        steps: usize,
        destination: *mut c_double,
        capacity: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
}

struct Handle {
    ptr: NonNull<c_void>,
    free: unsafe extern "C" fn(*mut c_void),
    _not_sync: PhantomData<*mut ()>,
}

impl Handle {
    fn new(
        ptr: *mut c_void,
        free: unsafe extern "C" fn(*mut c_void),
        kind: &str,
    ) -> Result<Self, String> {
        Ok(Self {
            ptr: NonNull::new(ptr).ok_or_else(|| format!("failed to create {kind}"))?,
            free,
            _not_sync: PhantomData,
        })
    }

    fn as_ptr(&self) -> *mut c_void {
        self.ptr.as_ptr()
    }
}

impl Drop for Handle {
    fn drop(&mut self) {
        unsafe { (self.free)(self.ptr.as_ptr()) };
    }
}

fn error_buffer() -> [c_char; ERROR_CAPACITY] {
    [0; ERROR_CAPACITY]
}

fn native_error(buffer: &[c_char]) -> String {
    let message = unsafe { CStr::from_ptr(buffer.as_ptr()) }.to_string_lossy();
    if message.is_empty() {
        "native operation failed".to_string()
    } else {
        message.into_owned()
    }
}

fn check(ok: c_int, buffer: &[c_char], fallback: &str) -> Result<(), String> {
    if ok != 0 {
        Ok(())
    } else {
        let message = native_error(buffer);
        if message == "native operation failed" {
            Err(fallback.to_string())
        } else {
            Err(message)
        }
    }
}

fn matrix<T: Copy>(rows: &[Vec<T>]) -> Result<(Vec<T>, usize, usize), String> {
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

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TreeCriterion {
    Gini,
    Entropy,
}

pub struct DecisionTreeClassifier {
    handle: Handle,
    feature_count: usize,
}

impl DecisionTreeClassifier {
    pub fn new(criterion: TreeCriterion) -> Result<Self, String> {
        let criterion = match criterion {
            TreeCriterion::Gini => 0,
            TreeCriterion::Entropy => 1,
        };
        Ok(Self {
            handle: Handle::new(
                unsafe { coolbox_tree_classifier_new(criterion) },
                coolbox_tree_classifier_free,
                "decision-tree classifier",
            )?,
            feature_count: 0,
        })
    }

    pub fn fit(
        &mut self,
        features: &[Vec<i32>],
        labels: &[i32],
        max_depth: i32,
    ) -> Result<(), String> {
        let (flat, rows, columns) = matrix(features)?;
        if labels.len() != rows {
            return Err("label count must match feature rows".to_string());
        }
        if max_depth < 0 {
            return Err("max_depth must be nonnegative".to_string());
        }
        let mut error = error_buffer();
        let ok = unsafe {
            coolbox_tree_classifier_fit(
                self.handle.as_ptr(),
                flat.as_ptr(),
                rows,
                columns,
                labels.as_ptr(),
                max_depth,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to fit decision-tree classifier")?;
        self.feature_count = columns;
        Ok(())
    }

    pub fn predict(&self, sample: &[i32]) -> Result<i32, String> {
        if sample.len() != self.feature_count {
            return Err("sample feature count does not match fitted model".to_string());
        }
        let mut prediction = 0;
        let mut error = error_buffer();
        let ok = unsafe {
            coolbox_tree_classifier_predict(
                self.handle.as_ptr(),
                sample.as_ptr(),
                sample.len(),
                &mut prediction,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(
            ok,
            &error,
            "failed to predict with decision-tree classifier",
        )?;
        Ok(prediction)
    }
}

pub struct DecisionTreeRegressor {
    handle: Handle,
    feature_count: usize,
}

impl DecisionTreeRegressor {
    pub fn new() -> Result<Self, String> {
        Ok(Self {
            handle: Handle::new(
                unsafe { coolbox_tree_regressor_new() },
                coolbox_tree_regressor_free,
                "decision-tree regressor",
            )?,
            feature_count: 0,
        })
    }

    pub fn fit(
        &mut self,
        features: &[Vec<f64>],
        targets: &[f64],
        max_depth: i32,
    ) -> Result<(), String> {
        let (flat, rows, columns) = matrix(features)?;
        if targets.len() != rows {
            return Err("target count must match feature rows".to_string());
        }
        let mut error = error_buffer();
        let ok = unsafe {
            coolbox_tree_regressor_fit(
                self.handle.as_ptr(),
                flat.as_ptr(),
                rows,
                columns,
                targets.as_ptr(),
                max_depth,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to fit decision-tree regressor")?;
        self.feature_count = columns;
        Ok(())
    }

    pub fn predict(&self, sample: &[f64]) -> Result<f64, String> {
        if sample.len() != self.feature_count {
            return Err("sample feature count does not match fitted model".to_string());
        }
        let mut prediction = 0.0;
        let mut error = error_buffer();
        let ok = unsafe {
            coolbox_tree_regressor_predict(
                self.handle.as_ptr(),
                sample.as_ptr(),
                sample.len(),
                &mut prediction,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to predict with decision-tree regressor")?;
        Ok(prediction)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TokenizeMode {
    Tokens,
    Words,
    Sentences,
    Processed,
}

pub struct TextProcessor;

impl TextProcessor {
    fn clean(
        text: &str,
        operation: unsafe extern "C" fn(*const c_char) -> *mut c_char,
    ) -> Result<String, String> {
        let input = CString::new(text).map_err(|_| "text cannot contain a NUL byte".to_string())?;
        let output = unsafe { operation(input.as_ptr()) };
        if output.is_null() {
            return Err("native text processing failed".to_string());
        }
        let value = unsafe { CStr::from_ptr(output) }
            .to_string_lossy()
            .into_owned();
        unsafe { coolbox_nlp_free_string(output) };
        Ok(value)
    }

    pub fn lowercase(text: &str) -> Result<String, String> {
        Self::clean(text, coolbox_nlp_lowercase)
    }
    pub fn remove_punctuation(text: &str) -> Result<String, String> {
        Self::clean(text, coolbox_nlp_remove_punctuation)
    }
    pub fn remove_numbers(text: &str) -> Result<String, String> {
        Self::clean(text, coolbox_nlp_remove_numbers)
    }
    pub fn normalize_whitespace(text: &str) -> Result<String, String> {
        Self::clean(text, coolbox_nlp_normalize_whitespace)
    }

    pub fn tokenize(text: &str, mode: TokenizeMode) -> Result<Vec<String>, String> {
        let input = CString::new(text).map_err(|_| "text cannot contain a NUL byte".to_string())?;
        let mode = match mode {
            TokenizeMode::Tokens => 0,
            TokenizeMode::Words => 1,
            TokenizeMode::Sentences => 2,
            TokenizeMode::Processed => 3,
        };
        let tokens = NonNull::new(unsafe { coolbox_nlp_tokenize(input.as_ptr(), mode) })
            .ok_or_else(|| "native tokenization failed".to_string())?;
        let count = unsafe { coolbox_nlp_token_count(tokens.as_ptr()) };
        let result = (0..count)
            .map(|index| {
                let token = unsafe { coolbox_nlp_token_at(tokens.as_ptr(), index) };
                if token.is_null() {
                    return Err(format!("failed to retrieve token at index {index}"));
                }
                let value = unsafe { CStr::from_ptr(token) }
                    .to_string_lossy()
                    .into_owned();
                unsafe { coolbox_nlp_free_string(token) };
                Ok(value)
            })
            .collect();
        unsafe { coolbox_nlp_tokens_free(tokens.as_ptr()) };
        result
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SeriesStatistic {
    Mean,
    StandardDeviation,
    Minimum,
    Maximum,
    Median,
}

#[derive(Debug, Clone, Copy)]
pub enum SeriesTransform {
    Normalize,
    MinMaxScale { minimum: f64, maximum: f64 },
    Difference { order: usize },
    Log,
    MovingAverage { window: usize },
    ExponentialSmoothing { alpha: f64 },
    Resample { factor: usize },
}

pub struct TimeSeries {
    handle: Handle,
}

impl TimeSeries {
    pub fn new(values: &[f64]) -> Result<Self, String> {
        Ok(Self {
            handle: Handle::new(
                unsafe { coolbox_series_new(values.as_ptr(), values.len()) },
                coolbox_series_free,
                "time series",
            )?,
        })
    }
    pub fn len(&self) -> usize {
        unsafe { coolbox_series_size(self.handle.as_ptr()) }
    }
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
    pub fn values(&self) -> Result<Vec<f64>, String> {
        let mut result = vec![0.0; self.len()];
        if unsafe {
            coolbox_series_copy_values(self.handle.as_ptr(), result.as_mut_ptr(), result.len())
        } == 0
        {
            return Err("failed to read time-series values".to_string());
        }
        Ok(result)
    }
    pub fn statistic(&self, statistic: SeriesStatistic) -> Result<f64, String> {
        let kind = match statistic {
            SeriesStatistic::Mean => 0,
            SeriesStatistic::StandardDeviation => 1,
            SeriesStatistic::Minimum => 2,
            SeriesStatistic::Maximum => 3,
            SeriesStatistic::Median => 4,
        };
        let mut result = 0.0;
        if unsafe { coolbox_series_statistic(self.handle.as_ptr(), kind, &mut result) } == 0 {
            return Err("cannot calculate statistic for this time series".to_string());
        }
        Ok(result)
    }
    pub fn autocorrelation(&self, max_lag: usize) -> Result<Vec<f64>, String> {
        let count = unsafe { coolbox_series_autocorrelation_size(self.handle.as_ptr(), max_lag) };
        let mut result = vec![0.0; count];
        let mut error = error_buffer();
        let ok = unsafe {
            coolbox_series_copy_autocorrelation(
                self.handle.as_ptr(),
                max_lag,
                result.as_mut_ptr(),
                result.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to calculate autocorrelation")?;
        Ok(result)
    }
    pub fn transform(&self, transform: SeriesTransform) -> Result<Self, String> {
        let (operation, integer, first, second) = match transform {
            SeriesTransform::Normalize => (0, 0, 0.0, 0.0),
            SeriesTransform::MinMaxScale { minimum, maximum } => (1, 0, minimum, maximum),
            SeriesTransform::Difference { order } => (2, order, 0.0, 0.0),
            SeriesTransform::Log => (3, 0, 0.0, 0.0),
            SeriesTransform::MovingAverage { window } => (4, window, 0.0, 0.0),
            SeriesTransform::ExponentialSmoothing { alpha } => (5, 0, alpha, 0.0),
            SeriesTransform::Resample { factor } => (6, factor, 0.0, 0.0),
        };
        let mut error = error_buffer();
        let ptr = unsafe {
            coolbox_series_transform(
                self.handle.as_ptr(),
                operation,
                integer,
                first,
                second,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Ok(Self {
            handle: Handle::new(ptr, coolbox_series_free, "transformed time series")
                .map_err(|_| native_error(&error))?,
        })
    }
}

#[derive(Debug, Clone, Copy)]
pub enum Forecaster {
    MovingAverage {
        window: usize,
    },
    ExponentialSmoothing {
        alpha: f64,
        beta: f64,
        gamma: f64,
    },
    AutoRegressive {
        order: usize,
    },
    Arma {
        autoregressive_order: usize,
        moving_average_order: usize,
    },
    Arima {
        autoregressive_order: usize,
        differencing_order: usize,
        moving_average_order: usize,
    },
}

pub struct ForecastModel {
    handle: Handle,
}

impl ForecastModel {
    pub fn new(model: Forecaster) -> Result<Self, String> {
        let (kind, p, d, q, alpha, beta, gamma) = match model {
            Forecaster::MovingAverage { window } => (0, window, 0, 0, 0.0, 0.0, 0.0),
            Forecaster::ExponentialSmoothing { alpha, beta, gamma } => {
                (1, 0, 0, 0, alpha, beta, gamma)
            }
            Forecaster::AutoRegressive { order } => (2, order, 0, 0, 0.0, 0.0, 0.0),
            Forecaster::Arma {
                autoregressive_order,
                moving_average_order,
            } => (
                3,
                autoregressive_order,
                moving_average_order,
                0,
                0.0,
                0.0,
                0.0,
            ),
            Forecaster::Arima {
                autoregressive_order,
                differencing_order,
                moving_average_order,
            } => (
                4,
                autoregressive_order,
                differencing_order,
                moving_average_order,
                0.0,
                0.0,
                0.0,
            ),
        };
        Ok(Self {
            handle: Handle::new(
                unsafe { coolbox_forecaster_new(kind, p, d, q, alpha, beta, gamma) },
                coolbox_forecaster_free,
                "forecast model",
            )?,
        })
    }
    pub fn fit(&mut self, values: &[f64]) -> Result<(), String> {
        let mut error = error_buffer();
        let ok = unsafe {
            coolbox_forecaster_fit(
                self.handle.as_ptr(),
                values.as_ptr(),
                values.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to fit forecast model")
    }
    pub fn forecast(&self, steps: usize) -> Result<Vec<f64>, String> {
        let mut result = vec![0.0; steps];
        let mut error = error_buffer();
        let ok = unsafe {
            coolbox_forecaster_forecast(
                self.handle.as_ptr(),
                steps,
                result.as_mut_ptr(),
                result.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to forecast")?;
        Ok(result)
    }
}
