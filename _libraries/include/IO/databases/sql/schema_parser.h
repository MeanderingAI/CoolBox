/**
 * Prisma Schema Parser
 * 
 * Parses Prisma schema files and extracts model definitions
 * for automatic CRUD generation.
 */

#ifndef ML_SQL_SCHEMA_PARSER_H
#define ML_SQL_SCHEMA_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace ml {
namespace sql {

// Field types supported in Prisma schemas
enum class FieldType {
    INT,
    BIGINT,
    STRING,
    BOOLEAN,
    FLOAT,
    DOUBLE,
    DATETIME,
    JSON,
    BYTES,
    DECIMAL
};

// Field attributes
struct FieldAttribute {
    std::string name;
    std::vector<std::string> args;
};

// Model field definition
struct Field {
    std::string name;
    FieldType type;
    bool is_optional = false;
    bool is_array = false;
    bool is_id = false;
    bool is_unique = false;
    bool auto_increment = false;
    std::string default_value;
    std::vector<FieldAttribute> attributes;
    
    // Relationship fields
    bool is_relation = false;
    std::string relation_model;
    std::string relation_field;
    
    std::string to_sql_type(const std::string& db_type = "sqlite") const;
};

// Model definition from Prisma schema
struct Model {
    std::string name;
    std::string table_name;
    std::vector<Field> fields;
    std::vector<std::string> indexes;
    std::map<std::string, std::string> attributes;
    
    // Get primary key field(s)
    std::vector<Field> get_primary_keys() const;
    
    // Get non-relation fields
    std::vector<Field> get_data_fields() const;
    
    // Get relation fields
    std::vector<Field> get_relation_fields() const;
};

// Prisma schema representation
class Schema {
private:
    std::vector<Model> models_;
    std::map<std::string, std::string> datasource_;
    std::map<std::string, std::string> generator_;
    
public:
    Schema() = default;
    
    void add_model(const Model& model);
    const std::vector<Model>& models() const { return models_; }
    Model* get_model(const std::string& name);
    const Model* get_model(const std::string& name) const;
    
    void set_datasource(const std::map<std::string, std::string>& ds) { datasource_ = ds; }
    void set_generator(const std::map<std::string, std::string>& gen) { generator_ = gen; }
    
    const std::map<std::string, std::string>& datasource() const { return datasource_; }
    const std::map<std::string, std::string>& generator() const { return generator_; }
    
    std::string get_provider() const;
};

// Prisma schema parser
class SchemaParser {
private:
    std::string content_;
    size_t pos_ = 0;
    
    // Parsing helpers
    void skip_whitespace();
    void skip_comment();
    char peek() const;
    char advance();
    std::string read_word();
    std::string read_until(char delimiter);
    std::string read_string();
    
    // Block parsing
    std::map<std::string, std::string> parse_block(const std::string& block_type);
    Model parse_model_block();
    Field parse_field_line(const std::string& line);
    FieldType parse_field_type(const std::string& type_str);
    std::vector<FieldAttribute> parse_attributes(const std::string& attr_str);
    
public:
    SchemaParser(const std::string& content) : content_(content) {}
    
    // Parse schema from string
    Schema parse();
    
    // Parse schema from file
    static Schema parse_file(const std::string& filepath);
};

// Convert Prisma type to SQL type
std::string prisma_type_to_sql(FieldType type, const std::string& db_provider = "sqlite");

// Generate SQL type with constraints
std::string field_to_sql_definition(const Field& field, const std::string& db_provider = "sqlite");

} // namespace sql
} // namespace ml

#endif // ML_SQL_SCHEMA_PARSER_H
