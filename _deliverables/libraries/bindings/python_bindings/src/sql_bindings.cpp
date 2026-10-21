#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "database.h"

namespace py = pybind11;

void bind_sql(py::module_& module) {
    using namespace ml::sql;
    py::module_ sql = module.def_submodule("sql", "Prisma schema and SQL database APIs");

    py::enum_<FieldType>(sql, "FieldType")
        .value("INT", FieldType::INT)
        .value("BIGINT", FieldType::BIGINT)
        .value("STRING", FieldType::STRING)
        .value("BOOLEAN", FieldType::BOOLEAN)
        .value("FLOAT", FieldType::FLOAT)
        .value("DOUBLE", FieldType::DOUBLE)
        .value("DATETIME", FieldType::DATETIME)
        .value("JSON", FieldType::JSON)
        .value("BYTES", FieldType::BYTES)
        .value("DECIMAL", FieldType::DECIMAL);

    py::class_<Field>(sql, "Field")
        .def(py::init<>())
        .def_readwrite("name", &Field::name)
        .def_readwrite("type", &Field::type)
        .def_readwrite("is_optional", &Field::is_optional)
        .def_readwrite("is_array", &Field::is_array)
        .def_readwrite("is_id", &Field::is_id)
        .def_readwrite("is_unique", &Field::is_unique)
        .def_readwrite("auto_increment", &Field::auto_increment)
        .def_readwrite("default_value", &Field::default_value)
        .def("to_sql_type", &Field::to_sql_type, py::arg("provider") = "sqlite");

    py::class_<Model>(sql, "Model")
        .def(py::init<>())
        .def_readwrite("name", &Model::name)
        .def_readwrite("table_name", &Model::table_name)
        .def_readwrite("fields", &Model::fields)
        .def("get_primary_keys", &Model::get_primary_keys)
        .def("get_data_fields", &Model::get_data_fields);

    py::class_<Schema>(sql, "Schema")
        .def(py::init<>())
        .def("models", &Schema::models, py::return_value_policy::reference_internal)
        .def("get_model", [](Schema& schema, const std::string& name) {
            return schema.get_model(name);
        },
             py::return_value_policy::reference_internal)
        .def("get_provider", &Schema::get_provider);

    py::class_<SchemaParser>(sql, "SchemaParser")
        .def(py::init<const std::string&>())
        .def("parse", &SchemaParser::parse)
        .def_static("parse_file", &SchemaParser::parse_file);

    py::class_<ResultSet>(sql, "ResultSet")
        .def_readonly("columns", &ResultSet::columns)
        .def_readonly("rows", &ResultSet::rows)
        .def_readonly("affected_rows", &ResultSet::affected_rows)
        .def_readonly("last_insert_id", &ResultSet::last_insert_id)
        .def("empty", &ResultSet::empty)
        .def("__len__", &ResultSet::size)
        .def("get_single", &ResultSet::get_single)
        .def("get_first", &ResultSet::get_first)
        .def("to_json", &ResultSet::to_json);

    py::class_<Database>(sql, "Database")
        .def_static("create", &Database::create, py::arg("provider"))
        .def("connect", &Database::connect, py::arg("connection_string"))
        .def("disconnect", &Database::disconnect)
        .def("is_connected", &Database::is_connected)
        .def("execute", [](Database& db, const std::string& query,
                           const std::vector<std::string>& params) {
            return db.execute(query, params);
        }, py::arg("query"), py::arg("params") = std::vector<std::string>{})
        .def("begin_transaction", &Database::begin_transaction)
        .def("commit", &Database::commit)
        .def("rollback", &Database::rollback)
        .def("create_table_from_model", &Database::create_table_from_model)
        .def("drop_table", &Database::drop_table)
        .def("get_provider", &Database::get_provider);

    using BoundRepository = Repository<void>;
    py::class_<BoundRepository>(sql, "Repository")
        .def(py::init<Database&, const Model&>(), py::keep_alive<1, 2>())
        .def("find_all", &BoundRepository::find_all)
        .def("find_by_id", &BoundRepository::find_by_id)
        .def("find_where", &BoundRepository::find_where)
        .def("create", &BoundRepository::create)
        .def("update", &BoundRepository::update)
        .def("update_where", &BoundRepository::update_where)
        .def("remove", &BoundRepository::remove)
        .def("remove_where", &BoundRepository::remove_where)
        .def("create_table", &BoundRepository::create_table)
        .def("drop_table", &BoundRepository::drop_table);

    py::class_<Migrator>(sql, "Migrator")
        .def(py::init<Database&, const Schema&>(),
             py::keep_alive<1, 2>(), py::keep_alive<1, 3>())
        .def("migrate_up", &Migrator::migrate_up)
        .def("migrate_down", &Migrator::migrate_down)
        .def("reset", &Migrator::reset)
        .def("generate_migration_sql", &Migrator::generate_migration_sql);

    py::class_<MigrationRunner>(sql, "MigrationRunner")
        .def(py::init<Database&, const std::string&>(),
             py::keep_alive<1, 2>())
        .def("migrate_up", &MigrationRunner::migrate_up)
        .def("migrate_down", &MigrationRunner::migrate_down,
             py::arg("steps") = 1)
        .def("current_version", &MigrationRunner::current_version);
}
