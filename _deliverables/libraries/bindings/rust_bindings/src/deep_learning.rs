use std::ffi::{c_char, c_int, c_void, CStr, CString};
use std::marker::PhantomData;
use std::ptr::NonNull;

const ERROR_CAPACITY: usize = 1024;

unsafe extern "C" {
    fn coolbox_dl_tensor_new(
        shape: *const usize,
        dimensions: usize,
        values: *const f64,
        value_count: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_dl_tensor_clone(
        tensor: *mut c_void,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_dl_tensor_reshape(
        tensor: *mut c_void,
        shape: *const usize,
        dimensions: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_dl_tensor_transpose(
        tensor: *mut c_void,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_dl_tensor_binary(
        lhs: *mut c_void,
        rhs: *mut c_void,
        operation: c_int,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_dl_tensor_scalar(
        tensor: *mut c_void,
        scalar: f64,
        divide: c_int,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_dl_tensor_matmul(
        lhs: *mut c_void,
        rhs: *mut c_void,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_dl_tensor_free(tensor: *mut c_void);
    fn coolbox_dl_tensor_dimensions(tensor: *mut c_void) -> usize;
    fn coolbox_dl_tensor_size(tensor: *mut c_void) -> usize;
    fn coolbox_dl_tensor_shape(
        tensor: *mut c_void,
        shape: *mut usize,
        capacity: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_tensor_data(
        tensor: *mut c_void,
        values: *mut f64,
        capacity: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_tensor_fill(
        tensor: *mut c_void,
        value: f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_tensor_randomize(
        tensor: *mut c_void,
        minimum: f64,
        maximum: f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_tensor_at(
        tensor: *mut c_void,
        index: usize,
        value: *mut f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_tensor_set(
        tensor: *mut c_void,
        index: usize,
        value: f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;

    fn coolbox_dl_layer_new(
        kind: c_int,
        parameters: *const usize,
        parameter_count: usize,
        rate: f64,
        return_sequences: c_int,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_dl_layer_free(layer: *mut c_void);
    fn coolbox_dl_layer_forward(
        layer: *mut c_void,
        input: *mut c_void,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_dl_layer_backward(
        layer: *mut c_void,
        gradient: *mut c_void,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_dl_layer_update(
        layer: *mut c_void,
        learning_rate: f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_layer_set_training(
        layer: *mut c_void,
        training: c_int,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_layer_has_parameters(layer: *mut c_void) -> c_int;
    fn coolbox_dl_layer_name(
        layer: *mut c_void,
        output: *mut c_char,
        capacity: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;

    fn coolbox_dl_loss_new(kind: c_int, error: *mut c_char, error_capacity: usize) -> *mut c_void;
    fn coolbox_dl_loss_free(loss: *mut c_void);
    fn coolbox_dl_loss_compute(
        loss: *mut c_void,
        predictions: *mut c_void,
        targets: *mut c_void,
        value: *mut f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_loss_gradient(
        loss: *mut c_void,
        predictions: *mut c_void,
        targets: *mut c_void,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_dl_optimizer_new(
        kind: c_int,
        learning_rate: f64,
        parameter1: f64,
        parameter2: f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_dl_optimizer_free(optimizer: *mut c_void);
    fn coolbox_dl_optimizer_step(
        optimizer: *mut c_void,
        parameters: *mut c_void,
        gradients: *mut c_void,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_optimizer_reset(
        optimizer: *mut c_void,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_optimizer_name(
        optimizer: *mut c_void,
        output: *mut c_char,
        capacity: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;

    fn coolbox_dl_network_new(error: *mut c_char, error_capacity: usize) -> *mut c_void;
    fn coolbox_dl_network_free(network: *mut c_void);
    fn coolbox_dl_network_add_layer(
        network: *mut c_void,
        layer: *mut c_void,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_network_set_loss(
        network: *mut c_void,
        loss: *mut c_void,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_network_set_optimizer(
        network: *mut c_void,
        optimizer: *mut c_void,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_network_predict(
        network: *mut c_void,
        input: *mut c_void,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_dl_network_train_step(
        network: *mut c_void,
        input: *mut c_void,
        target: *mut c_void,
        value: *mut f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_network_evaluate(
        network: *mut c_void,
        inputs: *mut *mut c_void,
        targets: *mut *mut c_void,
        count: usize,
        value: *mut f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_network_train(
        network: *mut c_void,
        inputs: *mut *mut c_void,
        targets: *mut *mut c_void,
        count: usize,
        epochs: c_int,
        batch_size: c_int,
        verbose: c_int,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_network_set_training(
        network: *mut c_void,
        training: c_int,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_network_layer_count(network: *mut c_void) -> usize;

    fn coolbox_dl_helix_new(
        actions: usize,
        truth: usize,
        rich_feedback: c_int,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_dl_helix_free(helix: *mut c_void);
    fn coolbox_dl_helix_select_action(
        helix: *mut c_void,
        action: *mut usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_helix_observe(
        helix: *mut c_void,
        action: usize,
        feedback: *const c_char,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_helix_interact(
        helix: *mut c_void,
        action: *mut usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_dl_helix_rounds(helix: *mut c_void) -> usize;
    fn coolbox_dl_helix_mistakes(helix: *mut c_void) -> usize;
    fn coolbox_dl_helix_cumulative_regret(helix: *mut c_void) -> f64;
    fn coolbox_dl_helix_identified(helix: *mut c_void) -> c_int;
    fn coolbox_dl_helix_version_space(
        helix: *mut c_void,
        indices: *mut usize,
        capacity: usize,
    ) -> usize;
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
        error: &[c_char],
    ) -> Result<Self, String> {
        Ok(Self {
            ptr: NonNull::new(ptr)
                .ok_or_else(|| error_message(error, &format!("failed to create {kind}")))?,
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

fn error_message(error: &[c_char], fallback: &str) -> String {
    let message = unsafe { CStr::from_ptr(error.as_ptr()) }.to_string_lossy();
    if message.is_empty() {
        fallback.to_string()
    } else {
        message.into_owned()
    }
}
fn check(ok: c_int, error: &[c_char], fallback: &str) -> Result<(), String> {
    if ok != 0 {
        Ok(())
    } else {
        Err(error_message(error, fallback))
    }
}

pub struct Tensor {
    handle: Handle,
}
impl Tensor {
    pub fn new(shape: &[usize], values: Option<&[f64]>) -> Result<Self, String> {
        if shape.is_empty() || shape.iter().any(|&dimension| dimension == 0) {
            return Err("tensor shape dimensions must be positive".into());
        }
        if let Some(values) = values {
            let expected = shape
                .iter()
                .try_fold(1usize, |size, dimension| size.checked_mul(*dimension))
                .ok_or_else(|| "tensor shape size overflows usize".to_string())?;
            if values.len() != expected {
                return Err("tensor data length does not match shape".into());
            }
        }
        let mut error = [0; ERROR_CAPACITY];
        let data = values.unwrap_or(&[]);
        let ptr = unsafe {
            coolbox_dl_tensor_new(
                shape.as_ptr(),
                shape.len(),
                data.as_ptr(),
                data.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Ok(Self {
            handle: Handle::new(ptr, coolbox_dl_tensor_free, "tensor", &error)?,
        })
    }
    fn from_ptr(ptr: *mut c_void, error: &[c_char], kind: &str) -> Result<Self, String> {
        Ok(Self {
            handle: Handle::new(ptr, coolbox_dl_tensor_free, kind, error)?,
        })
    }
    pub fn dimensions(&self) -> usize {
        unsafe { coolbox_dl_tensor_dimensions(self.handle.as_ptr()) }
    }
    pub fn len(&self) -> usize {
        unsafe { coolbox_dl_tensor_size(self.handle.as_ptr()) }
    }
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
    pub fn shape(&self) -> Result<Vec<usize>, String> {
        let mut shape = vec![0; self.dimensions()];
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_tensor_shape(
                self.handle.as_ptr(),
                shape.as_mut_ptr(),
                shape.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to read tensor shape")?;
        Ok(shape)
    }
    pub fn data(&self) -> Result<Vec<f64>, String> {
        let mut values = vec![0.0; self.len()];
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_tensor_data(
                self.handle.as_ptr(),
                values.as_mut_ptr(),
                values.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to read tensor data")?;
        Ok(values)
    }
    pub fn clone_tensor(&self) -> Result<Self, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_dl_tensor_clone(self.handle.as_ptr(), error.as_mut_ptr(), error.len())
        };
        Self::from_ptr(ptr, &error, "cloned tensor")
    }
    pub fn reshape(&self, shape: &[usize]) -> Result<Self, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_dl_tensor_reshape(
                self.handle.as_ptr(),
                shape.as_ptr(),
                shape.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Self::from_ptr(ptr, &error, "reshaped tensor")
    }
    pub fn transpose(&self) -> Result<Self, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_dl_tensor_transpose(self.handle.as_ptr(), error.as_mut_ptr(), error.len())
        };
        Self::from_ptr(ptr, &error, "transposed tensor")
    }
    pub fn add(&self, other: &Self) -> Result<Self, String> {
        self.binary(other, 0)
    }
    pub fn subtract(&self, other: &Self) -> Result<Self, String> {
        self.binary(other, 1)
    }
    pub fn multiply(&self, other: &Self) -> Result<Self, String> {
        self.binary(other, 2)
    }
    pub fn divide(&self, other: &Self) -> Result<Self, String> {
        self.binary(other, 3)
    }
    fn binary(&self, other: &Self, op: c_int) -> Result<Self, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_dl_tensor_binary(
                self.handle.as_ptr(),
                other.handle.as_ptr(),
                op,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Self::from_ptr(ptr, &error, "tensor result")
    }
    pub fn scale(&self, scalar: f64) -> Result<Self, String> {
        self.scalar(scalar, 0)
    }
    pub fn divide_scalar(&self, scalar: f64) -> Result<Self, String> {
        self.scalar(scalar, 1)
    }
    fn scalar(&self, scalar: f64, divide: c_int) -> Result<Self, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_dl_tensor_scalar(
                self.handle.as_ptr(),
                scalar,
                divide,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Self::from_ptr(ptr, &error, "tensor result")
    }
    pub fn matmul(&self, other: &Self) -> Result<Self, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_dl_tensor_matmul(
                self.handle.as_ptr(),
                other.handle.as_ptr(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Self::from_ptr(ptr, &error, "matrix product")
    }
    pub fn fill(&mut self, value: f64) -> Result<(), String> {
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_tensor_fill(self.handle.as_ptr(), value, error.as_mut_ptr(), error.len())
        };
        check(ok, &error, "failed to fill tensor")
    }
    pub fn randomize(&mut self, minimum: f64, maximum: f64) -> Result<(), String> {
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_tensor_randomize(
                self.handle.as_ptr(),
                minimum,
                maximum,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to randomize tensor")
    }
    pub fn get(&self, index: usize) -> Result<f64, String> {
        let mut value = 0.0;
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_tensor_at(
                self.handle.as_ptr(),
                index,
                &mut value,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "tensor index out of bounds")?;
        Ok(value)
    }
    pub fn set(&mut self, index: usize, value: f64) -> Result<(), String> {
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_tensor_set(
                self.handle.as_ptr(),
                index,
                value,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "tensor index out of bounds")
    }
}

#[derive(Debug, Clone, Copy)]
pub enum LayerKind {
    Dense {
        input: usize,
        output: usize,
    },
    Relu,
    Sigmoid,
    Tanh,
    Softmax,
    Dropout {
        rate: f64,
    },
    BatchNorm {
        features: usize,
    },
    LayerNorm {
        features: usize,
    },
    Flatten,
    Conv1D {
        input_channels: usize,
        output_channels: usize,
        kernel: usize,
    },
    Conv2D {
        input_channels: usize,
        output_channels: usize,
        kernel_height: usize,
        kernel_width: usize,
        stride: usize,
        padding: usize,
    },
    MaxPool2D {
        kernel_height: usize,
        kernel_width: usize,
        stride_height: usize,
        stride_width: usize,
    },
    AvgPool2D {
        kernel_height: usize,
        kernel_width: usize,
        stride_height: usize,
        stride_width: usize,
    },
    Rnn {
        input: usize,
        hidden: usize,
        return_sequences: bool,
    },
    Lstm {
        input: usize,
        hidden: usize,
        return_sequences: bool,
    },
    Gru {
        input: usize,
        hidden: usize,
        return_sequences: bool,
    },
    Embedding {
        vocabulary: usize,
        dimension: usize,
    },
}

pub struct Layer {
    handle: Handle,
}
impl Layer {
    pub fn new(kind: LayerKind) -> Result<Self, String> {
        let (kind, parameters, rate, sequences) = match kind {
            LayerKind::Dense { input, output } => (0, vec![input, output], 0.0, 0),
            LayerKind::Relu => (1, vec![], 0.0, 0),
            LayerKind::Sigmoid => (2, vec![], 0.0, 0),
            LayerKind::Tanh => (3, vec![], 0.0, 0),
            LayerKind::Softmax => (4, vec![], 0.0, 0),
            LayerKind::Dropout { rate } => (5, vec![], rate, 0),
            LayerKind::BatchNorm { features } => (6, vec![features], 0.0, 0),
            LayerKind::LayerNorm { features } => (7, vec![features], 0.0, 0),
            LayerKind::Flatten => (8, vec![], 0.0, 0),
            LayerKind::Conv1D {
                input_channels,
                output_channels,
                kernel,
            } => (9, vec![input_channels, output_channels, kernel], 0.0, 0),
            LayerKind::Conv2D {
                input_channels,
                output_channels,
                kernel_height,
                kernel_width,
                stride,
                padding,
            } => (
                10,
                vec![
                    input_channels,
                    output_channels,
                    kernel_height,
                    kernel_width,
                    stride,
                    padding,
                ],
                0.0,
                0,
            ),
            LayerKind::MaxPool2D {
                kernel_height,
                kernel_width,
                stride_height,
                stride_width,
            } => (
                11,
                vec![kernel_height, kernel_width, stride_height, stride_width],
                0.0,
                0,
            ),
            LayerKind::AvgPool2D {
                kernel_height,
                kernel_width,
                stride_height,
                stride_width,
            } => (
                12,
                vec![kernel_height, kernel_width, stride_height, stride_width],
                0.0,
                0,
            ),
            LayerKind::Rnn {
                input,
                hidden,
                return_sequences,
            } => (13, vec![input, hidden], 0.0, return_sequences as c_int),
            LayerKind::Lstm {
                input,
                hidden,
                return_sequences,
            } => (14, vec![input, hidden], 0.0, return_sequences as c_int),
            LayerKind::Gru {
                input,
                hidden,
                return_sequences,
            } => (15, vec![input, hidden], 0.0, return_sequences as c_int),
            LayerKind::Embedding {
                vocabulary,
                dimension,
            } => (16, vec![vocabulary, dimension], 0.0, 0),
        };
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_dl_layer_new(
                kind,
                parameters.as_ptr(),
                parameters.len(),
                rate,
                sequences,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Ok(Self {
            handle: Handle::new(ptr, coolbox_dl_layer_free, "layer", &error)?,
        })
    }
    pub fn forward(&mut self, input: &Tensor) -> Result<Tensor, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_dl_layer_forward(
                self.handle.as_ptr(),
                input.handle.as_ptr(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Tensor::from_ptr(ptr, &error, "layer output")
    }
    pub fn backward(&mut self, gradient: &Tensor) -> Result<Tensor, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_dl_layer_backward(
                self.handle.as_ptr(),
                gradient.handle.as_ptr(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Tensor::from_ptr(ptr, &error, "layer gradient")
    }
    pub fn update(&mut self, learning_rate: f64) -> Result<(), String> {
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_layer_update(
                self.handle.as_ptr(),
                learning_rate,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to update layer")
    }
    pub fn set_training(&mut self, training: bool) -> Result<(), String> {
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_layer_set_training(
                self.handle.as_ptr(),
                training as c_int,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "layer does not support training mode")
    }
    pub fn has_parameters(&self) -> bool {
        unsafe { coolbox_dl_layer_has_parameters(self.handle.as_ptr()) != 0 }
    }
    pub fn name(&self) -> Result<String, String> {
        let mut output = [0 as c_char; 256];
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_layer_name(
                self.handle.as_ptr(),
                output.as_mut_ptr(),
                output.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to get layer name")?;
        Ok(unsafe { CStr::from_ptr(output.as_ptr()) }
            .to_string_lossy()
            .into_owned())
    }
}

#[derive(Debug, Clone, Copy)]
pub enum LossKind {
    MeanSquaredError,
    BinaryCrossEntropy,
    CategoricalCrossEntropy,
}
pub struct Loss {
    handle: Handle,
}
impl Loss {
    pub fn new(kind: LossKind) -> Result<Self, String> {
        let kind = match kind {
            LossKind::MeanSquaredError => 0,
            LossKind::BinaryCrossEntropy => 1,
            LossKind::CategoricalCrossEntropy => 2,
        };
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe { coolbox_dl_loss_new(kind, error.as_mut_ptr(), error.len()) };
        Ok(Self {
            handle: Handle::new(ptr, coolbox_dl_loss_free, "loss", &error)?,
        })
    }
    pub fn compute(&self, predictions: &Tensor, targets: &Tensor) -> Result<f64, String> {
        let mut value = 0.0;
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_loss_compute(
                self.handle.as_ptr(),
                predictions.handle.as_ptr(),
                targets.handle.as_ptr(),
                &mut value,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to compute loss")?;
        Ok(value)
    }
    pub fn gradient(&self, predictions: &Tensor, targets: &Tensor) -> Result<Tensor, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_dl_loss_gradient(
                self.handle.as_ptr(),
                predictions.handle.as_ptr(),
                targets.handle.as_ptr(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Tensor::from_ptr(ptr, &error, "loss gradient")
    }
}

#[derive(Debug, Clone, Copy)]
pub enum OptimizerKind {
    Sgd { momentum: f64 },
    Adam { beta1: f64, beta2: f64 },
    Rmsprop { decay: f64 },
}
pub struct Optimizer {
    handle: Handle,
}
impl Optimizer {
    pub fn new(kind: OptimizerKind, learning_rate: f64) -> Result<Self, String> {
        let (kind, p1, p2) = match kind {
            OptimizerKind::Sgd { momentum } => (0, momentum, 0.0),
            OptimizerKind::Adam { beta1, beta2 } => (1, beta1, beta2),
            OptimizerKind::Rmsprop { decay } => (2, decay, 0.0),
        };
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_dl_optimizer_new(kind, learning_rate, p1, p2, error.as_mut_ptr(), error.len())
        };
        Ok(Self {
            handle: Handle::new(ptr, coolbox_dl_optimizer_free, "optimizer", &error)?,
        })
    }
    pub fn step(&mut self, parameters: &mut Tensor, gradients: &Tensor) -> Result<(), String> {
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_optimizer_step(
                self.handle.as_ptr(),
                parameters.handle.as_ptr(),
                gradients.handle.as_ptr(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "optimizer step failed")
    }
    pub fn reset(&mut self) -> Result<(), String> {
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_optimizer_reset(self.handle.as_ptr(), error.as_mut_ptr(), error.len())
        };
        check(ok, &error, "optimizer reset failed")
    }
    pub fn name(&self) -> Result<String, String> {
        let mut output = [0 as c_char; 256];
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_optimizer_name(
                self.handle.as_ptr(),
                output.as_mut_ptr(),
                output.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to get optimizer name")?;
        Ok(unsafe { CStr::from_ptr(output.as_ptr()) }
            .to_string_lossy()
            .into_owned())
    }
}

pub struct NeuralNetwork {
    handle: Handle,
}
impl NeuralNetwork {
    pub fn new() -> Result<Self, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe { coolbox_dl_network_new(error.as_mut_ptr(), error.len()) };
        Ok(Self {
            handle: Handle::new(ptr, coolbox_dl_network_free, "neural network", &error)?,
        })
    }
    pub fn add_layer(&mut self, layer: &Layer) -> Result<(), String> {
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_network_add_layer(
                self.handle.as_ptr(),
                layer.handle.as_ptr(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to add network layer")
    }
    pub fn set_loss(&mut self, loss: &Loss) -> Result<(), String> {
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_network_set_loss(
                self.handle.as_ptr(),
                loss.handle.as_ptr(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to set network loss")
    }
    pub fn set_optimizer(&mut self, optimizer: &Optimizer) -> Result<(), String> {
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_network_set_optimizer(
                self.handle.as_ptr(),
                optimizer.handle.as_ptr(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to set network optimizer")
    }
    pub fn predict(&self, input: &Tensor) -> Result<Tensor, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_dl_network_predict(
                self.handle.as_ptr(),
                input.handle.as_ptr(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Tensor::from_ptr(ptr, &error, "network prediction")
    }
    pub fn train_step(&mut self, input: &Tensor, target: &Tensor) -> Result<f64, String> {
        let mut value = 0.0;
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_network_train_step(
                self.handle.as_ptr(),
                input.handle.as_ptr(),
                target.handle.as_ptr(),
                &mut value,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "network training step failed")?;
        Ok(value)
    }
    pub fn evaluate(&self, inputs: &[Tensor], targets: &[Tensor]) -> Result<f64, String> {
        if inputs.len() != targets.len() || inputs.is_empty() {
            return Err("evaluation needs equal, non-empty input and target batches".into());
        }
        let mut input_ptrs: Vec<_> = inputs.iter().map(|t| t.handle.as_ptr()).collect();
        let mut target_ptrs: Vec<_> = targets.iter().map(|t| t.handle.as_ptr()).collect();
        let mut value = 0.0;
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_network_evaluate(
                self.handle.as_ptr(),
                input_ptrs.as_mut_ptr(),
                target_ptrs.as_mut_ptr(),
                inputs.len(),
                &mut value,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "network evaluation failed")?;
        Ok(value)
    }
    pub fn train(
        &mut self,
        inputs: &[Tensor],
        targets: &[Tensor],
        epochs: i32,
        batch_size: i32,
        verbose: bool,
    ) -> Result<(), String> {
        if inputs.len() != targets.len() || inputs.is_empty() {
            return Err("training needs equal, non-empty input and target batches".into());
        }
        let mut input_ptrs: Vec<_> = inputs.iter().map(|t| t.handle.as_ptr()).collect();
        let mut target_ptrs: Vec<_> = targets.iter().map(|t| t.handle.as_ptr()).collect();
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_network_train(
                self.handle.as_ptr(),
                input_ptrs.as_mut_ptr(),
                target_ptrs.as_mut_ptr(),
                inputs.len(),
                epochs,
                batch_size,
                verbose as c_int,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "network training failed")
    }
    pub fn set_training(&mut self, training: bool) -> Result<(), String> {
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_network_set_training(
                self.handle.as_ptr(),
                training as c_int,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to set network training mode")
    }
    pub fn layer_count(&self) -> usize {
        unsafe { coolbox_dl_network_layer_count(self.handle.as_ptr()) }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FeedbackMode {
    RewardOnly,
    Rich,
}
pub struct Helix {
    handle: Handle,
}
impl Helix {
    pub fn new(actions: usize, truth: usize, feedback_mode: FeedbackMode) -> Result<Self, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr = unsafe {
            coolbox_dl_helix_new(
                actions,
                truth,
                (feedback_mode == FeedbackMode::Rich) as c_int,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        Ok(Self {
            handle: Handle::new(ptr, coolbox_dl_helix_free, "HELiX learner", &error)?,
        })
    }
    pub fn select_action(&mut self) -> Result<usize, String> {
        let mut action = 0;
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_helix_select_action(
                self.handle.as_ptr(),
                &mut action,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to select action")?;
        Ok(action)
    }
    pub fn observe(&mut self, action: usize, feedback: &str) -> Result<(), String> {
        let feedback =
            CString::new(feedback).map_err(|_| "feedback cannot contain a NUL byte".to_string())?;
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_helix_observe(
                self.handle.as_ptr(),
                action,
                feedback.as_ptr(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to observe feedback")
    }
    pub fn interact(&mut self) -> Result<usize, String> {
        let mut action = 0;
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_dl_helix_interact(
                self.handle.as_ptr(),
                &mut action,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to interact")?;
        Ok(action)
    }
    pub fn rounds(&self) -> usize {
        unsafe { coolbox_dl_helix_rounds(self.handle.as_ptr()) }
    }
    pub fn mistakes(&self) -> usize {
        unsafe { coolbox_dl_helix_mistakes(self.handle.as_ptr()) }
    }
    pub fn cumulative_regret(&self) -> f64 {
        unsafe { coolbox_dl_helix_cumulative_regret(self.handle.as_ptr()) }
    }
    pub fn identified(&self) -> bool {
        unsafe { coolbox_dl_helix_identified(self.handle.as_ptr()) != 0 }
    }
    pub fn version_space(&self) -> Vec<usize> {
        let count = unsafe {
            coolbox_dl_helix_version_space(self.handle.as_ptr(), std::ptr::null_mut(), 0)
        };
        let mut indices = vec![0; count];
        if count != 0 {
            unsafe {
                coolbox_dl_helix_version_space(self.handle.as_ptr(), indices.as_mut_ptr(), count);
            }
        }
        indices
    }
}
