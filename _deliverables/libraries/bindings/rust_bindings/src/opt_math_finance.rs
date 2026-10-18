use std::ffi::{c_char, c_int, c_void, CStr};
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::ptr;
use std::slice;

use crate::deep_learning::Tensor;

const ERROR_CAPACITY: usize = 1024;

#[repr(C)]
#[derive(Default, Debug, Clone, Copy, PartialEq)]
pub struct MonteCarloEstimate {
    pub price: f64,
    pub standard_error: f64,
    pub confidence_interval_low: f64,
    pub confidence_interval_high: f64,
    pub sample_count: usize,
}

#[repr(C)]
#[derive(Default, Debug, Clone, Copy, PartialEq)]
pub struct MonteCarloGreeks {
    pub delta: MonteCarloEstimate,
    pub gamma: MonteCarloEstimate,
    pub vega: MonteCarloEstimate,
    pub theta: MonteCarloEstimate,
    pub rho: MonteCarloEstimate,
}

type ObjectiveFn = extern "C" fn(usize, *const f64, usize) -> f64;
type PayoffFn = extern "C" fn(usize, *const f64, usize, *mut f64) -> c_int;
type NowaLossFn = extern "C" fn(usize, *const f64, *const usize, usize, *mut f64) -> c_int;

unsafe extern "C" {
    fn coolbox_optimizer_create(
        kind: c_int,
        config: *const f64,
        config_count: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_optimizer_free(optimizer: *mut c_void);
    fn coolbox_optimizer_run(
        optimizer: *mut c_void,
        objective: ObjectiveFn,
        user_data: usize,
        initial: *const f64,
        count: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_optimizer_solution_size(optimizer: *mut c_void) -> usize;
    fn coolbox_optimizer_copy_solution(
        optimizer: *mut c_void,
        output: *mut f64,
        count: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_optimizer_best_score(optimizer: *mut c_void) -> f64;
    fn coolbox_optimizer_best_probability(optimizer: *mut c_void) -> f64;
    fn coolbox_optimizer_quantum_bounds(
        optimizer: *mut c_void,
        output: *mut f64,
        count: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_optimizer_warp_point(
        optimizer: *mut c_void,
        input: *const f64,
        count: usize,
        output: *mut f64,
        unwarp: c_int,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_math_heat(
        dimensions: c_int,
        stochastic: c_int,
        input: *const f64,
        nx: usize,
        ny: usize,
        nz: usize,
        alpha: f64,
        sigma: f64,
        dx: f64,
        dy: f64,
        dz: f64,
        dt: f64,
        steps: c_int,
        seed: u32,
        output: *mut f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_finance_formula(
        operation: c_int,
        values: *const f64,
        count: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> f64;
    fn coolbox_finance_path(
        geometric: c_int,
        initial_value: f64,
        drift: f64,
        volatility: f64,
        time_steps: *const f64,
        step_count: usize,
        normal_shocks: *const f64,
        output: *mut f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_finance_ito(
        variance: c_int,
        values: *const f64,
        increments: *const f64,
        count: usize,
        output: *mut f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_finance_monte_carlo(
        operation: c_int,
        spot: f64,
        strike: f64,
        barrier: f64,
        risk_free_rate: f64,
        volatility: f64,
        time_steps: *const f64,
        step_count: usize,
        shock_paths: *const f64,
        path_count: usize,
        confidence_z_score: f64,
        relative_bump: f64,
        time_bump_years: f64,
        estimate: *mut MonteCarloEstimate,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_finance_monte_carlo_greeks(
        operation: c_int,
        spot: f64,
        strike: f64,
        barrier: f64,
        risk_free_rate: f64,
        volatility: f64,
        time_steps: *const f64,
        step_count: usize,
        shock_paths: *const f64,
        path_count: usize,
        confidence_z_score: f64,
        relative_bump: f64,
        time_bump_years: f64,
        greeks: *mut MonteCarloGreeks,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_finance_monte_carlo_generic(
        spot: f64,
        risk_free_rate: f64,
        volatility: f64,
        time_steps: *const f64,
        step_count: usize,
        shock_paths: *const f64,
        path_count: usize,
        confidence_z_score: f64,
        payoff: PayoffFn,
        user_data: usize,
        estimate: *mut MonteCarloEstimate,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_finance_monte_carlo_greeks_generic(
        spot: f64,
        strike: f64,
        risk_free_rate: f64,
        volatility: f64,
        time_steps: *const f64,
        step_count: usize,
        shock_paths: *const f64,
        path_count: usize,
        confidence_z_score: f64,
        relative_bump: f64,
        time_bump_years: f64,
        payoff: PayoffFn,
        user_data: usize,
        greeks: *mut MonteCarloGreeks,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_owa_naive_average(
        parameters: *const f64,
        models: usize,
        parameter_count: usize,
        output: *mut f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_owa_linear_regression(
        parameters: *const f64,
        models: usize,
        parameter_count: usize,
        features: *const f64,
        rows: usize,
        columns: usize,
        targets: *const f64,
        target_count: usize,
        iterations: c_int,
        learning_rate: f64,
        regularization: f64,
        project_to_simplex: c_int,
        merged: *mut f64,
        weights: *mut f64,
        validation_loss: *mut f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_owa_cross_validation_linear_regression(
        local_parameters: *const f64,
        model_count: usize,
        parameter_count: usize,
        fold_features: *const f64,
        fold_row_offsets: *const usize,
        fold_targets: *const f64,
        fold_target_offsets: *const usize,
        fold_count: usize,
        feature_count: usize,
        iterations: c_int,
        learning_rate: f64,
        regularization: f64,
        project_to_simplex: c_int,
        fold_losses: *mut f64,
        mean_loss: *mut f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_owa_nonlinear_layers(
        local_layers: *const f64,
        model_count: usize,
        layer_count: usize,
        layer_shapes: *const usize,
        layer_shape_offsets: *const usize,
        layer_sizes: *const usize,
        layer_value_offsets: *const usize,
        layer_dimensions: *const usize,
        iterations: c_int,
        learning_rate: f64,
        regularization: f64,
        project_to_simplex: c_int,
        loss_fn: NowaLossFn,
        user_data: usize,
        merged_layers: *mut f64,
        layer_weights: *mut f64,
        validation_loss: *mut f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_owa_weighted_tensor_average(
        local_tensors: *const f64,
        model_count: usize,
        shape: *const usize,
        shape_dimensions: usize,
        weights: *const f64,
        tensor_size: usize,
        merged_tensor: *mut f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
}

fn native_error(buffer: &[c_char], fallback: &str) -> String {
    let message = unsafe { CStr::from_ptr(buffer.as_ptr()) }.to_string_lossy();
    if message.is_empty() {
        fallback.to_string()
    } else {
        message.into_owned()
    }
}

fn check(ok: c_int, buffer: &[c_char], fallback: &str) -> Result<(), String> {
    if ok != 0 {
        Ok(())
    } else {
        Err(native_error(buffer, fallback))
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum OptimizerType {
    GeneticSearch = 0,
    SimulatedAnnealing = 1,
    HillClimbing = 2,
    QuantumControlBayesian = 3,
}

#[derive(Debug, Clone, Copy)]
pub struct CallbackFailure;

struct CallbackData<'a> {
    objective: &'a mut dyn FnMut(&[f64]) -> f64,
    failed: bool,
}

extern "C" fn objective_callback(user_data: usize, values: *const f64, count: usize) -> f64 {
    let context = unsafe { &mut *(user_data as *mut CallbackData<'_>) };
    let points = unsafe { slice::from_raw_parts(values, count) };
    match catch_unwind(AssertUnwindSafe(|| (context.objective)(points))) {
        Ok(score) => score,
        Err(_) => {
            context.failed = true;
            f64::NAN
        }
    }
}

pub struct SearchOptimizer {
    handle: *mut c_void,
    kind: OptimizerType,
}

impl SearchOptimizer {
    pub fn new(kind: OptimizerType, config: &[f64]) -> Result<Self, String> {
        let mut error = [0; ERROR_CAPACITY];
        let config_ptr = if config.is_empty() {
            ptr::null()
        } else {
            config.as_ptr()
        };
        let handle = unsafe {
            coolbox_optimizer_create(
                kind as c_int,
                config_ptr,
                config.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        if handle.is_null() {
            return Err(native_error(&error, "failed to create optimizer"));
        }
        Ok(Self { handle, kind })
    }

    pub fn optimize<F>(&mut self, objective: &mut F, initial: &[f64]) -> Result<Vec<f64>, String>
    where
        F: FnMut(&[f64]) -> f64,
    {
        if self.handle.is_null() {
            return Err("optimizer is closed".to_string());
        }
        let mut context = CallbackData {
            objective,
            failed: false,
        };
        let mut error = [0; ERROR_CAPACITY];
        let user_data = (&mut context as *mut CallbackData<'_>) as usize;
        let initial_ptr = if initial.is_empty() {
            ptr::null()
        } else {
            initial.as_ptr()
        };
        let ok = unsafe {
            coolbox_optimizer_run(
                self.handle,
                objective_callback,
                user_data,
                initial_ptr,
                initial.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "optimization failed")?;
        if context.failed {
            return Err("objective callback panicked".to_string());
        }
        let count = unsafe { coolbox_optimizer_solution_size(self.handle) };
        let mut result = vec![0.0; count];
        let output = if result.is_empty() {
            ptr::null_mut()
        } else {
            result.as_mut_ptr()
        };
        let ok = unsafe {
            coolbox_optimizer_copy_solution(
                self.handle,
                output,
                count,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to read optimizer solution")?;
        Ok(result)
    }

    pub fn best_score(&self) -> Result<f64, String> {
        if self.handle.is_null() {
            return Err("optimizer is closed".to_string());
        }
        Ok(unsafe { coolbox_optimizer_best_score(self.handle) })
    }

    pub fn best_probability(&self) -> Result<f64, String> {
        if self.kind != OptimizerType::QuantumControlBayesian {
            return Err(
                "best probability is available only for quantum-control optimization".to_string(),
            );
        }
        if self.handle.is_null() {
            return Err("optimizer is closed".to_string());
        }
        Ok(unsafe { coolbox_optimizer_best_probability(self.handle) })
    }

    pub fn quantum_bounds(&self) -> Result<[f64; 5], String> {
        let mut output = [0.0; 5];
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_optimizer_quantum_bounds(
                self.handle,
                output.as_mut_ptr(),
                output.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to get quantum-control bounds")?;
        Ok(output)
    }

    pub fn transform_frequency_point(
        &self,
        point: &[f64],
        unwarp: bool,
    ) -> Result<Vec<f64>, String> {
        if point.is_empty() {
            return Err("point must not be empty".to_string());
        }
        let mut output = vec![0.0; point.len()];
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_optimizer_warp_point(
                self.handle,
                point.as_ptr(),
                point.len(),
                output.as_mut_ptr(),
                if unwarp { 1 } else { 0 },
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to transform quantum-control point")?;
        Ok(output)
    }
}

impl Drop for SearchOptimizer {
    fn drop(&mut self) {
        if !self.handle.is_null() {
            unsafe { coolbox_optimizer_free(self.handle) };
            self.handle = ptr::null_mut();
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct HeatGrid {
    pub values: Vec<f64>,
    pub nx: usize,
    pub ny: usize,
    pub nz: usize,
}

pub fn solve_heat_equation(
    initial: &HeatGrid,
    dimensions: i32,
    alpha: f64,
    sigma: f64,
    spacing: [f64; 3],
    dt: f64,
    steps: i32,
    seed: u32,
    stochastic: bool,
) -> Result<HeatGrid, String> {
    let expected = initial
        .nx
        .checked_mul(initial.ny)
        .and_then(|size| size.checked_mul(initial.nz))
        .ok_or_else(|| "heat grid dimensions overflow".to_string())?;
    if expected == 0 || initial.values.len() != expected {
        return Err("heat grid shape does not match the supplied values".to_string());
    }
    let mut result = HeatGrid {
        values: vec![0.0; expected],
        nx: initial.nx,
        ny: initial.ny,
        nz: initial.nz,
    };
    let mut error = [0; ERROR_CAPACITY];
    let ok = unsafe {
        coolbox_math_heat(
            dimensions,
            if stochastic { 1 } else { 0 },
            initial.values.as_ptr(),
            initial.nx,
            initial.ny,
            initial.nz,
            alpha,
            sigma,
            spacing[0],
            spacing[1],
            spacing[2],
            dt,
            steps,
            seed,
            result.values.as_mut_ptr(),
            error.as_mut_ptr(),
            error.len(),
        )
    };
    check(ok, &error, "heat solver failed")?;
    Ok(result)
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum FinanceFormula {
    StandardNormalPdf = 0,
    StandardNormalCdf = 1,
    BlackScholesD1 = 2,
    BlackScholesD2 = 3,
    BlackScholesCallPrice = 4,
    BlackScholesPutPrice = 5,
    BlackScholesCallDelta = 6,
    BlackScholesPutDelta = 7,
    BlackScholesGamma = 8,
    BlackScholesVega = 9,
    BlackScholesCallTheta = 10,
    BlackScholesPutTheta = 11,
    BlackScholesCallRho = 12,
    BlackScholesPutRho = 13,
    DigitalCallPrice = 14,
    DigitalPutPrice = 15,
    GeometricAsianCallPrice = 16,
    GeometricAsianPutPrice = 17,
    DownAndInCallPrice = 18,
    DownAndOutCallPrice = 19,
    UpAndInCallPrice = 20,
    UpAndOutCallPrice = 21,
    DownAndInPutPrice = 22,
    DownAndOutPutPrice = 23,
}

pub fn evaluate_finance_formula(formula: FinanceFormula, values: &[f64]) -> Result<f64, String> {
    let mut error = [0; ERROR_CAPACITY];
    let ptr = if values.is_empty() {
        ptr::null()
    } else {
        values.as_ptr()
    };
    let result = unsafe {
        coolbox_finance_formula(
            formula as c_int,
            ptr,
            values.len(),
            error.as_mut_ptr(),
            error.len(),
        )
    };
    if error[0] != 0 {
        Err(native_error(&error, "finance formula failed"))
    } else {
        Ok(result)
    }
}

pub fn brownian_motion_path(
    times: &[f64],
    shocks: &[f64],
    initial: f64,
) -> Result<Vec<f64>, String> {
    finance_path(false, initial, 0.0, 0.0, times, shocks)
}

pub fn geometric_brownian_motion_path(
    initial: f64,
    drift: f64,
    volatility: f64,
    times: &[f64],
    shocks: &[f64],
) -> Result<Vec<f64>, String> {
    finance_path(true, initial, drift, volatility, times, shocks)
}

fn finance_path(
    geometric: bool,
    initial: f64,
    drift: f64,
    volatility: f64,
    times: &[f64],
    shocks: &[f64],
) -> Result<Vec<f64>, String> {
    if times.is_empty() || times.len() != shocks.len() {
        return Err("time steps and normal shocks must have equal, non-empty lengths".to_string());
    }
    let mut result = vec![0.0; times.len() + 1];
    let mut error = [0; ERROR_CAPACITY];
    let ok = unsafe {
        coolbox_finance_path(
            if geometric { 1 } else { 0 },
            initial,
            drift,
            volatility,
            times.as_ptr(),
            times.len(),
            shocks.as_ptr(),
            result.as_mut_ptr(),
            error.as_mut_ptr(),
            error.len(),
        )
    };
    check(ok, &error, "finance path generation failed")?;
    Ok(result)
}

pub fn ito_integral(integrand: &[f64], increments: &[f64]) -> Result<f64, String> {
    ito_value(false, integrand, increments)
}

pub fn ito_isometry_variance(integrand: &[f64], time_steps: &[f64]) -> Result<f64, String> {
    ito_value(true, integrand, time_steps)
}

fn ito_value(variance: bool, values: &[f64], increments: &[f64]) -> Result<f64, String> {
    if values.is_empty() || values.len() != increments.len() {
        return Err("integrand and increments must have equal, non-empty lengths".to_string());
    }
    let mut result = 0.0;
    let mut error = [0; ERROR_CAPACITY];
    let ok = unsafe {
        coolbox_finance_ito(
            if variance { 1 } else { 0 },
            values.as_ptr(),
            increments.as_ptr(),
            values.len(),
            &mut result,
            error.as_mut_ptr(),
            error.len(),
        )
    };
    check(ok, &error, "Ito calculation failed")?;
    Ok(result)
}

pub fn estimate_monte_carlo(
    contract: i32,
    spot: f64,
    strike: f64,
    barrier: f64,
    rate: f64,
    volatility: f64,
    times: &[f64],
    shock_paths: &[Vec<f64>],
    confidence_z: f64,
) -> Result<MonteCarloEstimate, String> {
    let flat = flatten_shocks(times, shock_paths)?;
    let mut result = MonteCarloEstimate::default();
    let mut error = [0; ERROR_CAPACITY];
    let ok = unsafe {
        coolbox_finance_monte_carlo(
            contract,
            spot,
            strike,
            barrier,
            rate,
            volatility,
            times.as_ptr(),
            times.len(),
            flat.as_ptr(),
            shock_paths.len(),
            confidence_z,
            0.01,
            0.01,
            &mut result,
            error.as_mut_ptr(),
            error.len(),
        )
    };
    check(ok, &error, "Monte Carlo estimate failed")?;
    Ok(result)
}

pub fn estimate_monte_carlo_greeks(
    contract: i32,
    spot: f64,
    strike: f64,
    barrier: f64,
    rate: f64,
    volatility: f64,
    times: &[f64],
    shock_paths: &[Vec<f64>],
    confidence_z: f64,
    relative_bump: f64,
    time_bump: f64,
) -> Result<MonteCarloGreeks, String> {
    let flat = flatten_shocks(times, shock_paths)?;
    let mut result = MonteCarloGreeks::default();
    let mut error = [0; ERROR_CAPACITY];
    let ok = unsafe {
        coolbox_finance_monte_carlo_greeks(
            contract,
            spot,
            strike,
            barrier,
            rate,
            volatility,
            times.as_ptr(),
            times.len(),
            flat.as_ptr(),
            shock_paths.len(),
            confidence_z,
            relative_bump,
            time_bump,
            &mut result,
            error.as_mut_ptr(),
            error.len(),
        )
    };
    check(ok, &error, "Monte Carlo Greeks failed")?;
    Ok(result)
}

fn flatten_shocks(times: &[f64], paths: &[Vec<f64>]) -> Result<Vec<f64>, String> {
    if times.is_empty() || paths.is_empty() {
        return Err("time steps and shock paths must not be empty".to_string());
    }
    if paths.iter().any(|path| path.len() != times.len()) {
        return Err("each normal shock path must match the number of time steps".to_string());
    }
    let capacity = paths
        .len()
        .checked_mul(times.len())
        .ok_or_else(|| "normal shock dimensions overflow".to_string())?;
    let mut flat = Vec::with_capacity(capacity);
    flat.extend(paths.iter().flatten().copied());
    Ok(flat)
}

struct PayoffContext<'a> {
    payoff: &'a mut dyn FnMut(&[f64]) -> Result<f64, String>,
    error: Option<String>,
}

extern "C" fn payoff_callback(
    user_data: usize,
    path: *const f64,
    count: usize,
    output: *mut f64,
) -> c_int {
    let context = unsafe { &mut *(user_data as *mut PayoffContext<'_>) };
    let values = unsafe { slice::from_raw_parts(path, count) };
    match catch_unwind(AssertUnwindSafe(|| (context.payoff)(values))) {
        Ok(Ok(payoff)) if payoff.is_finite() => {
            unsafe { *output = payoff };
            1
        }
        Ok(Ok(_)) => {
            context.error = Some("custom Monte Carlo payoff must be finite".to_string());
            0
        }
        Ok(Err(error)) => {
            context.error = Some(error);
            0
        }
        Err(_) => {
            context.error = Some("custom Monte Carlo payoff callback panicked".to_string());
            0
        }
    }
}

pub fn estimate_monte_carlo_custom_payoff<F>(
    spot: f64,
    rate: f64,
    volatility: f64,
    times: &[f64],
    shock_paths: &[Vec<f64>],
    confidence_z: f64,
    payoff: &mut F,
) -> Result<MonteCarloEstimate, String>
where
    F: FnMut(&[f64]) -> Result<f64, String>,
{
    let flat = flatten_shocks(times, shock_paths)?;
    let mut context = PayoffContext {
        payoff,
        error: None,
    };
    let mut result = MonteCarloEstimate::default();
    let mut error = [0; ERROR_CAPACITY];
    let ok = unsafe {
        coolbox_finance_monte_carlo_generic(
            spot,
            rate,
            volatility,
            times.as_ptr(),
            times.len(),
            flat.as_ptr(),
            shock_paths.len(),
            confidence_z,
            payoff_callback,
            (&mut context as *mut PayoffContext<'_>) as usize,
            &mut result,
            error.as_mut_ptr(),
            error.len(),
        )
    };
    if let Some(error) = context.error {
        return Err(error);
    }
    check(ok, &error, "custom Monte Carlo estimate failed")?;
    Ok(result)
}

pub fn estimate_monte_carlo_custom_payoff_greeks<F>(
    spot: f64,
    strike: f64,
    rate: f64,
    volatility: f64,
    times: &[f64],
    shock_paths: &[Vec<f64>],
    confidence_z: f64,
    relative_bump: f64,
    time_bump: f64,
    payoff: &mut F,
) -> Result<MonteCarloGreeks, String>
where
    F: FnMut(&[f64]) -> Result<f64, String>,
{
    let flat = flatten_shocks(times, shock_paths)?;
    let mut context = PayoffContext {
        payoff,
        error: None,
    };
    let mut result = MonteCarloGreeks::default();
    let mut error = [0; ERROR_CAPACITY];
    let ok = unsafe {
        coolbox_finance_monte_carlo_greeks_generic(
            spot,
            strike,
            rate,
            volatility,
            times.as_ptr(),
            times.len(),
            flat.as_ptr(),
            shock_paths.len(),
            confidence_z,
            relative_bump,
            time_bump,
            payoff_callback,
            (&mut context as *mut PayoffContext<'_>) as usize,
            &mut result,
            error.as_mut_ptr(),
            error.len(),
        )
    };
    if let Some(error) = context.error {
        return Err(error);
    }
    check(ok, &error, "custom Monte Carlo Greeks failed")?;
    Ok(result)
}

pub fn naive_average_parameters(models: &[Vec<f64>]) -> Result<Vec<f64>, String> {
    let width = models.first().map_or(0, Vec::len);
    if width == 0 || models.iter().any(|model| model.len() != width) {
        return Err("parameter models must have matching non-empty lengths".to_string());
    }
    let flat: Vec<f64> = models.iter().flatten().copied().collect();
    let mut result = vec![0.0; width];
    let mut error = [0; ERROR_CAPACITY];
    let ok = unsafe {
        coolbox_owa_naive_average(
            flat.as_ptr(),
            models.len(),
            width,
            result.as_mut_ptr(),
            error.as_mut_ptr(),
            error.len(),
        )
    };
    check(ok, &error, "parameter averaging failed")?;
    Ok(result)
}

pub struct OwaResult {
    pub merged_parameters: Vec<f64>,
    pub model_weights: Vec<f64>,
    pub validation_loss: f64,
}

pub fn optimal_weighted_average_linear_regression(
    models: &[Vec<f64>],
    features: &[Vec<f64>],
    targets: &[f64],
    iterations: i32,
    learning_rate: f64,
    regularization: f64,
    project_to_simplex: bool,
) -> Result<OwaResult, String> {
    let parameter_count = models.first().map_or(0, Vec::len);
    let feature_count = features.first().map_or(0, Vec::len);
    if parameter_count == 0
        || feature_count == 0
        || models.iter().any(|model| model.len() != parameter_count)
        || features.iter().any(|row| row.len() != feature_count)
        || targets.len() != features.len()
        || parameter_count != feature_count
    {
        return Err("OWA model parameters and validation data are incompatible".to_string());
    }
    let parameter_flat: Vec<f64> = models.iter().flatten().copied().collect();
    let feature_flat: Vec<f64> = features.iter().flatten().copied().collect();
    let mut merged_parameters = vec![0.0; parameter_count];
    let mut model_weights = vec![0.0; models.len()];
    let mut loss = 0.0;
    let mut error = [0; ERROR_CAPACITY];
    let ok = unsafe {
        coolbox_owa_linear_regression(
            parameter_flat.as_ptr(),
            models.len(),
            parameter_count,
            feature_flat.as_ptr(),
            features.len(),
            feature_count,
            targets.as_ptr(),
            targets.len(),
            iterations,
            learning_rate,
            regularization,
            if project_to_simplex { 1 } else { 0 },
            merged_parameters.as_mut_ptr(),
            model_weights.as_mut_ptr(),
            &mut loss,
            error.as_mut_ptr(),
            error.len(),
        )
    };
    check(ok, &error, "optimal weighted averaging failed")?;
    Ok(OwaResult {
        merged_parameters,
        model_weights,
        validation_loss: loss,
    })
}

#[derive(Debug, Clone, Copy)]
pub struct OwaConfig {
    pub iterations: i32,
    pub learning_rate: f64,
    pub regularization: f64,
    pub project_to_simplex: bool,
}

impl Default for OwaConfig {
    fn default() -> Self {
        Self {
            iterations: 500,
            learning_rate: 0.05,
            regularization: 1e-4,
            project_to_simplex: true,
        }
    }
}

pub struct OwaFold {
    pub features: Vec<Vec<f64>>,
    pub targets: Vec<f64>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct OwaCrossValidationResult {
    pub mean_loss: f64,
    pub fold_losses: Vec<f64>,
}

pub fn fast_owa_cross_validation_linear_regression(
    models: &[Vec<f64>],
    folds: &[OwaFold],
    config: OwaConfig,
) -> Result<OwaCrossValidationResult, String> {
    let parameter_count = models.first().map_or(0, Vec::len);
    if models.len() < 2
        || folds.len() != models.len()
        || parameter_count == 0
        || models.iter().any(|model| model.len() != parameter_count)
        || config.iterations < 0
    {
        return Err(
            "OWA cross-validation requires matching models, folds, and valid configuration".into(),
        );
    }
    let feature_count = parameter_count;
    let parameters: Vec<f64> = models.iter().flatten().copied().collect();
    let mut fold_features = Vec::new();
    let mut fold_targets = Vec::new();
    let mut row_offsets = Vec::with_capacity(folds.len() + 1);
    let mut target_offsets = Vec::with_capacity(folds.len() + 1);
    row_offsets.push(0usize);
    target_offsets.push(0usize);
    for (index, fold) in folds.iter().enumerate() {
        if fold.features.is_empty()
            || fold.features.len() != fold.targets.len()
            || fold.features.iter().any(|row| row.len() != feature_count)
        {
            return Err(format!(
                "OWA fold {index} has incompatible features and targets"
            ));
        }
        fold_features.extend(fold.features.iter().flatten().copied());
        fold_targets.extend(fold.targets.iter().copied());
        row_offsets.push(fold_features.len() / feature_count);
        target_offsets.push(fold_targets.len());
    }
    let mut fold_losses = vec![0.0; folds.len()];
    let mut mean_loss = 0.0;
    let mut error = [0; ERROR_CAPACITY];
    let ok = unsafe {
        coolbox_owa_cross_validation_linear_regression(
            parameters.as_ptr(),
            models.len(),
            parameter_count,
            fold_features.as_ptr(),
            row_offsets.as_ptr(),
            fold_targets.as_ptr(),
            target_offsets.as_ptr(),
            folds.len(),
            feature_count,
            config.iterations,
            config.learning_rate,
            config.regularization,
            if config.project_to_simplex { 1 } else { 0 },
            fold_losses.as_mut_ptr(),
            &mut mean_loss,
            error.as_mut_ptr(),
            error.len(),
        )
    };
    check(ok, &error, "OWA cross-validation failed")?;
    Ok(OwaCrossValidationResult {
        mean_loss,
        fold_losses,
    })
}

#[derive(Debug, Clone, PartialEq)]
pub struct TensorSnapshot {
    pub shape: Vec<usize>,
    pub values: Vec<f64>,
}

pub struct NowaResult {
    pub merged_layers: Vec<Tensor>,
    pub layer_weights: Vec<Vec<f64>>,
    pub validation_loss: f64,
}

struct NowaLossContext<'a> {
    loss: &'a mut dyn FnMut(&[TensorSnapshot]) -> Result<f64, String>,
    shapes: Vec<Vec<usize>>,
    error: Option<String>,
}

extern "C" fn nowa_loss_callback(
    user_data: usize,
    values: *const f64,
    layer_sizes: *const usize,
    layer_count: usize,
    output: *mut f64,
) -> c_int {
    let context = unsafe { &mut *(user_data as *mut NowaLossContext<'_>) };
    let sizes = unsafe { slice::from_raw_parts(layer_sizes, layer_count) };
    if sizes.len() != context.shapes.len() {
        context.error = Some("NOWA callback received an unexpected layer count".to_string());
        return 0;
    }
    let Some(total) = sizes
        .iter()
        .try_fold(0usize, |total, size| total.checked_add(*size))
    else {
        context.error = Some("NOWA callback tensor data size overflows".to_string());
        return 0;
    };
    let flat = unsafe { slice::from_raw_parts(values, total) };
    let mut offset = 0;
    let layers = sizes
        .iter()
        .zip(&context.shapes)
        .map(|(&size, shape)| {
            let end = offset + size;
            let snapshot = TensorSnapshot {
                shape: shape.clone(),
                values: flat[offset..end].to_vec(),
            };
            offset = end;
            snapshot
        })
        .collect::<Vec<_>>();
    match catch_unwind(AssertUnwindSafe(|| (context.loss)(&layers))) {
        Ok(Ok(loss)) if loss.is_finite() => {
            unsafe { *output = loss };
            1
        }
        Ok(Ok(_)) => {
            context.error = Some("NOWA validation loss must be finite".to_string());
            0
        }
        Ok(Err(error)) => {
            context.error = Some(error);
            0
        }
        Err(_) => {
            context.error = Some("NOWA validation callback panicked".to_string());
            0
        }
    }
}

pub fn nonlinear_optimal_weighted_average_layers<F>(
    models: &[Vec<Tensor>],
    loss: &mut F,
    config: OwaConfig,
) -> Result<NowaResult, String>
where
    F: FnMut(&[TensorSnapshot]) -> Result<f64, String>,
{
    let first_model = models
        .first()
        .ok_or_else(|| "NOWA requires at least one local model".to_string())?;
    let layer_count = first_model.len();
    if layer_count == 0 || config.iterations < 0 {
        return Err("NOWA requires model layers and a non-negative iteration count".into());
    }
    if models.iter().any(|model| model.len() != layer_count) {
        return Err("NOWA local models must have matching layer counts".into());
    }
    let mut shapes = Vec::with_capacity(layer_count);
    let mut layer_sizes = Vec::with_capacity(layer_count);
    let mut layer_shape_offsets = Vec::with_capacity(layer_count + 1);
    let mut layer_value_offsets = Vec::with_capacity(layer_count + 1);
    let mut local_layers = Vec::new();
    layer_shape_offsets.push(0usize);
    layer_value_offsets.push(0usize);
    for (layer_index, tensor) in first_model.iter().enumerate() {
        let shape = tensor.shape()?;
        let values = tensor.data()?;
        shapes.push(shape.clone());
        layer_sizes.push(values.len());
        layer_shape_offsets.push(
            layer_shape_offsets[layer_index]
                .checked_add(shape.len())
                .ok_or_else(|| "NOWA shape dimensions overflow".to_string())?,
        );
        layer_value_offsets.push(
            layer_value_offsets[layer_index]
                .checked_add(values.len())
                .ok_or_else(|| "NOWA tensor data size overflows".to_string())?,
        );
    }
    let layer_dimensions: Vec<usize> = shapes.iter().map(Vec::len).collect();
    let layer_shapes: Vec<usize> = shapes.iter().flatten().copied().collect();
    let values_per_model = *layer_value_offsets.last().unwrap_or(&0);
    let input_size = models
        .len()
        .checked_mul(values_per_model)
        .ok_or_else(|| "NOWA input dimensions overflow".to_string())?;
    local_layers.reserve(input_size);
    for (model_index, model) in models.iter().enumerate() {
        for (layer_index, tensor) in model.iter().enumerate() {
            if tensor.shape()? != shapes[layer_index] {
                return Err(format!(
                    "model {model_index} layer {layer_index} has a different tensor shape"
                ));
            }
            local_layers.extend(tensor.data()?);
        }
    }
    let weight_count = models
        .len()
        .checked_mul(layer_count)
        .ok_or_else(|| "NOWA weight dimensions overflow".to_string())?;
    let mut merged_flat = vec![0.0; values_per_model];
    let mut layer_weights = vec![0.0; weight_count];
    let mut validation_loss = 0.0;
    let mut context = NowaLossContext {
        loss,
        shapes: shapes.clone(),
        error: None,
    };
    let user_data = (&mut context as *mut NowaLossContext<'_>) as usize;
    let mut error = [0; ERROR_CAPACITY];
    let ok = unsafe {
        coolbox_owa_nonlinear_layers(
            local_layers.as_ptr(),
            models.len(),
            layer_count,
            layer_shapes.as_ptr(),
            layer_shape_offsets.as_ptr(),
            layer_sizes.as_ptr(),
            layer_value_offsets.as_ptr(),
            layer_dimensions.as_ptr(),
            config.iterations,
            config.learning_rate,
            config.regularization,
            if config.project_to_simplex { 1 } else { 0 },
            nowa_loss_callback,
            user_data,
            merged_flat.as_mut_ptr(),
            layer_weights.as_mut_ptr(),
            &mut validation_loss,
            error.as_mut_ptr(),
            error.len(),
        )
    };
    if let Some(error) = context.error {
        return Err(error);
    }
    check(ok, &error, "nonlinear weighted averaging failed")?;
    let mut merged_layers = Vec::with_capacity(layer_count);
    for (layer, shape) in shapes.iter().enumerate() {
        let start = layer_value_offsets[layer];
        let end = layer_value_offsets[layer + 1];
        merged_layers.push(Tensor::new(shape, Some(&merged_flat[start..end]))?);
    }
    let weights = layer_weights
        .chunks_exact(models.len())
        .map(<[f64]>::to_vec)
        .collect();
    Ok(NowaResult {
        merged_layers,
        layer_weights: weights,
        validation_loss,
    })
}

pub fn weighted_tensor_average(
    tensors: &[&Tensor],
    weights: &[f64],
) -> Result<Tensor, String> {
    let first = tensors
        .first()
        .ok_or_else(|| "weighted tensor averaging requires tensors".to_string())?;
    if tensors.len() != weights.len() {
        return Err("weighted tensor averaging requires one weight per tensor".into());
    }
    let shape = first.shape()?;
    if shape.is_empty() {
        return Err("weighted tensor shape must not be empty".into());
    }
    let size = shape
        .iter()
        .try_fold(1usize, |size, dimension| size.checked_mul(*dimension))
        .ok_or_else(|| "weighted tensor shape size overflows".to_string())?;
    let mut local_tensors = Vec::with_capacity(
        tensors
            .len()
            .checked_mul(size)
            .ok_or_else(|| "weighted tensor data size overflows".to_string())?,
    );
    for (index, tensor) in tensors.iter().enumerate() {
        if tensor.shape()? != shape {
            return Err(format!("tensor {index} has a different shape"));
        }
        let values = tensor.data()?;
        if values.len() != size {
            return Err(format!("tensor {index} data does not match its shape"));
        }
        local_tensors.extend(values);
    }
    let mut result = vec![0.0; size];
    let mut error = [0; ERROR_CAPACITY];
    let ok = unsafe {
        coolbox_owa_weighted_tensor_average(
            local_tensors.as_ptr(),
            tensors.len(),
            shape.as_ptr(),
            shape.len(),
            weights.as_ptr(),
            size,
            result.as_mut_ptr(),
            error.as_mut_ptr(),
            error.len(),
        )
    };
    check(ok, &error, "weighted tensor averaging failed")?;
    Tensor::new(&shape, Some(&result))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn prices_options_and_generates_paths() {
        let call = evaluate_finance_formula(
            FinanceFormula::BlackScholesCallPrice,
            &[100.0, 100.0, 0.05, 0.2, 1.0],
        )
        .unwrap();
        assert!((call - 10.4505835722).abs() < 1e-6);
        assert_eq!(
            brownian_motion_path(&[0.25, 0.25], &[0.0, 2.0], 0.0).unwrap(),
            vec![0.0, 0.0, 1.0]
        );
    }

    #[test]
    fn solves_heat_and_runs_optimizer_callback() {
        let result = solve_heat_equation(
            &HeatGrid {
                values: vec![0.0, 1.0, 0.0],
                nx: 3,
                ny: 1,
                nz: 1,
            },
            1,
            1.0,
            0.0,
            [1.0; 3],
            0.1,
            1,
            42,
            false,
        )
        .unwrap();
        assert!(result.values[1] > 0.0 && result.values[1] < 1.0);

        let mut optimizer = SearchOptimizer::new(
            OptimizerType::HillClimbing,
            &[1.0, 40.0, 20.0, 1.0, 0.25, -10.0, 10.0, 2026.0],
        )
        .unwrap();
        let solution = optimizer
            .optimize(&mut |point| point[0] * point[0], &[4.0])
            .unwrap();
        assert_eq!(solution.len(), 1);
        assert!(optimizer.best_score().unwrap() <= 16.0);
    }

    #[test]
    fn merges_distributed_model_parameters() {
        assert_eq!(
            naive_average_parameters(&[vec![1.0, 3.0], vec![3.0, 5.0]]).unwrap(),
            vec![2.0, 4.0]
        );
    }

    #[test]
    fn estimates_custom_payoffs_and_propagates_callback_errors() {
        let estimate = estimate_monte_carlo_custom_payoff(
            100.0,
            0.05,
            0.2,
            &[1.0],
            &[vec![0.0]],
            1.959963984540054,
            &mut |path| Ok(path[1]),
        )
        .unwrap();
        assert!(estimate.price.is_finite());
        assert_eq!(estimate.sample_count, 1);
        let greeks = estimate_monte_carlo_custom_payoff_greeks(
            100.0,
            100.0,
            0.05,
            0.2,
            &[1.0],
            &[vec![0.0]],
            1.959963984540054,
            0.01,
            0.01,
            &mut |path| Ok(path[1]),
        )
        .unwrap();
        assert!(greeks.delta.price.is_finite());

        let error = estimate_monte_carlo_custom_payoff(
            100.0,
            0.05,
            0.2,
            &[1.0],
            &[vec![0.0]],
            1.959963984540054,
            &mut |_| Err("payoff rejected path".to_string()),
        )
        .unwrap_err();
        assert_eq!(error, "payoff rejected path");
    }

    #[test]
    fn exposes_owa_cross_validation_and_tensor_layer_merging() {
        let cross_validation = fast_owa_cross_validation_linear_regression(
            &[vec![0.0, 1.0], vec![2.0, 1.0], vec![4.0, 3.0]],
            &[
                OwaFold {
                    features: vec![vec![1.0, 2.0]],
                    targets: vec![2.0],
                },
                OwaFold {
                    features: vec![vec![2.0, 1.0]],
                    targets: vec![1.0],
                },
                OwaFold {
                    features: vec![vec![1.0, 1.0]],
                    targets: vec![3.0],
                },
            ],
            OwaConfig {
                iterations: 10,
                ..OwaConfig::default()
            },
        )
        .unwrap();
        assert_eq!(cross_validation.fold_losses.len(), 3);
        assert!(cross_validation.mean_loss.is_finite());

        let models = vec![
            vec![
                Tensor::new(&[2], Some(&[1.0, 2.0])).unwrap(),
                Tensor::new(&[1], Some(&[3.0])).unwrap(),
            ],
            vec![
                Tensor::new(&[2], Some(&[3.0, 1.0])).unwrap(),
                Tensor::new(&[1], Some(&[5.0])).unwrap(),
            ],
        ];
        let average = weighted_tensor_average(&[&models[0][0], &models[1][0]], &[0.5, 0.5])
            .unwrap();
        assert_eq!(average.data().unwrap(), vec![2.0, 1.5]);
        let result = nonlinear_optimal_weighted_average_layers(
            &models,
            &mut |layers| {
                Ok(layers
                    .iter()
                    .map(|layer| layer.values.iter().sum::<f64>())
                    .sum())
            },
            OwaConfig {
                iterations: 10,
                ..OwaConfig::default()
            },
        )
        .unwrap();
        assert_eq!(result.merged_layers.len(), 2);
        assert_eq!(result.merged_layers[0].shape().unwrap(), vec![2]);
        assert_eq!(result.layer_weights.len(), 2);
        assert!(result.validation_loss.is_finite());
        let error = nonlinear_optimal_weighted_average_layers(
            &models,
            &mut |_| Err("validation loss rejected layers".to_string()),
            OwaConfig {
                iterations: 10,
                ..OwaConfig::default()
            },
        )
        .err()
        .expect("NOWA callback error should be returned");
        assert_eq!(error, "validation loss rejected layers");
    }
}
