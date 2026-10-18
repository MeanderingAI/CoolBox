use std::ffi::{c_char, c_void, CStr};
use std::marker::PhantomData;
use std::ptr::NonNull;

const ERROR_CAPACITY: usize = 512;

unsafe extern "C" {
    fn coolbox_matrix_new(
        rows: usize,
        columns: usize,
        values: *const f64,
        count: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_matrix_identity(
        size: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_matrix_free(matrix: *mut c_void);
    fn coolbox_matrix_rows(matrix: *const c_void) -> usize;
    fn coolbox_matrix_columns(matrix: *const c_void) -> usize;
    fn coolbox_matrix_copy_data(
        matrix: *const c_void,
        output: *mut f64,
        capacity: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> i32;
    fn coolbox_matrix_get(
        matrix: *const c_void,
        row: usize,
        column: usize,
        value: *mut f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> i32;
    fn coolbox_matrix_set(
        matrix: *mut c_void,
        row: usize,
        column: usize,
        value: f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> i32;
    fn coolbox_matrix_binary(
        lhs: *const c_void,
        rhs: *const c_void,
        operation: i32,
        optimization: i32,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_matrix_transpose(
        matrix: *const c_void,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_matrix_rowwise_add(
        matrix: *const c_void,
        vector: *const c_void,
        optimization: i32,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_matrix_norm(matrix: *const c_void) -> f64;

    fn coolbox_vector_new(
        values: *const f64,
        size: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_vector_free(vector: *mut c_void);
    fn coolbox_vector_size(vector: *const c_void) -> usize;
    fn coolbox_vector_copy_data(
        vector: *const c_void,
        output: *mut f64,
        capacity: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> i32;
    fn coolbox_vector_get(
        vector: *const c_void,
        index: usize,
        value: *mut f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> i32;
    fn coolbox_vector_set(
        vector: *mut c_void,
        index: usize,
        value: f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> i32;
    fn coolbox_vector_norm(vector: *const c_void) -> f64;
    fn coolbox_vector_dot(
        lhs: *const c_void,
        rhs: *const c_void,
        optimization: i32,
        result: *mut f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> i32;
    fn coolbox_vector_add(
        lhs: *const c_void,
        rhs: *const c_void,
        optimization: i32,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_vector_transpose(
        vector: *const c_void,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_cpu_optimization_name(optimization: i32) -> *const c_char;
    fn coolbox_cpu_optimization_available(optimization: i32) -> i32;
    fn coolbox_matrix_set_backend(backend: i32, error: *mut c_char, error_capacity: usize) -> i32;
    fn coolbox_matrix_get_backend() -> i32;
    fn coolbox_matrix_get_active_backend() -> i32;
    fn coolbox_matrix_backend_name(backend: i32) -> *const c_char;
    fn coolbox_matrix_backend_available(backend: i32) -> i32;
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
        error: &[c_char],
        fallback: &str,
    ) -> Result<Self, String> {
        let message = native_error(error, fallback);
        Ok(Self {
            ptr: NonNull::new(ptr).ok_or(message)?,
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

fn native_error(error: &[c_char], fallback: &str) -> String {
    let message = unsafe { CStr::from_ptr(error.as_ptr()) }.to_string_lossy();
    if message.is_empty() {
        fallback.to_string()
    } else {
        message.into_owned()
    }
}

fn check(ok: i32, error: &[c_char], fallback: &str) -> Result<(), String> {
    if ok != 0 {
        Ok(())
    } else {
        Err(native_error(error, fallback))
    }
}

fn matrix_count(rows: usize, columns: usize) -> Result<usize, String> {
    if rows == 0 || columns == 0 {
        return Err("matrix dimensions must be positive".into());
    }
    rows.checked_mul(columns)
        .ok_or_else(|| "matrix dimensions overflow usize".into())
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum CpuOptimization {
    Auto = 0,
    Scalar = 1,
    Avx2Fma = 2,
    Neon = 3,
}

impl CpuOptimization {
    pub fn name(self) -> String {
        let name = unsafe { coolbox_cpu_optimization_name(self as i32) };
        if name.is_null() {
            String::new()
        } else {
            unsafe { CStr::from_ptr(name) }
                .to_string_lossy()
                .into_owned()
        }
    }
    pub fn is_available(self) -> bool {
        unsafe { coolbox_cpu_optimization_available(self as i32) != 0 }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum MatrixBackend {
    Cpu = 0,
    Eigen = 1,
    Auto = 2,
    OpenCl = 3,
    Cuda = 4,
    Metal = 5,
}

impl MatrixBackend {
    pub fn set(self) -> Result<(), String> {
        let mut error = [0; ERROR_CAPACITY];
        let ok =
            unsafe { coolbox_matrix_set_backend(self as i32, error.as_mut_ptr(), error.len()) };
        check(ok, &error, "failed to set matrix backend")
    }

    pub fn current() -> Result<Self, String> {
        Self::from_native(unsafe { coolbox_matrix_get_backend() })
    }

    pub fn active() -> Result<Self, String> {
        Self::from_native(unsafe { coolbox_matrix_get_active_backend() })
    }

    fn from_native(value: i32) -> Result<Self, String> {
        match value {
            0 => Ok(Self::Cpu),
            1 => Ok(Self::Eigen),
            2 => Ok(Self::Auto),
            3 => Ok(Self::OpenCl),
            4 => Ok(Self::Cuda),
            5 => Ok(Self::Metal),
            value => Err(format!("unknown native matrix backend {value}")),
        }
    }

    pub fn name(self) -> String {
        let name = unsafe { coolbox_matrix_backend_name(self as i32) };
        if name.is_null() {
            String::new()
        } else {
            unsafe { CStr::from_ptr(name) }
                .to_string_lossy()
                .into_owned()
        }
    }

    pub fn is_available(self) -> bool {
        unsafe { coolbox_matrix_backend_available(self as i32) != 0 }
    }
}

pub struct DenseMatrix {
    handle: Handle,
}

impl DenseMatrix {
    pub fn new(rows: usize, columns: usize) -> Result<Self, String> {
        let count = matrix_count(rows, columns)?;
        Self::from_data(rows, columns, &vec![0.0; count])
    }
    pub fn zeros(rows: usize, columns: usize) -> Result<Self, String> {
        Self::new(rows, columns)
    }
    pub fn ones(rows: usize, columns: usize) -> Result<Self, String> {
        let count = matrix_count(rows, columns)?;
        Self::from_data(rows, columns, &vec![1.0; count])
    }
    pub fn from_data(rows: usize, columns: usize, values: &[f64]) -> Result<Self, String> {
        let count = matrix_count(rows, columns)?;
        if values.len() != count {
            return Err("matrix data length does not match dimensions".into());
        }
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_matrix_new(
                rows,
                columns,
                values.as_ptr(),
                values.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Ok(Self {
            handle: Handle::new(ptr, coolbox_matrix_free, &error, "failed to create matrix")?,
        })
    }
    pub fn identity(size: usize) -> Result<Self, String> {
        matrix_count(size, size)?;
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe { coolbox_matrix_identity(size, error.as_mut_ptr(), error.len()) };
        Ok(Self {
            handle: Handle::new(
                ptr,
                coolbox_matrix_free,
                &error,
                "failed to create identity matrix",
            )?,
        })
    }
    pub fn rows(&self) -> usize {
        unsafe { coolbox_matrix_rows(self.handle.as_ptr()) }
    }
    pub fn columns(&self) -> usize {
        unsafe { coolbox_matrix_columns(self.handle.as_ptr()) }
    }
    pub fn shape(&self) -> (usize, usize) {
        (self.rows(), self.columns())
    }
    pub fn data(&self) -> Result<Vec<f64>, String> {
        let mut result = vec![0.0; self.rows() * self.columns()];
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_matrix_copy_data(
                self.handle.as_ptr(),
                result.as_mut_ptr(),
                result.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to read matrix data")?;
        Ok(result)
    }
    pub fn get(&self, row: usize, column: usize) -> Result<f64, String> {
        let mut value = 0.0;
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_matrix_get(
                self.handle.as_ptr(),
                row,
                column,
                &mut value,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "matrix index out of range")?;
        Ok(value)
    }
    pub fn set(&mut self, row: usize, column: usize, value: f64) -> Result<(), String> {
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_matrix_set(
                self.handle.as_ptr(),
                row,
                column,
                value,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "matrix index out of range")
    }
    fn binary(
        &self,
        other: &Self,
        operation: i32,
        optimization: CpuOptimization,
    ) -> Result<Self, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_matrix_binary(
                self.handle.as_ptr(),
                other.handle.as_ptr(),
                operation,
                optimization as i32,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Ok(Self {
            handle: Handle::new(ptr, coolbox_matrix_free, &error, "matrix operation failed")?,
        })
    }
    pub fn add(&self, other: &Self, optimization: CpuOptimization) -> Result<Self, String> {
        self.binary(other, 0, optimization)
    }
    pub fn subtract(&self, other: &Self) -> Result<Self, String> {
        self.binary(other, 1, CpuOptimization::Auto)
    }
    pub fn multiply(&self, other: &Self, optimization: CpuOptimization) -> Result<Self, String> {
        self.binary(other, 2, optimization)
    }
    pub fn transpose(&self) -> Result<Self, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_matrix_transpose(self.handle.as_ptr(), error.as_mut_ptr(), error.len())
        };
        Ok(Self {
            handle: Handle::new(ptr, coolbox_matrix_free, &error, "matrix transpose failed")?,
        })
    }
    pub fn rowwise_add(
        &self,
        vector: &DenseVector,
        optimization: CpuOptimization,
    ) -> Result<Self, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_matrix_rowwise_add(
                self.handle.as_ptr(),
                vector.handle.as_ptr(),
                optimization as i32,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Ok(Self {
            handle: Handle::new(ptr, coolbox_matrix_free, &error, "row-wise addition failed")?,
        })
    }
    pub fn norm(&self) -> f64 {
        unsafe { coolbox_matrix_norm(self.handle.as_ptr()) }
    }
}

pub struct DenseVector {
    handle: Handle,
}

impl DenseVector {
    pub fn new(size: usize) -> Result<Self, String> {
        Self::from_data(&vec![0.0; size])
    }
    pub fn from_data(values: &[f64]) -> Result<Self, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_vector_new(
                values.as_ptr(),
                values.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Ok(Self {
            handle: Handle::new(ptr, coolbox_vector_free, &error, "failed to create vector")?,
        })
    }
    pub fn size(&self) -> usize {
        unsafe { coolbox_vector_size(self.handle.as_ptr()) }
    }
    pub fn data(&self) -> Result<Vec<f64>, String> {
        let mut result = vec![0.0; self.size()];
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_vector_copy_data(
                self.handle.as_ptr(),
                result.as_mut_ptr(),
                result.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to read vector data")?;
        Ok(result)
    }
    pub fn get(&self, index: usize) -> Result<f64, String> {
        let mut value = 0.0;
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_vector_get(
                self.handle.as_ptr(),
                index,
                &mut value,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "vector index out of range")?;
        Ok(value)
    }
    pub fn set(&mut self, index: usize, value: f64) -> Result<(), String> {
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_vector_set(
                self.handle.as_ptr(),
                index,
                value,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "vector index out of range")
    }
    pub fn norm(&self) -> f64 {
        unsafe { coolbox_vector_norm(self.handle.as_ptr()) }
    }
    pub fn dot(&self, other: &Self, optimization: CpuOptimization) -> Result<f64, String> {
        let mut error = [0; ERROR_CAPACITY];
        let mut result = 0.0;
        let ok = unsafe {
            coolbox_vector_dot(
                self.handle.as_ptr(),
                other.handle.as_ptr(),
                optimization as i32,
                &mut result,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "vector dot product failed")?;
        Ok(result)
    }
    pub fn add(&self, other: &Self, optimization: CpuOptimization) -> Result<Self, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_vector_add(
                self.handle.as_ptr(),
                other.handle.as_ptr(),
                optimization as i32,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Ok(Self {
            handle: Handle::new(ptr, coolbox_vector_free, &error, "vector addition failed")?,
        })
    }
    pub fn transpose(&self) -> Result<DenseMatrix, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_vector_transpose(self.handle.as_ptr(), error.as_mut_ptr(), error.len())
        };
        Ok(DenseMatrix {
            handle: Handle::new(ptr, coolbox_matrix_free, &error, "vector transpose failed")?,
        })
    }
}
