#include "coolbox/coolbox_c.h"

#include "database.h"
#include "rest_api/server.h"
#include "rest_api/forms.h"
#include "json.h"

#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

struct CoolBoxSqlDatabase {
    std::unique_ptr<ml::sql::Database> value;
};

struct CoolBoxSqlSchema {
    ml::sql::Schema value;
};

struct CoolBoxSqlRepository {
    CoolBoxSqlDatabase *database;
    ml::sql::Repository<void> value;

    CoolBoxSqlRepository(CoolBoxSqlDatabase *db, const ml::sql::Model& model)
        : database(db), value(*db->value, model) {}
};

struct CoolBoxRouteServer {
    ml::rest_api::Server value;
    explicit CoolBoxRouteServer(int port) : value(port) {}
};

namespace {

void set_error(char *output, size_t capacity, const char *message) {
    if (!output || capacity == 0) return;
    const size_t length = std::min(capacity - 1, std::strlen(message));
    std::memcpy(output, message, length);
    output[length] = '\0';
}

char *copy_string(const std::string& value) {
    auto *result = static_cast<char *>(std::malloc(value.size() + 1));
    if (!result) return nullptr;
    std::memcpy(result, value.c_str(), value.size() + 1);
    return result;
}

template <typename Function>
int invoke(char *error, size_t capacity, Function&& function) {
    try {
        function();
        return 1;
    } catch (const std::exception& exception) {
        set_error(error, capacity, exception.what());
    } catch (...) {
        set_error(error, capacity, "unknown native exception");
    }
    return 0;
}

template <typename Function>
char *invoke_string(char *error, size_t capacity, Function&& function) {
    try {
        return copy_string(function());
    } catch (const std::exception& exception) {
        set_error(error, capacity, exception.what());
    } catch (...) {
        set_error(error, capacity, "unknown native exception");
    }
    return nullptr;
}

std::map<std::string, std::string> make_map(
    const char *const *keys, const char *const *values, size_t count) {
    if (count != 0 && (!keys || !values)) {
        throw std::invalid_argument("map keys and values are required");
    }
    std::map<std::string, std::string> result;
    for (size_t i = 0; i < count; ++i) {
        if (!keys[i]) throw std::invalid_argument("map key cannot be null");
        result[keys[i]] = values[i] ? values[i] : "";
    }
    return result;
}

ml::rest_api::HttpMethod parse_method(const std::string& method) {
    if (method == "GET") return ml::rest_api::HttpMethod::GET;
    if (method == "POST") return ml::rest_api::HttpMethod::POST;
    if (method == "PUT") return ml::rest_api::HttpMethod::PUT;
    if (method == "DELETE") return ml::rest_api::HttpMethod::DELETE;
    if (method == "PATCH") return ml::rest_api::HttpMethod::PATCH;
    if (method == "OPTIONS") return ml::rest_api::HttpMethod::OPTIONS;
    throw std::invalid_argument("unsupported HTTP method");
}

std::string method_name(ml::rest_api::HttpMethod method) {
    switch (method) {
        case ml::rest_api::HttpMethod::GET: return "GET";
        case ml::rest_api::HttpMethod::POST: return "POST";
        case ml::rest_api::HttpMethod::PUT: return "PUT";
        case ml::rest_api::HttpMethod::DELETE: return "DELETE";
        case ml::rest_api::HttpMethod::PATCH: return "PATCH";
        case ml::rest_api::HttpMethod::OPTIONS: return "OPTIONS";
    }
    return "";
}

} // namespace

extern "C" {

void coolbox_c_free_string(char *value) {
    std::free(value);
}

char *coolbox_c_duplicate_string(const char *value) {
    return copy_string(value ? value : "");
}

CoolBoxSqlDatabase *coolbox_sql_database_create(
    const char *provider, char *error, size_t error_capacity) {
    if (error && error_capacity) error[0] = '\0';
    try {
        if (!provider) throw std::invalid_argument("database provider is required");
        auto *database = new CoolBoxSqlDatabase;
        database->value = ml::sql::Database::create(provider);
        return database;
    } catch (const std::exception& exception) {
        set_error(error, error_capacity, exception.what());
        return nullptr;
    }
}

void coolbox_sql_database_destroy(CoolBoxSqlDatabase *database) {
    delete database;
}

int coolbox_sql_database_connect(
    CoolBoxSqlDatabase *database, const char *connection_string,
    char *error, size_t error_capacity) {
    if (error && error_capacity) error[0] = '\0';
    if (!database || !connection_string) {
        set_error(error, error_capacity, "database and connection string are required");
        return 0;
    }
    try {
        if (!database->value->connect(connection_string)) {
            const auto& message = database->value->last_error();
            set_error(error, error_capacity,
                      message.empty() ? "database connection failed" : message.c_str());
            return 0;
        }
        return 1;
    } catch (const std::exception& exception) {
        set_error(error, error_capacity, exception.what());
        return 0;
    }
}

void coolbox_sql_database_disconnect(CoolBoxSqlDatabase *database) {
    if (database) database->value->disconnect();
}

char *coolbox_sql_database_execute(
    CoolBoxSqlDatabase *database, const char *query,
    const char *const *params, size_t param_count,
    char *error, size_t error_capacity) {
    if (error && error_capacity) error[0] = '\0';
    return invoke_string(error, error_capacity, [&]() {
        if (!database || !query) throw std::invalid_argument("database and query are required");
        std::vector<std::string> values;
        values.reserve(param_count);
        for (size_t i = 0; i < param_count; ++i) {
            if (!params || !params[i]) throw std::invalid_argument("query parameter cannot be null");
            values.emplace_back(params[i]);
        }
        return database->value->execute(query, values).to_json();
    });
}

int coolbox_sql_database_begin(CoolBoxSqlDatabase *database) {
    return database && database->value->begin_transaction();
}
int coolbox_sql_database_commit(CoolBoxSqlDatabase *database) {
    return database && database->value->commit();
}
int coolbox_sql_database_rollback(CoolBoxSqlDatabase *database) {
    return database && database->value->rollback();
}

CoolBoxSqlSchema *coolbox_sql_schema_parse(
    const char *schema_text, char *error, size_t error_capacity) {
    if (error && error_capacity) error[0] = '\0';
    try {
        if (!schema_text) throw std::invalid_argument("schema text is required");
        auto *schema = new CoolBoxSqlSchema;
        schema->value = ml::sql::SchemaParser(schema_text).parse();
        return schema;
    } catch (const std::exception& exception) {
        set_error(error, error_capacity, exception.what());
        return nullptr;
    }
}

void coolbox_sql_schema_destroy(CoolBoxSqlSchema *schema) {
    delete schema;
}

size_t coolbox_sql_schema_model_count(const CoolBoxSqlSchema *schema) {
    return schema ? schema->value.models().size() : 0;
}

char *coolbox_sql_schema_model_name(const CoolBoxSqlSchema *schema, size_t index) {
    if (!schema || index >= schema->value.models().size()) return nullptr;
    return copy_string(schema->value.models()[index].name);
}

int coolbox_sql_migrate_up(
    CoolBoxSqlDatabase *database, const CoolBoxSqlSchema *schema,
    char *error, size_t error_capacity) {
    if (error && error_capacity) error[0] = '\0';
    if (!database || !schema) {
        set_error(error, error_capacity, "database and schema are required");
        return 0;
    }
    return invoke(error, error_capacity, [&]() {
        if (!ml::sql::Migrator(*database->value, schema->value).migrate_up()) {
            throw std::runtime_error("database migration failed");
        }
    });
}

int coolbox_sql_migrations_up(
    CoolBoxSqlDatabase *database, const char *directory, char *error, size_t error_capacity) {
    if (error && error_capacity) error[0] = '\0';
    return invoke(error, error_capacity, [&]() {
        if (!database || !directory) {
            throw std::invalid_argument("database and migration directory are required");
        }
        ml::sql::MigrationRunner(*database->value, directory).migrate_up();
    });
}

int coolbox_sql_migrations_down(
    CoolBoxSqlDatabase *database, const char *directory, size_t steps,
    char *error, size_t error_capacity) {
    if (error && error_capacity) error[0] = '\0';
    return invoke(error, error_capacity, [&]() {
        if (!database || !directory) {
            throw std::invalid_argument("database and migration directory are required");
        }
        ml::sql::MigrationRunner(*database->value, directory).migrate_down(steps);
    });
}

char *coolbox_sql_migrations_current_version(
    CoolBoxSqlDatabase *database, const char *directory, char *error, size_t error_capacity) {
    if (error && error_capacity) error[0] = '\0';
    return invoke_string(error, error_capacity, [&]() {
        if (!database || !directory) {
            throw std::invalid_argument("database and migration directory are required");
        }
        return ml::sql::MigrationRunner(*database->value, directory).current_version();
    });
}

CoolBoxSqlRepository *coolbox_sql_repository_create(
    CoolBoxSqlDatabase *database, const CoolBoxSqlSchema *schema,
    const char *model_name, char *error, size_t error_capacity) {
    if (error && error_capacity) error[0] = '\0';
    try {
        if (!database || !schema || !model_name) {
            throw std::invalid_argument("database, schema, and model name are required");
        }
        const auto *model = schema->value.get_model(model_name);
        if (!model) throw std::invalid_argument("model was not found in schema");
        return new CoolBoxSqlRepository(database, *model);
    } catch (const std::exception& exception) {
        set_error(error, error_capacity, exception.what());
        return nullptr;
    }
}

void coolbox_sql_repository_destroy(CoolBoxSqlRepository *repository) {
    delete repository;
}

char *coolbox_sql_repository_call(
    CoolBoxSqlRepository *repository, const char *operation, const char *id,
    const char *const *keys, const char *const *values, size_t count,
    const char *const *condition_keys, const char *const *condition_values,
    size_t condition_count, char *error, size_t error_capacity) {
    if (error && error_capacity) error[0] = '\0';
    return invoke_string(error, error_capacity, [&]() {
        if (!repository || !operation) {
            throw std::invalid_argument("repository and operation are required");
        }
        auto data = make_map(keys, values, count);
        auto conditions = make_map(condition_keys, condition_values, condition_count);
        ml::sql::ResultSet result;
        const std::string op(operation);
        if (op == "find_all") result = repository->value.find_all();
        else if (op == "find_by_id") {
            if (!id) throw std::invalid_argument("record id is required");
            result = repository->value.find_by_id(id);
        } else if (op == "find_where") result = repository->value.find_where(conditions);
        else if (op == "create") result = repository->value.create(data);
        else if (op == "update") {
            if (!id) throw std::invalid_argument("record id is required");
            result = repository->value.update(id, data);
        } else if (op == "update_where") result = repository->value.update_where(conditions, data);
        else if (op == "remove") {
            if (!id) throw std::invalid_argument("record id is required");
            result = repository->value.remove(id);
        } else if (op == "remove_where") result = repository->value.remove_where(conditions);
        else if (op == "create_table") {
            if (!repository->value.create_table()) throw std::runtime_error("table creation failed");
            return std::string("{\"success\":true}");
        } else if (op == "drop_table") {
            if (!repository->value.drop_table()) throw std::runtime_error("table drop failed");
            return std::string("{\"success\":true}");
        } else {
            throw std::invalid_argument("unsupported repository operation");
        }
        return result.to_json();
    });
}

CoolBoxRouteServer *coolbox_http_server_create(
    int port, char *error, size_t error_capacity) {
    if (error && error_capacity) error[0] = '\0';
    try {
        if (port < 0 || port > 65535) throw std::invalid_argument("port must be 0-65535");
        return new CoolBoxRouteServer(port);
    } catch (const std::exception& exception) {
        set_error(error, error_capacity, exception.what());
        return nullptr;
    }
}

int coolbox_http_server_add_route(
    CoolBoxRouteServer *server, const char *method, const char *path,
    CoolBoxRouteHandler handler, void *context, char *error, size_t error_capacity) {
    if (error && error_capacity) error[0] = '\0';
    return invoke(error, error_capacity, [&]() {
        if (!server || !method || !path || !handler) {
            throw std::invalid_argument("server, method, path, and handler are required");
        }
        const auto http_method = parse_method(method);
        auto route_handler = [handler, context, http_method](const ml::rest_api::Request& request) {
            int status_code = 200;
            std::vector<std::string> path_keys, path_values, query_keys, query_values;
            std::vector<const char *> path_key_ptrs, path_value_ptrs, query_key_ptrs, query_value_ptrs;
            for (const auto& [key, value] : request.path_params()) {
                path_keys.push_back(key);
                path_values.push_back(value);
            }
            for (const auto& [key, value] : request.query_params()) {
                query_keys.push_back(key);
                query_values.push_back(value);
            }
            for (const auto& value : path_keys) path_key_ptrs.push_back(value.c_str());
            for (const auto& value : path_values) path_value_ptrs.push_back(value.c_str());
            for (const auto& value : query_keys) query_key_ptrs.push_back(value.c_str());
            for (const auto& value : query_values) query_value_ptrs.push_back(value.c_str());
            char *body = handler(context, method_name(http_method).c_str(),
                                 request.path().c_str(), request.body().c_str(),
                                 path_key_ptrs.data(), path_value_ptrs.data(), path_keys.size(),
                                 query_key_ptrs.data(), query_value_ptrs.data(), query_keys.size(),
                                 &status_code);
            if (status_code < 100 || status_code > 599) {
                std::free(body);
                return ml::rest_api::Response(500, "Invalid handler status code");
            }
            ml::rest_api::Response response(status_code, body ? body : "");
            std::free(body);
            response.set_header("Content-Type", "application/json");
            return response;
        };
        switch (http_method) {
            case ml::rest_api::HttpMethod::GET: server->value.get(path, route_handler); break;
            case ml::rest_api::HttpMethod::POST: server->value.post(path, route_handler); break;
            case ml::rest_api::HttpMethod::PUT: server->value.put(path, route_handler); break;
            case ml::rest_api::HttpMethod::DELETE: server->value.delete_(path, route_handler); break;
            case ml::rest_api::HttpMethod::PATCH: server->value.patch(path, route_handler); break;
            case ml::rest_api::HttpMethod::OPTIONS: server->value.options(path, route_handler); break;
        }
    });
}

int coolbox_http_server_register_handler(
    CoolBoxRouteServer *server, const char *name,
    CoolBoxRouteHandler handler, void *context, char *error, size_t error_capacity) {
    if (error && error_capacity) error[0] = '\0';
    return invoke(error, error_capacity, [&]() {
        if (!server || !name || !handler) {
            throw std::invalid_argument("server, handler name, and function are required");
        }
        server->value.register_handler(name, [handler, context](const ml::rest_api::Request& request) {
            int status_code = 200;
            std::vector<std::string> path_keys, path_values, query_keys, query_values;
            std::vector<const char *> path_key_ptrs, path_value_ptrs, query_key_ptrs, query_value_ptrs;
            for (const auto& [key, value] : request.path_params()) {
                path_keys.push_back(key);
                path_values.push_back(value);
            }
            for (const auto& [key, value] : request.query_params()) {
                query_keys.push_back(key);
                query_values.push_back(value);
            }
            for (const auto& value : path_keys) path_key_ptrs.push_back(value.c_str());
            for (const auto& value : path_values) path_value_ptrs.push_back(value.c_str());
            for (const auto& value : query_keys) query_key_ptrs.push_back(value.c_str());
            for (const auto& value : query_values) query_value_ptrs.push_back(value.c_str());
            const std::string method_value = method_name(request.method());
            const char *method = method_value.c_str();
            char *body = handler(context, method, request.path().c_str(), request.body().c_str(),
                                 path_key_ptrs.data(), path_value_ptrs.data(), path_keys.size(),
                                 query_key_ptrs.data(), query_value_ptrs.data(), query_keys.size(),
                                 &status_code);
            if (status_code < 100 || status_code > 599) {
                std::free(body);
                return ml::rest_api::Response(500, "Invalid handler status code");
            }
            ml::rest_api::Response response(status_code, body ? body : "");
            std::free(body);
            response.set_header("Content-Type", "application/json");
            return response;
        });
    });
}

int coolbox_http_server_load_routes(
    CoolBoxRouteServer *server, const char *routes_file, char *error, size_t error_capacity) {
    if (error && error_capacity) error[0] = '\0';
    return invoke(error, error_capacity, [&]() {
        if (!server || !routes_file) {
            throw std::invalid_argument("server and routes file are required");
        }
        server->value.load_routes(routes_file);
    });
}

int coolbox_http_server_start(
    CoolBoxRouteServer *server, char *error, size_t error_capacity) {
    if (error && error_capacity) error[0] = '\0';
    if (!server) {
        set_error(error, error_capacity, "server is required");
        return 0;
    }
    return invoke(error, error_capacity, [&]() { server->value.start(); });
}

void coolbox_http_server_stop(CoolBoxRouteServer *server) {
    if (server) server->value.stop();
}

int coolbox_http_server_is_running(const CoolBoxRouteServer *server) {
    return server && server->value.is_running();
}

void coolbox_http_server_destroy(CoolBoxRouteServer *server) {
    delete server;
}

} // extern "C"

namespace {
using dataformats::json::Object;
using dataformats::json::Parser;

std::map<std::string, std::string> json_string_map(const std::string& text) {
    const auto value = Parser::parse(text);
    if (!value.is_object()) throw std::invalid_argument("Expected a JSON object");
    std::map<std::string, std::string> result;
    for (const auto& [key, item] : value.as_object()) {
        if (!item.is_string()) throw std::invalid_argument("Expected JSON string values");
        result[key] = item.as_string();
    }
    return result;
}

Object json_object(const std::map<std::string, std::string>& values) {
    Object object;
    for (const auto& [key, value] : values) object.set(key, value);
    return object;
}

ml::rest_api::RouteHandler callback_v2(CoolBoxRouteHandlerV2 handler, void* context) {
    return [handler, context](const ml::rest_api::Request& request) {
        int status = 200;
        std::vector<const char*> pk, pv, qk, qv;
        for (const auto& [key, value] : request.path_params()) {
            pk.push_back(key.c_str()); pv.push_back(value.c_str());
        }
        for (const auto& [key, value] : request.query_params()) {
            qk.push_back(key.c_str()); qv.push_back(value.c_str());
        }
        const auto headers = json_object(request.headers()).to_string();
        char* raw_headers = nullptr;
        char* raw_body = handler(context, method_name(request.method()).c_str(),
            request.path().c_str(), request.body().c_str(), headers.c_str(),
            pk.data(), pv.data(), pk.size(), qk.data(), qv.data(), qk.size(),
            &status, &raw_headers);
        std::unique_ptr<char, decltype(&std::free)> body(raw_body, &std::free);
        std::unique_ptr<char, decltype(&std::free)> response_headers(raw_headers, &std::free);
        if (status < 100 || status > 599) throw std::invalid_argument("Invalid handler status code");
        if (!body) throw std::runtime_error("HTTP callback returned no body");
        ml::rest_api::Response response(status, body.get());
        response.set_header("Content-Type", "application/json");
        if (response_headers) {
            for (const auto& [key, value] : json_string_map(response_headers.get())) {
                response.set_header(key, value);
            }
        }
        return response;
    };
}
} // namespace

extern "C" {
int coolbox_http_server_add_route_v2(
    CoolBoxRouteServer* server, const char* method, const char* path,
    CoolBoxRouteHandlerV2 handler, void* context, char* error, size_t capacity) {
    return invoke(error, capacity, [&]() {
        if (!server || !method || !path || !handler) throw std::invalid_argument("Route arguments are required");
        const auto callback = callback_v2(handler, context);
        switch (parse_method(method)) {
            case ml::rest_api::HttpMethod::GET: server->value.get(path, callback); break;
            case ml::rest_api::HttpMethod::POST: server->value.post(path, callback); break;
            case ml::rest_api::HttpMethod::PUT: server->value.put(path, callback); break;
            case ml::rest_api::HttpMethod::DELETE: server->value.delete_(path, callback); break;
            case ml::rest_api::HttpMethod::PATCH: server->value.patch(path, callback); break;
            case ml::rest_api::HttpMethod::OPTIONS: server->value.options(path, callback); break;
        }
    });
}

int coolbox_http_server_register_handler_v2(
    CoolBoxRouteServer* server, const char* name, CoolBoxRouteHandlerV2 handler,
    void* context, char* error, size_t capacity) {
    return invoke(error, capacity, [&]() {
        if (!server || !name || !handler) throw std::invalid_argument("Handler arguments are required");
        server->value.register_handler(name, callback_v2(handler, context));
    });
}

char* coolbox_http_form_load(const char* filename, char* error, size_t capacity) {
    return invoke_string(error, capacity, [&]() {
        if (!filename) throw std::invalid_argument("Schema filename is required");
        return ml::rest_api::FormSchema::from_file(filename).definition();
    });
}

char* coolbox_http_form_validate(const char* schema, const char* body, const char* content_type,
                                char* error, size_t capacity) {
    return invoke_string(error, capacity, [&]() {
        if (!schema || !body || !content_type) throw std::invalid_argument("Form arguments are required");
        return ml::rest_api::FormSchema(schema).validate(body, content_type).to_json();
    });
}

char* coolbox_http_html_file(const char* root, const char* filename, const char* context,
                            int render, char* error, size_t capacity) {
    return invoke_string(error, capacity, [&]() {
        if (!root || !filename || (render && !context)) throw std::invalid_argument("HTML arguments are required");
        const auto response = render
            ? ml::rest_api::Response::render(root, filename, json_string_map(context))
            : ml::rest_api::Response::html_file(root, filename);
        Object result;
        result.set("status_code", response.status_code());
        result.set("body", response.body());
        result.set("headers", json_object(response.headers()));
        return result.to_string();
    });
}
} // extern "C"
