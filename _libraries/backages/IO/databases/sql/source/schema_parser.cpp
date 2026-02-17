#include "../headers/schema_parser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace ml {
namespace sql {

// =========================================================================
// Field Implementation
// =========================================================================

std::string Field::to_sql_type(const std::string& db_type) const {
    return prisma_type_to_sql(type, db_type);
}

// =========================================================================
// Model Implementation
// =========================================================================

std::vector<Field> Model::get_primary_keys() const {
    std::vector<Field> pks;
    for (const auto& field : fields) {
        if (field.is_id) {
            pks.push_back(field);
        }
    }
    return pks;
}

std::vector<Field> Model::get_data_fields() const {
    std::vector<Field> data_fields;
    for (const auto& field : fields) {
        if (!field.is_relation) {
            data_fields.push_back(field);
        }
    }
    return data_fields;
}

std::vector<Field> Model::get_relation_fields() const {
    std::vector<Field> rel_fields;
    for (const auto& field : fields) {
        if (field.is_relation) {
            rel_fields.push_back(field);
        }
    }
    return rel_fields;
}

// =========================================================================
// Schema Implementation
// =========================================================================

void Schema::add_model(const Model& model) {
    models_.push_back(model);
}

Model* Schema::get_model(const std::string& name) {
    for (auto& model : models_) {
        if (model.name == name) {
            return &model;
        }
    }
    return nullptr;
}

const Model* Schema::get_model(const std::string& name) const {
    for (const auto& model : models_) {
        if (model.name == name) {
            return &model;
        }
    }
    return nullptr;
}

std::string Schema::get_provider() const {
    auto it = datasource_.find("provider");
    if (it != datasource_.end()) {
        return it->second;
    }
    return "sqlite";
}

// =========================================================================
// SchemaParser Implementation
// =========================================================================

void SchemaParser::skip_whitespace() {
    while (pos_ < content_.length() && std::isspace(content_[pos_])) {
        pos_++;
    }
}

void SchemaParser::skip_comment() {
    if (pos_ < content_.length() && content_[pos_] == '/') {
        if (pos_ + 1 < content_.length() && content_[pos_ + 1] == '/') {
            // Single line comment
            while (pos_ < content_.length() && content_[pos_] != '\n') {
                pos_++;
            }
        }
    }
}

char SchemaParser::peek() const {
    if (pos_ < content_.length()) {
        return content_[pos_];
    }
    return '\0';
}

char SchemaParser::advance() {
    if (pos_ < content_.length()) {
        return content_[pos_++];
    }
    return '\0';
}

std::string SchemaParser::read_word() {
    skip_whitespace();
    std::string word;
    while (pos_ < content_.length() && 
           (std::isalnum(content_[pos_]) || content_[pos_] == '_')) {
        word += content_[pos_++];
    }
    return word;
}

std::string SchemaParser::read_until(char delimiter) {
    std::string result;
    while (pos_ < content_.length() && content_[pos_] != delimiter) {
        result += content_[pos_++];
    }
    return result;
}

std::string SchemaParser::read_string() {
    skip_whitespace();
    if (peek() == '"') {
        advance();  // Skip opening quote
        std::string str = read_until('"');
        advance();  // Skip closing quote
        return str;
    }
    return read_word();
}

std::map<std::string, std::string> SchemaParser::parse_block(const std::string& block_type) {
    std::map<std::string, std::string> result;
    
    skip_whitespace();
    std::string name = read_word();
    skip_whitespace();
    
    if (peek() == '{') {
        advance();  // Skip '{'
        
        while (pos_ < content_.length() && peek() != '}') {
            skip_whitespace();
            skip_comment();
            
            if (peek() == '}') break;
            
            std::string key = read_word();
            skip_whitespace();
            
            if (peek() == '=') {
                advance();  // Skip '='
                skip_whitespace();
                std::string value = read_string();
                result[key] = value;
            }
            
            // Skip to next line
            while (pos_ < content_.length() && peek() != '\n' && peek() != '}') {
                advance();
            }
            if (peek() == '\n') advance();
        }
        
        if (peek() == '}') advance();  // Skip '}'
    }
    
    return result;
}

FieldType SchemaParser::parse_field_type(const std::string& type_str) {
    std::string type_lower = type_str;
    std::transform(type_lower.begin(), type_lower.end(), type_lower.begin(), ::tolower);
    
    if (type_lower == "int") return FieldType::INT;
    if (type_lower == "bigint") return FieldType::BIGINT;
    if (type_lower == "string") return FieldType::STRING;
    if (type_lower == "boolean" || type_lower == "bool") return FieldType::BOOLEAN;
    if (type_lower == "float") return FieldType::FLOAT;
    if (type_lower == "double") return FieldType::DOUBLE;
    if (type_lower == "datetime") return FieldType::DATETIME;
    if (type_lower == "json") return FieldType::JSON;
    if (type_lower == "bytes") return FieldType::BYTES;
    if (type_lower == "decimal") return FieldType::DECIMAL;
    
    return FieldType::STRING;  // Default
}

std::vector<FieldAttribute> SchemaParser::parse_attributes(const std::string& attr_str) {
    std::vector<FieldAttribute> attributes;
    
    size_t pos = 0;
    while (pos < attr_str.length()) {
        if (attr_str[pos] == '@') {
            pos++;  // Skip '@'
            
            FieldAttribute attr;
            
            // Read attribute name
            while (pos < attr_str.length() && 
                   (std::isalnum(attr_str[pos]) || attr_str[pos] == '_')) {
                attr.name += attr_str[pos++];
            }
            
            // Check for arguments
            while (pos < attr_str.length() && std::isspace(attr_str[pos])) pos++;
            
            if (pos < attr_str.length() && attr_str[pos] == '(') {
                pos++;  // Skip '('
                std::string arg;
                int depth = 1;
                
                while (pos < attr_str.length() && depth > 0) {
                    if (attr_str[pos] == '(') depth++;
                    if (attr_str[pos] == ')') {
                        depth--;
                        if (depth == 0) break;
                    }
                    if (attr_str[pos] == ',' && depth == 1) {
                        attr.args.push_back(arg);
                        arg.clear();
                    } else {
                        arg += attr_str[pos];
                    }
                    pos++;
                }
                
                if (!arg.empty()) {
                    attr.args.push_back(arg);
                }
                
                if (pos < attr_str.length() && attr_str[pos] == ')') {
                    pos++;  // Skip ')'
                }
            }
            
            attributes.push_back(attr);
        }
        pos++;
    }
    
    return attributes;
}

Field SchemaParser::parse_field_line(const std::string& line) {
    Field field;
    
    std::istringstream iss(line);
    std::string token;
    
    // Read field name
    iss >> field.name;
    
    // Read field type
    iss >> token;
    
    // Check for optional (?)
    if (!token.empty() && token.back() == '?') {
        field.is_optional = true;
        token.pop_back();
    }
    
    // Check for array ([])
    if (!token.empty() && token.back() == ']' && token[token.length() - 2] == '[') {
        field.is_array = true;
        token = token.substr(0, token.length() - 2);
    }
    
    field.type = parse_field_type(token);
    
    // Read attributes
    std::string rest;
    std::getline(iss, rest);
    field.attributes = parse_attributes(rest);
    
    // Process attributes
    for (const auto& attr : field.attributes) {
        if (attr.name == "id") {
            field.is_id = true;
            if (!attr.args.empty() && attr.args[0] == "auto") {
                field.auto_increment = true;
            }
        } else if (attr.name == "unique") {
            field.is_unique = true;
        } else if (attr.name == "default") {
            if (!attr.args.empty()) {
                field.default_value = attr.args[0];
            }
        }
    }
    
    return field;
}

Model SchemaParser::parse_model_block() {
    Model model;
    
    skip_whitespace();
    model.name = read_word();
    model.table_name = model.name;
    
    skip_whitespace();
    
    if (peek() == '{') {
        advance();  // Skip '{'
        
        while (pos_ < content_.length() && peek() != '}') {
            skip_whitespace();
            skip_comment();
            
            if (peek() == '}') break;
            
            // Read line
            std::string line;
            while (pos_ < content_.length() && peek() != '\n' && peek() != '}') {
                line += advance();
            }
            
            if (!line.empty()) {
                // Trim whitespace
                line.erase(0, line.find_first_not_of(" \t"));
                line.erase(line.find_last_not_of(" \t") + 1);
                
                if (!line.empty() && line[0] != '/' && line.find("@@") == std::string::npos) {
                    Field field = parse_field_line(line);
                    model.fields.push_back(field);
                }
            }
            
            if (peek() == '\n') advance();
        }
        
        if (peek() == '}') advance();  // Skip '}'
    }
    
    return model;
}

Schema SchemaParser::parse() {
    Schema schema;
    
    while (pos_ < content_.length()) {
        skip_whitespace();
        skip_comment();
        
        std::string keyword = read_word();
        
        if (keyword == "datasource") {
            auto ds = parse_block("datasource");
            schema.set_datasource(ds);
        } else if (keyword == "generator") {
            auto gen = parse_block("generator");
            schema.set_generator(gen);
        } else if (keyword == "model") {
            Model model = parse_model_block();
            schema.add_model(model);
        }
    }
    
    return schema;
}

Schema SchemaParser::parse_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open schema file: " + filepath);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    SchemaParser parser(content);
    return parser.parse();
}

// =========================================================================
// Helper Functions
// =========================================================================

std::string prisma_type_to_sql(FieldType type, const std::string& db_provider) {
    if (db_provider == "sqlite") {
        switch (type) {
            case FieldType::INT: return "INTEGER";
            case FieldType::BIGINT: return "INTEGER";
            case FieldType::STRING: return "TEXT";
            case FieldType::BOOLEAN: return "INTEGER";
            case FieldType::FLOAT: return "REAL";
            case FieldType::DOUBLE: return "REAL";
            case FieldType::DATETIME: return "TEXT";
            case FieldType::JSON: return "TEXT";
            case FieldType::BYTES: return "BLOB";
            case FieldType::DECIMAL: return "REAL";
        }
    } else if (db_provider == "postgresql") {
        switch (type) {
            case FieldType::INT: return "INTEGER";
            case FieldType::BIGINT: return "BIGINT";
            case FieldType::STRING: return "VARCHAR(255)";
            case FieldType::BOOLEAN: return "BOOLEAN";
            case FieldType::FLOAT: return "REAL";
            case FieldType::DOUBLE: return "DOUBLE PRECISION";
            case FieldType::DATETIME: return "TIMESTAMP";
            case FieldType::JSON: return "JSONB";
            case FieldType::BYTES: return "BYTEA";
            case FieldType::DECIMAL: return "DECIMAL";
        }
    }
    
    return "TEXT";
}

std::string field_to_sql_definition(const Field& field, const std::string& db_provider) {
    std::string sql = field.name + " " + field.to_sql_type(db_provider);
    
    if (field.is_id) {
        sql += " PRIMARY KEY";
        if (field.auto_increment && db_provider == "sqlite") {
            sql += " AUTOINCREMENT";
        }
    }
    
    if (!field.is_optional && !field.is_id) {
        sql += " NOT NULL";
    }
    
    if (field.is_unique && !field.is_id) {
        sql += " UNIQUE";
    }
    
    if (!field.default_value.empty()) {
        sql += " DEFAULT " + field.default_value;
    }
    
    return sql;
}

} // namespace sql
} // namespace ml
