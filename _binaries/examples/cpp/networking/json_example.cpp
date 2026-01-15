/**
 * JSON Library Example
 * 
 * Demonstrates:
 * - Creating JSON values
 * - Building JSON objects
 * - Parsing JSON strings
 * - Working with arrays
 */

#include "dataformats/json/json.h"
#include <iostream>

using namespace networking::json;

int main() {
    std::cout << "=== JSON Library Example ===" << std::endl;
    
    // ========================================
    // Example 1: Building JSON with Builder
    // ========================================
    std::cout << "\n1. Building JSON with Builder:" << std::endl;
    
    Builder user_builder;
    user_builder.add("id", 123)
               .add("name", "Alice")
               .add("email", "alice@example.com")
               .add("active", true);
    
    Value user = user_builder.build();
    std::cout << user.to_string() << std::endl;
    
    // ========================================
    // Example 2: Creating nested JSON
    // ========================================
    std::cout << "\n2. Creating nested JSON:" << std::endl;
    
    Builder address_builder;
    address_builder.add("street", "123 Main St")
                  .add("city", "Boston")
                  .add("zip", "02101");
    
    Builder person_builder;
    person_builder.add("name", "Bob")
                 .add("age", 30)
                 .add("address", address_builder.build());
    
    Value person = person_builder.build();
    std::cout << person.to_string() << std::endl;
    
    // ========================================
    // Example 3: Working with arrays
    // ========================================
    std::cout << "\n3. Working with arrays:" << std::endl;
    
    Array tags;
    tags.push_back(Value("python"));
    tags.push_back(Value("machine learning"));
    tags.push_back(Value("rest api"));
    
    Builder project_builder;
    project_builder.add("name", "ML Toolbox")
                  .add("version", "0.2.0")
                  .add("tags", Value(tags));
    
    Value project = project_builder.build();
    std::cout << project.to_string() << std::endl;
    
    // ========================================
    // Example 4: Parsing JSON strings
    // ========================================
    std::cout << "\n4. Parsing JSON strings:" << std::endl;
    
    std::string json_str = R"({
        "model": "neural_network",
        "layers": [64, 128, 64],
        "activation": "relu",
        "dropout": 0.5,
        "trained": true
    })";
    
    Parser parser(json_str);
    Value config = parser.parse();
    std::cout << "Parsed: " << config.to_string() << std::endl;
    
    // Access values
    if (config.is_object()) {
        Object& obj = config.as_object();
        std::cout << "Model: " << obj.get("model").as_string() << std::endl;
        std::cout << "Dropout: " << obj.get("dropout").as_number() << std::endl;
        std::cout << "Trained: " << (obj.get("trained").as_bool() ? "Yes" : "No") << std::endl;
    }
    
    // ========================================
    // Example 5: Array of objects
    // ========================================
    std::cout << "\n5. Array of objects:" << std::endl;
    
    Array users;
    
    Builder user1;
    user1.add("id", 1).add("name", "Alice");
    users.push_back(user1.build());
    
    Builder user2;
    user2.add("id", 2).add("name", "Bob");
    users.push_back(user2.build());
    
    Builder user3;
    user3.add("id", 3).add("name", "Charlie");
    users.push_back(user3.build());
    
    Builder response_builder;
    response_builder.add("count", 3)
                   .add("users", Value(users));
    
    Value response = response_builder.build();
    std::cout << response.to_string() << std::endl;
    
    // ========================================
    // Example 6: Simple utilities
    // ========================================
    std::cout << "\n6. Simple utilities (backward compatibility):" << std::endl;
    
    std::map<std::string, std::string> data = {
        {"key1", "value1"},
        {"key2", "value2"}
    };
    std::string simple_json = simple::encode(data);
    std::cout << "Encoded: " << simple_json << std::endl;
    
    auto decoded = simple::decode(simple_json);
    std::cout << "Decoded keys: ";
    for (const auto& [k, v] : decoded) {
        std::cout << k << " ";
    }
    std::cout << std::endl;
    
    std::vector<std::string> items = {"item1", "item2", "item3"};
    std::string array_json = simple::encode_array(items);
    std::cout << "Array: " << array_json << std::endl;
    
    return 0;
}
