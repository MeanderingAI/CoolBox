use std::collections::BTreeMap;
use std::ffi::{c_char, c_int, c_void, CStr, CString};
use std::ptr;
use std::sync::Mutex;

#[repr(C)]
struct CoolBoxSqlDatabase {
    _private: [u8; 0],
}
#[repr(C)]
struct CoolBoxSqlSchema {
    _private: [u8; 0],
}
#[repr(C)]
struct CoolBoxSqlRepository {
    _private: [u8; 0],
}
#[repr(C)]
struct CoolBoxRouteServer {
    _private: [u8; 0],
}

type RouteHandler = unsafe extern "C" fn(
    *mut c_void,
    *const c_char,
    *const c_char,
    *const c_char,
    *const c_char,
    *const *const c_char,
    *const *const c_char,
    usize,
    *const *const c_char,
    *const *const c_char,
    usize,
    *mut c_int,
    *mut *mut c_char,
) -> *mut c_char;

#[link(name = "coolbox_c_bindings")]
extern "C" {
    fn coolbox_c_free_string(value: *mut c_char);
    fn coolbox_c_duplicate_string(value: *const c_char) -> *mut c_char;

    fn coolbox_sql_database_create(
        provider: *const c_char,
        error: *mut c_char,
        capacity: usize,
    ) -> *mut CoolBoxSqlDatabase;
    fn coolbox_sql_database_destroy(database: *mut CoolBoxSqlDatabase);
    fn coolbox_sql_database_connect(
        database: *mut CoolBoxSqlDatabase,
        connection_string: *const c_char,
        error: *mut c_char,
        capacity: usize,
    ) -> c_int;
    fn coolbox_sql_database_disconnect(database: *mut CoolBoxSqlDatabase);
    fn coolbox_sql_database_execute(
        database: *mut CoolBoxSqlDatabase,
        query: *const c_char,
        params: *const *const c_char,
        param_count: usize,
        error: *mut c_char,
        capacity: usize,
    ) -> *mut c_char;
    fn coolbox_sql_database_begin(database: *mut CoolBoxSqlDatabase) -> c_int;
    fn coolbox_sql_database_commit(database: *mut CoolBoxSqlDatabase) -> c_int;
    fn coolbox_sql_database_rollback(database: *mut CoolBoxSqlDatabase) -> c_int;

    fn coolbox_sql_schema_parse(
        schema_text: *const c_char,
        error: *mut c_char,
        capacity: usize,
    ) -> *mut CoolBoxSqlSchema;
    fn coolbox_sql_schema_destroy(schema: *mut CoolBoxSqlSchema);
    fn coolbox_sql_schema_model_count(schema: *const CoolBoxSqlSchema) -> usize;
    fn coolbox_sql_schema_model_name(schema: *const CoolBoxSqlSchema, index: usize) -> *mut c_char;
    fn coolbox_sql_migrate_up(
        database: *mut CoolBoxSqlDatabase,
        schema: *const CoolBoxSqlSchema,
        error: *mut c_char,
        capacity: usize,
    ) -> c_int;
    fn coolbox_sql_migrations_up(
        database: *mut CoolBoxSqlDatabase,
        directory: *const c_char,
        error: *mut c_char,
        capacity: usize,
    ) -> c_int;
    fn coolbox_sql_migrations_down(
        database: *mut CoolBoxSqlDatabase,
        directory: *const c_char,
        steps: usize,
        error: *mut c_char,
        capacity: usize,
    ) -> c_int;
    fn coolbox_sql_migrations_current_version(
        database: *mut CoolBoxSqlDatabase,
        directory: *const c_char,
        error: *mut c_char,
        capacity: usize,
    ) -> *mut c_char;
    fn coolbox_sql_repository_create(
        database: *mut CoolBoxSqlDatabase,
        schema: *const CoolBoxSqlSchema,
        model_name: *const c_char,
        error: *mut c_char,
        capacity: usize,
    ) -> *mut CoolBoxSqlRepository;
    fn coolbox_sql_repository_destroy(repository: *mut CoolBoxSqlRepository);
    fn coolbox_sql_repository_call(
        repository: *mut CoolBoxSqlRepository,
        operation: *const c_char,
        id: *const c_char,
        keys: *const *const c_char,
        values: *const *const c_char,
        count: usize,
        condition_keys: *const *const c_char,
        condition_values: *const *const c_char,
        condition_count: usize,
        error: *mut c_char,
        capacity: usize,
    ) -> *mut c_char;

    fn coolbox_http_server_create(
        port: c_int,
        error: *mut c_char,
        capacity: usize,
    ) -> *mut CoolBoxRouteServer;
    fn coolbox_http_server_add_route_v2(
        server: *mut CoolBoxRouteServer,
        method: *const c_char,
        path: *const c_char,
        handler: Option<RouteHandler>,
        context: *mut c_void,
        error: *mut c_char,
        capacity: usize,
    ) -> c_int;
    fn coolbox_http_server_register_handler_v2(
        server: *mut CoolBoxRouteServer,
        name: *const c_char,
        handler: Option<RouteHandler>,
        context: *mut c_void,
        error: *mut c_char,
        capacity: usize,
    ) -> c_int;
    fn coolbox_http_server_load_routes(
        server: *mut CoolBoxRouteServer,
        routes_file: *const c_char,
        error: *mut c_char,
        capacity: usize,
    ) -> c_int;
    fn coolbox_http_server_start(
        server: *mut CoolBoxRouteServer,
        error: *mut c_char,
        capacity: usize,
    ) -> c_int;
    fn coolbox_http_server_stop(server: *mut CoolBoxRouteServer);
    fn coolbox_http_server_is_running(server: *const CoolBoxRouteServer) -> c_int;
    fn coolbox_http_server_destroy(server: *mut CoolBoxRouteServer);
    fn coolbox_http_form_load(filename: *const c_char, error: *mut c_char, capacity: usize) -> *mut c_char;
    fn coolbox_http_form_validate(
        schema: *const c_char, body: *const c_char, content_type: *const c_char,
        error: *mut c_char, capacity: usize,
    ) -> *mut c_char;
    fn coolbox_http_html_file(
        root: *const c_char, filename: *const c_char, context: *const c_char,
        render: c_int, error: *mut c_char, capacity: usize,
    ) -> *mut c_char;
}

const ERROR_CAPACITY: usize = 1024;

fn error_buffer() -> [c_char; ERROR_CAPACITY] {
    [0; ERROR_CAPACITY]
}

fn native_error(buffer: &[c_char]) -> String {
    unsafe { CStr::from_ptr(buffer.as_ptr()) }
        .to_string_lossy()
        .into_owned()
}

fn c_string(value: &str) -> Result<CString, String> {
    CString::new(value).map_err(|_| "string contains an interior NUL byte".to_string())
}

unsafe fn take_native_string(value: *mut c_char) -> Result<String, String> {
    if value.is_null() {
        return Err("native operation returned no result".to_string());
    }
    let output = CStr::from_ptr(value).to_string_lossy().into_owned();
    coolbox_c_free_string(value);
    Ok(output)
}

pub struct SqlDatabase {
    native: *mut CoolBoxSqlDatabase,
}

impl SqlDatabase {
    pub fn new(provider: &str) -> Result<Self, String> {
        let provider = c_string(provider)?;
        let mut error = error_buffer();
        let native = unsafe {
            coolbox_sql_database_create(provider.as_ptr(), error.as_mut_ptr(), ERROR_CAPACITY)
        };
        if native.is_null() {
            return Err(native_error(&error));
        }
        Ok(Self { native })
    }

    pub fn connect(&self, connection_string: &str) -> Result<(), String> {
        let connection_string = c_string(connection_string)?;
        let mut error = error_buffer();
        let success = unsafe {
            coolbox_sql_database_connect(
                self.native,
                connection_string.as_ptr(),
                error.as_mut_ptr(),
                ERROR_CAPACITY,
            )
        };
        if success == 0 {
            Err(native_error(&error))
        } else {
            Ok(())
        }
    }

    pub fn disconnect(&self) {
        unsafe { coolbox_sql_database_disconnect(self.native) }
    }

    pub fn execute(&self, query: &str, params: &[&str]) -> Result<String, String> {
        let query = c_string(query)?;
        let parameters = params
            .iter()
            .map(|value| c_string(value))
            .collect::<Result<Vec<_>, _>>()?;
        let pointers = parameters.iter().map(|value| value.as_ptr()).collect::<Vec<_>>();
        let mut error = error_buffer();
        let output = unsafe {
            coolbox_sql_database_execute(
                self.native,
                query.as_ptr(),
                pointers.as_ptr(),
                pointers.len(),
                error.as_mut_ptr(),
                ERROR_CAPACITY,
            )
        };
        if output.is_null() {
            Err(native_error(&error))
        } else {
            unsafe { take_native_string(output) }
        }
    }

    pub fn begin(&self) -> Result<(), String> {
        transaction_result(unsafe { coolbox_sql_database_begin(self.native) })
    }
    pub fn commit(&self) -> Result<(), String> {
        transaction_result(unsafe { coolbox_sql_database_commit(self.native) })
    }
    pub fn rollback(&self) -> Result<(), String> {
        transaction_result(unsafe { coolbox_sql_database_rollback(self.native) })
    }

    pub fn migrate_up(&self, schema: &SqlSchema) -> Result<(), String> {
        let mut error = error_buffer();
        let success = unsafe {
            coolbox_sql_migrate_up(self.native, schema.native, error.as_mut_ptr(), ERROR_CAPACITY)
        };
        if success == 0 {
            Err(native_error(&error))
        } else {
            Ok(())
        }
    }

    pub fn migrate_directory_up(&self, directory: &str) -> Result<(), String> {
        self.migrate_directory(directory, 0, true)
    }

    pub fn migrate_directory_down(&self, directory: &str, steps: usize) -> Result<(), String> {
        self.migrate_directory(directory, steps, false)
    }

    fn migrate_directory(&self, directory: &str, steps: usize, up: bool) -> Result<(), String> {
        let directory = c_string(directory)?;
        let mut error = error_buffer();
        let success = unsafe {
            if up {
                coolbox_sql_migrations_up(
                    self.native,
                    directory.as_ptr(),
                    error.as_mut_ptr(),
                    ERROR_CAPACITY,
                )
            } else {
                coolbox_sql_migrations_down(
                    self.native,
                    directory.as_ptr(),
                    steps,
                    error.as_mut_ptr(),
                    ERROR_CAPACITY,
                )
            }
        };
        if success == 0 {
            Err(native_error(&error))
        } else {
            Ok(())
        }
    }

    pub fn current_migration(&self, directory: &str) -> Result<String, String> {
        let directory = c_string(directory)?;
        let mut error = error_buffer();
        let version = unsafe {
            coolbox_sql_migrations_current_version(
                self.native,
                directory.as_ptr(),
                error.as_mut_ptr(),
                ERROR_CAPACITY,
            )
        };
        if version.is_null() {
            Err(native_error(&error))
        } else {
            unsafe { take_native_string(version) }
        }
    }

    pub fn repository(
        self: &std::sync::Arc<Self>,
        schema: &SqlSchema,
        model: &str,
    ) -> Result<SqlRepository, String> {
        let model = c_string(model)?;
        let mut error = error_buffer();
        let native = unsafe {
            coolbox_sql_repository_create(
                self.native,
                schema.native,
                model.as_ptr(),
                error.as_mut_ptr(),
                ERROR_CAPACITY,
            )
        };
        if native.is_null() {
            return Err(native_error(&error));
        }
        Ok(SqlRepository {
            native,
            _database: self.clone(),
        })
    }
}

impl Drop for SqlDatabase {
    fn drop(&mut self) {
        unsafe { coolbox_sql_database_destroy(self.native) }
    }
}

fn transaction_result(success: c_int) -> Result<(), String> {
    if success == 0 {
        Err("database transaction operation failed".to_string())
    } else {
        Ok(())
    }
}

pub struct SqlSchema {
    native: *mut CoolBoxSqlSchema,
}

impl SqlSchema {
    pub fn parse(schema_text: &str) -> Result<Self, String> {
        let schema_text = c_string(schema_text)?;
        let mut error = error_buffer();
        let native = unsafe {
            coolbox_sql_schema_parse(schema_text.as_ptr(), error.as_mut_ptr(), ERROR_CAPACITY)
        };
        if native.is_null() {
            return Err(native_error(&error));
        }
        Ok(Self { native })
    }

    pub fn models(&self) -> Result<Vec<String>, String> {
        let count = unsafe { coolbox_sql_schema_model_count(self.native) };
        let mut models = Vec::with_capacity(count);
        for index in 0..count {
            let name = unsafe { coolbox_sql_schema_model_name(self.native, index) };
            models.push(unsafe { take_native_string(name)? });
        }
        Ok(models)
    }
}

impl Drop for SqlSchema {
    fn drop(&mut self) {
        unsafe { coolbox_sql_schema_destroy(self.native) }
    }
}

pub struct SqlRepository {
    native: *mut CoolBoxSqlRepository,
    _database: std::sync::Arc<SqlDatabase>,
}

impl SqlRepository {
    fn call(
        &self,
        operation: &str,
        id: Option<&str>,
        data: &BTreeMap<String, String>,
        conditions: &BTreeMap<String, String>,
    ) -> Result<String, String> {
        let operation = c_string(operation)?;
        let id = id.map(c_string).transpose()?;
        let data_keys = data.keys().map(|key| c_string(key)).collect::<Result<Vec<_>, _>>()?;
        let data_values = data.values().map(|value| c_string(value)).collect::<Result<Vec<_>, _>>()?;
        let condition_keys = conditions
            .keys()
            .map(|key| c_string(key))
            .collect::<Result<Vec<_>, _>>()?;
        let condition_values = conditions
            .values()
            .map(|value| c_string(value))
            .collect::<Result<Vec<_>, _>>()?;
        let data_key_ptrs = data_keys.iter().map(|value| value.as_ptr()).collect::<Vec<_>>();
        let data_value_ptrs = data_values.iter().map(|value| value.as_ptr()).collect::<Vec<_>>();
        let condition_key_ptrs = condition_keys.iter().map(|value| value.as_ptr()).collect::<Vec<_>>();
        let condition_value_ptrs = condition_values.iter().map(|value| value.as_ptr()).collect::<Vec<_>>();
        let mut error = error_buffer();
        let output = unsafe {
            coolbox_sql_repository_call(
                self.native,
                operation.as_ptr(),
                id.as_ref().map_or(ptr::null(), |value| value.as_ptr()),
                data_key_ptrs.as_ptr(),
                data_value_ptrs.as_ptr(),
                data_key_ptrs.len(),
                condition_key_ptrs.as_ptr(),
                condition_value_ptrs.as_ptr(),
                condition_key_ptrs.len(),
                error.as_mut_ptr(),
                ERROR_CAPACITY,
            )
        };
        if output.is_null() {
            Err(native_error(&error))
        } else {
            unsafe { take_native_string(output) }
        }
    }

    pub fn find_all(&self) -> Result<String, String> {
        self.call("find_all", None, &BTreeMap::new(), &BTreeMap::new())
    }
    pub fn find_by_id(&self, id: &str) -> Result<String, String> {
        self.call("find_by_id", Some(id), &BTreeMap::new(), &BTreeMap::new())
    }
    pub fn find_where(&self, conditions: &BTreeMap<String, String>) -> Result<String, String> {
        self.call("find_where", None, &BTreeMap::new(), conditions)
    }
    pub fn create(&self, data: &BTreeMap<String, String>) -> Result<String, String> {
        self.call("create", None, data, &BTreeMap::new())
    }
    pub fn update(&self, id: &str, data: &BTreeMap<String, String>) -> Result<String, String> {
        self.call("update", Some(id), data, &BTreeMap::new())
    }
    pub fn update_where(
        &self,
        conditions: &BTreeMap<String, String>,
        data: &BTreeMap<String, String>,
    ) -> Result<String, String> {
        self.call("update_where", None, data, conditions)
    }
    pub fn remove(&self, id: &str) -> Result<String, String> {
        self.call("remove", Some(id), &BTreeMap::new(), &BTreeMap::new())
    }
    pub fn remove_where(&self, conditions: &BTreeMap<String, String>) -> Result<String, String> {
        self.call("remove_where", None, &BTreeMap::new(), conditions)
    }
    pub fn create_table(&self) -> Result<String, String> {
        self.call("create_table", None, &BTreeMap::new(), &BTreeMap::new())
    }
    pub fn drop_table(&self) -> Result<String, String> {
        self.call("drop_table", None, &BTreeMap::new(), &BTreeMap::new())
    }
}

impl Drop for SqlRepository {
    fn drop(&mut self) {
        unsafe { coolbox_sql_repository_destroy(self.native) }
    }
}

unsafe extern "C" fn route_handler(
    context: *mut c_void,
    method: *const c_char,
    path: *const c_char,
    body: *const c_char,
    headers_json: *const c_char,
    path_keys: *const *const c_char,
    path_values: *const *const c_char,
    path_count: usize,
    query_keys: *const *const c_char,
    query_values: *const *const c_char,
    query_count: usize,
    status: *mut c_int,
    response_headers: *mut *mut c_char,
) -> *mut c_char {
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| unsafe {
        let handler = &*(context as *const Box<dyn Fn(HttpRequest) -> HttpResponse + Send + Sync>);
        handler(HttpRequest {
            method: CStr::from_ptr(method).to_string_lossy().into_owned(),
            path: CStr::from_ptr(path).to_string_lossy().into_owned(),
            body: CStr::from_ptr(body).to_string_lossy().into_owned(),
            path_params: native_string_map(path_keys, path_values, path_count),
            query_params: native_string_map(query_keys, query_values, query_count),
            headers: serde_json::from_str(
                &CStr::from_ptr(headers_json).to_string_lossy()
            ).expect("native request headers must be a JSON string map"),
        })
    }));
    match result {
        Ok(response) => unsafe {
            *status = response.status_code;
            if response.status_code < 100 || response.status_code > 599 {
                *status = 500;
                return coolbox_c_duplicate_string(b"invalid handler status code\0".as_ptr().cast());
            }
            let body = match c_string(&response.body) {
                Ok(body) => body,
                Err(error) => {
                    eprintln!("coolbox: {error}");
                    *status = 500;
                    return coolbox_c_duplicate_string(b"Internal Server Error\0".as_ptr().cast());
                }
            };
            let headers = serde_json::to_string(&response.headers).expect("string map serialization");
            let headers = CString::new(headers).expect("JSON serialization escapes NUL");
            *response_headers = coolbox_c_duplicate_string(headers.as_ptr());
            coolbox_c_duplicate_string(body.as_ptr())
        },
        Err(_) => unsafe {
            eprintln!("coolbox: HTTP handler panicked");
            *status = 500;
            coolbox_c_duplicate_string(b"Internal Server Error\0".as_ptr().cast())
        },
    }
}

#[derive(Clone, Debug)]
pub struct HttpRequest {
    pub method: String,
    pub path: String,
    pub body: String,
    pub path_params: BTreeMap<String, String>,
    pub query_params: BTreeMap<String, String>,
    pub headers: BTreeMap<String, String>,
}

unsafe fn native_string_map(
    keys: *const *const c_char,
    values: *const *const c_char,
    count: usize,
) -> BTreeMap<String, String> {
    if count == 0 {
        return BTreeMap::new();
    }
    let keys = std::slice::from_raw_parts(keys, count);
    let values = std::slice::from_raw_parts(values, count);
    keys.iter()
        .zip(values)
        .map(|(key, value)| (
            CStr::from_ptr(*key).to_string_lossy().into_owned(),
            CStr::from_ptr(*value).to_string_lossy().into_owned(),
        ))
        .collect()
}

#[derive(Clone, Debug)]
pub struct HttpResponse {
    pub status_code: i32,
    pub body: String,
    pub headers: BTreeMap<String, String>,
}

impl HttpResponse {
    pub fn new(status_code: i32, body: impl Into<String>) -> Self {
        Self { status_code, body: body.into(), headers: BTreeMap::new() }
    }

    pub fn html(body: impl Into<String>) -> Self {
        let mut response = Self::new(200, body);
        response.headers.insert("Content-Type".into(), "text/html; charset=utf-8".into());
        response
    }

    pub fn html_file(root: &str, filename: &str) -> Result<Self, String> {
        Self::file_response(root, filename, &BTreeMap::new(), false)
    }

    pub fn render(root: &str, filename: &str, context: &BTreeMap<String, String>) -> Result<Self, String> {
        Self::file_response(root, filename, context, true)
    }

    fn file_response(root: &str, filename: &str, context: &BTreeMap<String, String>, render: bool) -> Result<Self, String> {
        let root = c_string(root)?;
        let filename = c_string(filename)?;
        let context = c_string(&serde_json::to_string(context).map_err(|e| e.to_string())?)?;
        let mut error = error_buffer();
        let output = unsafe {
            coolbox_http_html_file(root.as_ptr(), filename.as_ptr(), context.as_ptr(),
                i32::from(render), error.as_mut_ptr(), ERROR_CAPACITY)
        };
        if output.is_null() { return Err(native_error(&error)); }
        let text = unsafe { take_native_string(output)? };
        let envelope: serde_json::Value = serde_json::from_str(&text).map_err(|e| e.to_string())?;
        let status_code = envelope["status_code"].as_i64().ok_or("Missing response status")?;
        let body = envelope["body"].as_str().ok_or("Missing response body")?.to_string();
        let headers = serde_json::from_value(envelope["headers"].clone()).map_err(|e| e.to_string())?;
        Ok(Self { status_code: i32::try_from(status_code).map_err(|e| e.to_string())?, body, headers })
    }
}

pub struct FormSchema {
    definition: String,
}

pub struct ValidationResult {
    pub valid: bool,
    pub values: serde_json::Value,
    pub errors: BTreeMap<String, Vec<String>>,
}

impl FormSchema {
    pub fn parse(definition: &str) -> Result<Self, String> {
        let form = Self { definition: definition.to_string() };
        form.validate("{}", "application/json")?;
        Ok(form)
    }

    pub fn from_file(filename: &str) -> Result<Self, String> {
        let filename = c_string(filename)?;
        let mut error = error_buffer();
        let output = unsafe { coolbox_http_form_load(filename.as_ptr(), error.as_mut_ptr(), ERROR_CAPACITY) };
        if output.is_null() { return Err(native_error(&error)); }
        Ok(Self { definition: unsafe { take_native_string(output)? } })
    }

    pub fn validate(&self, body: &str, content_type: &str) -> Result<ValidationResult, String> {
        let schema = c_string(&self.definition)?;
        let body = c_string(body)?;
        let content_type = c_string(content_type)?;
        let mut error = error_buffer();
        let output = unsafe { coolbox_http_form_validate(schema.as_ptr(), body.as_ptr(),
            content_type.as_ptr(), error.as_mut_ptr(), ERROR_CAPACITY) };
        if output.is_null() { return Err(native_error(&error)); }
        let text = unsafe { take_native_string(output)? };
        let result: serde_json::Value = serde_json::from_str(&text).map_err(|e| e.to_string())?;
        Ok(ValidationResult {
            valid: result["valid"].as_bool().ok_or("Missing validation status")?,
            values: result["values"].clone(),
            errors: serde_json::from_value(result["errors"].clone()).map_err(|e| e.to_string())?,
        })
    }

    pub fn validate_request(&self, request: &HttpRequest) -> Result<ValidationResult, String> {
        let content_type = request.headers.iter()
            .find(|(name, _)| name.eq_ignore_ascii_case("Content-Type"))
            .map(|(_, value)| value.as_str()).unwrap_or("");
        self.validate(&request.body, content_type)
    }
}

pub struct HttpServer {
    native: *mut CoolBoxRouteServer,
    contexts: Mutex<Vec<usize>>,
}

unsafe impl Send for HttpServer {}
unsafe impl Sync for HttpServer {}

impl HttpServer {
    pub fn new(port: i32) -> Result<Self, String> {
        let mut error = error_buffer();
        let native = unsafe { coolbox_http_server_create(port, error.as_mut_ptr(), ERROR_CAPACITY) };
        if native.is_null() {
            return Err(native_error(&error));
        }
        Ok(Self { native, contexts: Mutex::new(Vec::new()) })
    }

    pub fn route<F>(&self, method: &str, path: &str, handler: F) -> Result<(), String>
    where
        F: Fn(HttpRequest) -> HttpResponse + Send + Sync + 'static,
    {
        let method = c_string(method)?;
        let path = c_string(path)?;
        let callback: Box<dyn Fn(HttpRequest) -> HttpResponse + Send + Sync> = Box::new(handler);
        let context = Box::into_raw(Box::new(callback));
        let mut error = error_buffer();
        let success = unsafe {
            coolbox_http_server_add_route_v2(
                self.native,
                method.as_ptr(),
                path.as_ptr(),
                Some(route_handler),
                context.cast(),
                error.as_mut_ptr(),
                ERROR_CAPACITY,
            )
        };
        if success == 0 {
            unsafe { drop(Box::from_raw(context)) }
            return Err(native_error(&error));
        }
        self.contexts.lock().unwrap().push(context as usize);
        Ok(())
    }

    pub fn register_handler<F>(&self, name: &str, handler: F) -> Result<(), String>
    where
        F: Fn(HttpRequest) -> HttpResponse + Send + Sync + 'static,
    {
        let name = c_string(name)?;
        let callback: Box<dyn Fn(HttpRequest) -> HttpResponse + Send + Sync> = Box::new(handler);
        let context = Box::into_raw(Box::new(callback));
        let mut error = error_buffer();
        let success = unsafe {
            coolbox_http_server_register_handler_v2(
                self.native,
                name.as_ptr(),
                Some(route_handler),
                context.cast(),
                error.as_mut_ptr(),
                ERROR_CAPACITY,
            )
        };
        if success == 0 {
            unsafe { drop(Box::from_raw(context)) }
            return Err(native_error(&error));
        }
        self.contexts.lock().unwrap().push(context as usize);
        Ok(())
    }

    pub fn load_routes(&self, routes_file: &str) -> Result<(), String> {
        let routes_file = c_string(routes_file)?;
        let mut error = error_buffer();
        let success = unsafe {
            coolbox_http_server_load_routes(
                self.native,
                routes_file.as_ptr(),
                error.as_mut_ptr(),
                ERROR_CAPACITY,
            )
        };
        if success == 0 {
            Err(native_error(&error))
        } else {
            Ok(())
        }
    }

    pub fn start(&self) -> Result<(), String> {
        let mut error = error_buffer();
        let success = unsafe {
            coolbox_http_server_start(self.native, error.as_mut_ptr(), ERROR_CAPACITY)
        };
        if success == 0 {
            Err(native_error(&error))
        } else {
            Ok(())
        }
    }

    pub fn stop(&self) {
        unsafe { coolbox_http_server_stop(self.native) }
    }

    pub fn is_running(&self) -> bool {
        unsafe { coolbox_http_server_is_running(self.native) != 0 }
    }
}

impl Drop for HttpServer {
    fn drop(&mut self) {
        unsafe {
            coolbox_http_server_stop(self.native);
            coolbox_http_server_destroy(self.native);
        }
        for context in self.contexts.get_mut().unwrap().drain(..) {
            unsafe { drop(Box::from_raw(context as *mut Box<dyn Fn(HttpRequest) -> HttpResponse + Send + Sync>)) }
        }
    }
}
