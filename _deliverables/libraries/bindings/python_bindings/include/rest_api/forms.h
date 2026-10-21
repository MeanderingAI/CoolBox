#ifndef ML_REST_API_FORMS_H
#define ML_REST_API_FORMS_H

#include <map>
#include <string>
#include <vector>

namespace ml {
namespace rest_api {

struct ValidationResult {
    bool valid = false;
    std::string values_json;
    std::map<std::string, std::vector<std::string>> errors;
    std::string to_json() const;
};

class FormSchema {
public:
    explicit FormSchema(const std::string& definition);
    static FormSchema from_file(const std::string& filename);
    ValidationResult validate(const std::string& body,
                              const std::string& content_type = "application/json") const;
    const std::string& definition() const { return definition_; }

private:
    std::string definition_;
};

} // namespace rest_api
} // namespace ml
#endif
