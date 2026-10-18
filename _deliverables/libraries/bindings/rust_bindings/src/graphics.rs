use std::ffi::{c_char, c_int, c_void, CStr, CString};
use std::marker::PhantomData;
use std::ptr::NonNull;

const ERROR_CAPACITY: usize = 1024;

unsafe extern "C" {
    fn coolbox_graph_new(
        width: c_int,
        height: c_int,
        kind: c_int,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_graph_free(graph: *mut c_void);
    fn coolbox_graph_set_title(
        graph: *mut c_void,
        title: *const c_char,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_graph_set_x_label(
        graph: *mut c_void,
        label: *const c_char,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_graph_set_y_label(
        graph: *mut c_void,
        label: *const c_char,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_graph_add_series(
        graph: *mut c_void,
        label: *const c_char,
        x_values: *const f64,
        y_values: *const f64,
        count: usize,
        red: u8,
        green: u8,
        blue: u8,
        alpha: u8,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_graph_render(
        graph: *mut c_void,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_plot_2d(
        x_values: *const f64,
        y_values: *const f64,
        count: usize,
        title: *const c_char,
        x_label: *const c_char,
        y_label: *const c_char,
        series_label: *const c_char,
        red: u8,
        green: u8,
        blue: u8,
        alpha: u8,
        width: c_int,
        height: c_int,
        kind: c_int,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_plot_3d(
        x_values: *const f64,
        y_values: *const f64,
        z_values: *const f64,
        count: usize,
        title: *const c_char,
        x_label: *const c_char,
        y_label: *const c_char,
        z_label: *const c_char,
        series_label: *const c_char,
        red: u8,
        green: u8,
        blue: u8,
        alpha: u8,
        width: c_int,
        height: c_int,
        kind: c_int,
        azimuth_degrees: f64,
        elevation_degrees: f64,
        error: *mut c_char,
        error_capacity: usize,
    ) -> *mut c_void;
    fn coolbox_canvas_show_window(
        canvas: *mut c_void,
        title: *const c_char,
        save_path: *const c_char,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_canvas_free(canvas: *mut c_void);
    fn coolbox_canvas_width(canvas: *mut c_void) -> c_int;
    fn coolbox_canvas_height(canvas: *mut c_void) -> c_int;
    fn coolbox_canvas_data_size(canvas: *mut c_void) -> usize;
    fn coolbox_canvas_copy_data(
        canvas: *mut c_void,
        output: *mut u8,
        capacity: usize,
        error: *mut c_char,
        error_capacity: usize,
    ) -> c_int;
    fn coolbox_canvas_save(
        canvas: *mut c_void,
        path: *const c_char,
        format: c_int,
        quality: c_int,
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
        error: &[c_char],
    ) -> Result<Self, String> {
        Ok(Self {
            ptr: NonNull::new(ptr)
                .ok_or_else(|| native_error(error, &format!("failed to create {kind}")))?,
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

fn native_error(buffer: &[c_char], fallback: &str) -> String {
    let message = unsafe { CStr::from_ptr(buffer.as_ptr()) }.to_string_lossy();
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
        Err(native_error(error, fallback))
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GraphType {
    Line,
    Bar,
    Scatter,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Graph3DType {
    Line,
    Scatter,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Color {
    pub red: u8,
    pub green: u8,
    pub blue: u8,
    pub alpha: u8,
}

impl Default for Color {
    fn default() -> Self {
        Self {
            red: 31,
            green: 119,
            blue: 180,
            alpha: 255,
        }
    }
}

#[derive(Debug, Clone)]
pub struct Plot2DOptions {
    pub width: i32,
    pub height: i32,
    pub title: String,
    pub x_label: String,
    pub y_label: String,
    pub series_label: String,
    pub color: Color,
    pub graph_type: GraphType,
    pub show_window: bool,
    pub save_path: String,
}

impl Default for Plot2DOptions {
    fn default() -> Self {
        Self {
            width: 800,
            height: 600,
            title: String::new(),
            x_label: "X".to_string(),
            y_label: "Y".to_string(),
            series_label: "Series".to_string(),
            color: Color::default(),
            graph_type: GraphType::Line,
            show_window: false,
            save_path: "plot.png".to_string(),
        }
    }
}

#[derive(Debug, Clone)]
pub struct Plot3DOptions {
    pub width: i32,
    pub height: i32,
    pub title: String,
    pub x_label: String,
    pub y_label: String,
    pub z_label: String,
    pub series_label: String,
    pub color: Color,
    pub graph_type: Graph3DType,
    pub azimuth_degrees: f64,
    pub elevation_degrees: f64,
    pub show_window: bool,
    pub save_path: String,
}

impl Default for Plot3DOptions {
    fn default() -> Self {
        Self {
            width: 800,
            height: 600,
            title: String::new(),
            x_label: "X".to_string(),
            y_label: "Y".to_string(),
            z_label: "Z".to_string(),
            series_label: "Series".to_string(),
            color: Color::default(),
            graph_type: Graph3DType::Scatter,
            azimuth_degrees: 45.0,
            elevation_degrees: 25.0,
            show_window: false,
            save_path: "plot.png".to_string(),
        }
    }
}

pub struct Graph {
    handle: Handle,
}

impl Graph {
    pub fn new(width: i32, height: i32, kind: GraphType) -> Result<Self, String> {
        let kind = match kind {
            GraphType::Line => 0,
            GraphType::Bar => 1,
            GraphType::Scatter => 2,
        };
        let mut error = [0; ERROR_CAPACITY];
        let ptr =
            unsafe { coolbox_graph_new(width, height, kind, error.as_mut_ptr(), error.len()) };
        Ok(Self {
            handle: Handle::new(ptr, coolbox_graph_free, "graph", &error)?,
        })
    }
}

pub fn plot_2d(x: &[f64], y: &[f64], options: Plot2DOptions) -> Result<Canvas, String> {
    if x.is_empty() || x.len() != y.len() {
        return Err("plot x and y values must have equal, non-empty lengths".to_string());
    }
    let title = CString::new(options.title.as_str())
        .map_err(|_| "plot title cannot contain a NUL byte".to_string())?;
    let x_label = CString::new(options.x_label.as_str())
        .map_err(|_| "x label cannot contain a NUL byte".to_string())?;
    let y_label = CString::new(options.y_label.as_str())
        .map_err(|_| "y label cannot contain a NUL byte".to_string())?;
    let series_label = CString::new(options.series_label.as_str())
        .map_err(|_| "series label cannot contain a NUL byte".to_string())?;
    let kind = match options.graph_type {
        GraphType::Line => 0,
        GraphType::Bar => 1,
        GraphType::Scatter => 2,
    };
    let mut error = [0; ERROR_CAPACITY];
    let ptr = unsafe {
        coolbox_plot_2d(
            x.as_ptr(),
            y.as_ptr(),
            x.len(),
            title.as_ptr(),
            x_label.as_ptr(),
            y_label.as_ptr(),
            series_label.as_ptr(),
            options.color.red,
            options.color.green,
            options.color.blue,
            options.color.alpha,
            options.width,
            options.height,
            kind,
            error.as_mut_ptr(),
            error.len(),
        )
    };
    let canvas = Canvas {
        handle: Handle::new(ptr, coolbox_canvas_free, "canvas", &error)?,
    };
    if options.show_window {
        canvas.show_window(&options.title, &options.save_path)?;
    }
    Ok(canvas)
}

pub fn plot_3d(x: &[f64], y: &[f64], z: &[f64], options: Plot3DOptions) -> Result<Canvas, String> {
    if x.is_empty() || x.len() != y.len() || x.len() != z.len() {
        return Err("plot x, y, and z values must have equal, non-empty lengths".to_string());
    }
    if !options.azimuth_degrees.is_finite() || !options.elevation_degrees.is_finite() {
        return Err("3D view angles must be finite".to_string());
    }
    let title = CString::new(options.title.as_str())
        .map_err(|_| "plot title cannot contain a NUL byte".to_string())?;
    let x_label = CString::new(options.x_label.as_str())
        .map_err(|_| "x label cannot contain a NUL byte".to_string())?;
    let y_label = CString::new(options.y_label.as_str())
        .map_err(|_| "y label cannot contain a NUL byte".to_string())?;
    let z_label = CString::new(options.z_label.as_str())
        .map_err(|_| "z label cannot contain a NUL byte".to_string())?;
    let series_label = CString::new(options.series_label.as_str())
        .map_err(|_| "series label cannot contain a NUL byte".to_string())?;
    let kind = match options.graph_type {
        Graph3DType::Line => 0,
        Graph3DType::Scatter => 1,
    };
    let mut error = [0; ERROR_CAPACITY];
    let ptr = unsafe {
        coolbox_plot_3d(
            x.as_ptr(),
            y.as_ptr(),
            z.as_ptr(),
            x.len(),
            title.as_ptr(),
            x_label.as_ptr(),
            y_label.as_ptr(),
            z_label.as_ptr(),
            series_label.as_ptr(),
            options.color.red,
            options.color.green,
            options.color.blue,
            options.color.alpha,
            options.width,
            options.height,
            kind,
            options.azimuth_degrees,
            options.elevation_degrees,
            error.as_mut_ptr(),
            error.len(),
        )
    };
    let canvas = Canvas {
        handle: Handle::new(ptr, coolbox_canvas_free, "canvas", &error)?,
    };
    if options.show_window {
        canvas.show_window(&options.title, &options.save_path)?;
    }
    Ok(canvas)
}

impl Graph {
    fn set_text(
        &mut self,
        text: &str,
        operation: unsafe extern "C" fn(*mut c_void, *const c_char, *mut c_char, usize) -> c_int,
        what: &str,
    ) -> Result<(), String> {
        let text = CString::new(text).map_err(|_| format!("{what} cannot contain a NUL byte"))?;
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            operation(
                self.handle.as_ptr(),
                text.as_ptr(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, &format!("failed to set {what}"))
    }

    pub fn set_title(&mut self, title: &str) -> Result<(), String> {
        self.set_text(title, coolbox_graph_set_title, "title")
    }
    pub fn set_x_label(&mut self, label: &str) -> Result<(), String> {
        self.set_text(label, coolbox_graph_set_x_label, "x label")
    }
    pub fn set_y_label(&mut self, label: &str) -> Result<(), String> {
        self.set_text(label, coolbox_graph_set_y_label, "y label")
    }

    pub fn add_series(
        &mut self,
        label: &str,
        x: &[f64],
        y: &[f64],
        color: Color,
    ) -> Result<(), String> {
        if x.len() != y.len() {
            return Err("x and y series lengths must match".to_string());
        }
        let label = CString::new(label)
            .map_err(|_| "series label cannot contain a NUL byte".to_string())?;
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_graph_add_series(
                self.handle.as_ptr(),
                label.as_ptr(),
                x.as_ptr(),
                y.as_ptr(),
                x.len(),
                color.red,
                color.green,
                color.blue,
                color.alpha,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to add graph series")
    }

    pub fn render(&self) -> Result<Canvas, String> {
        let mut error = [0; ERROR_CAPACITY];
        let ptr =
            unsafe { coolbox_graph_render(self.handle.as_ptr(), error.as_mut_ptr(), error.len()) };
        Ok(Canvas {
            handle: Handle::new(ptr, coolbox_canvas_free, "canvas", &error)?,
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ImageFormat {
    Bmp,
    Png,
    Jpeg,
}

pub struct Canvas {
    handle: Handle,
}

impl Canvas {
    /// Requests the native viewer; the current Rust bridge reports it unavailable.
    pub fn show_window(&self, title: &str, save_path: &str) -> Result<(), String> {
        let title = CString::new(title)
            .map_err(|_| "window title cannot contain a NUL byte".to_string())?;
        let save_path = CString::new(save_path)
            .map_err(|_| "save path cannot contain a NUL byte".to_string())?;
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_canvas_show_window(
                self.handle.as_ptr(),
                title.as_ptr(),
                save_path.as_ptr(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to open plot window")
    }

    pub fn width(&self) -> u32 {
        unsafe { coolbox_canvas_width(self.handle.as_ptr()).max(0) as u32 }
    }
    pub fn height(&self) -> u32 {
        unsafe { coolbox_canvas_height(self.handle.as_ptr()).max(0) as u32 }
    }
    pub fn rgba_bytes(&self) -> Result<Vec<u8>, String> {
        let size = unsafe { coolbox_canvas_data_size(self.handle.as_ptr()) };
        let mut output = vec![0; size];
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_canvas_copy_data(
                self.handle.as_ptr(),
                output.as_mut_ptr(),
                output.len(),
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to copy canvas pixels")?;
        Ok(output)
    }
    pub fn save(&self, path: &str, format: ImageFormat, quality: u8) -> Result<(), String> {
        let path =
            CString::new(path).map_err(|_| "image path cannot contain a NUL byte".to_string())?;
        let (format, quality) = match format {
            ImageFormat::Bmp => (0, 0),
            ImageFormat::Png => (1, 0),
            ImageFormat::Jpeg => (2, quality as c_int),
        };
        let mut error = [0; ERROR_CAPACITY];
        let ok = unsafe {
            coolbox_canvas_save(
                self.handle.as_ptr(),
                path.as_ptr(),
                format,
                quality,
                error.as_mut_ptr(),
                error.len(),
            )
        };
        check(ok, &error, "failed to save canvas")
    }
}
