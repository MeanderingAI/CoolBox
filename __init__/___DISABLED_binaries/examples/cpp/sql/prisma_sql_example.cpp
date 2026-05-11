/**
 * Prisma SQL Library Example
 * 
 * Demonstrates:
 * - Parsing Prisma schema files
 * - Generating CREATE TABLE statements
 * - Automatic CRUD operation generation
 * - Database operations with SQLite
 * - Repository pattern usage
 */

#include "databases/sql/schema_parser.h"
#include "databases/sql/query_builder.h"
#include "databases/sql/database.h"
#include <iostream>

using namespace ml::sql;
using namespace networking::json;

int main() {
    std::cout << "=== Prisma SQL Library Example ===" << std::endl;
    
    // ========================================
    // 1. Parse Prisma Schema
    // ========================================
    std::cout << "\n1. Parsing Prisma Schema..." << std::endl;
    
    try {
        Schema schema = SchemaParser::parse_file("../schema.prisma");
        
        std::cout << "Provider: " << schema.get_provider() << std::endl;
        std::cout << "Models found: " << schema.models().size() << std::endl;
        
        for (const auto& model : schema.models()) {
            std::cout << "  - " << model.name << " (" 
                     << model.fields.size() << " fields)" << std::endl;
        }
        
        // ========================================
        // 2. Generate SQL for Models
        // ========================================
        std::cout << "\n2. Generating SQL..." << std::endl;
        
        const Model* user_model = schema.get_model("User");
        if (user_model) {
            CRUDGenerator crud_gen(*user_model, "sqlite");
            
            std::cout << "\nCREATE TABLE for User:" << std::endl;
            std::cout << crud_gen.generate_create_table() << std::endl;
            
            std::cout << "\nSample CRUD queries:" << std::endl;
            std::cout << "Find all: " << crud_gen.generate_find_all() << std::endl;
            std::cout << "Find by ID: " << crud_gen.generate_find_by_id("1") << std::endl;
        }
        
        // ========================================
        // 3. Create Database and Migrate
        // ========================================
        std::cout << "\n3. Creating database and migrating schema..." << std::endl;
        
        auto db = Database::create("sqlite");
        if (!db->connect("test.db")) {
            std::cerr << "Failed to connect to database" << std::endl;
            return 1;
        }
        
        Migrator migrator(*db, schema);
        
        std::cout << "Resetting database..." << std::endl;
        if (migrator.reset()) {
            std::cout << "✓ Migration complete" << std::endl;
        } else {
            std::cerr << "✗ Migration failed" << std::endl;
            return 1;
        }
        
        // ========================================
        // 4. Insert Data
        // ========================================
        std::cout << "\n4. Inserting data..." << std::endl;
        
        if (user_model) {
            Repository<void> user_repo(*db, *user_model);
            
            // Create users
            std::map<std::string, std::string> user1 = {
                {"email", "alice@example.com"},
                {"name", "Alice"},
                {"age", "30"},
                {"active", "1"}
            };
            
            std::map<std::string, std::string> user2 = {
                {"email", "bob@example.com"},
                {"name", "Bob"},
                {"age", "25"},
                {"active", "1"}
            };
            
            std::map<std::string, std::string> user3 = {
                {"email", "charlie@example.com"},
                {"name", "Charlie"},
                {"age", "35"},
                {"active", "0"}
            };
            
            auto result1 = user_repo.create(user1);
            auto result2 = user_repo.create(user2);
            auto result3 = user_repo.create(user3);
            
            std::cout << "✓ Inserted 3 users" << std::endl;
            std::cout << "  Last insert ID: " << result3.last_insert_id << std::endl;
        }
        
        // ========================================
        // 5. Query Data
        // ========================================
        std::cout << "\n5. Querying data..." << std::endl;
        
        if (user_model) {
            Repository<void> user_repo(*db, *user_model);
            
            // Find all users
            auto all_users = user_repo.find_all();
            std::cout << "\nAll users (" << all_users.size() << "):" << std::endl;
            for (const auto& row : all_users.rows) {
                std::cout << "  ID: " << row.at("id") 
                         << ", Name: " << row.at("name")
                         << ", Email: " << row.at("email") << std::endl;
            }
            
            // Find by ID
            auto user = user_repo.find_by_id("1");
            if (!user.empty()) {
                std::cout << "\nUser with ID=1:" << std::endl;
                auto row = user.get_first();
                std::cout << "  Name: " << row["name"] << std::endl;
                std::cout << "  Email: " << row["email"] << std::endl;
            }
            
            // Find with conditions
            auto active_users = user_repo.find_where({{"active", "1"}});
            std::cout << "\nActive users: " << active_users.size() << std::endl;
        }
        
        // ========================================
        // 6. Update Data
        // ========================================
        std::cout << "\n6. Updating data..." << std::endl;
        
        if (user_model) {
            Repository<void> user_repo(*db, *user_model);
            
            // Update user
            auto result = user_repo.update("2", {{"age", "26"}});
            std::cout << "✓ Updated " << result.affected_rows << " row(s)" << std::endl;
            
            // Verify update
            auto updated_user = user_repo.find_by_id("2");
            if (!updated_user.empty()) {
                auto row = updated_user.get_first();
                std::cout << "  New age: " << row["age"] << std::endl;
            }
        }
        
        // ========================================
        // 7. Advanced Query Builder
        // ========================================
        std::cout << "\n7. Advanced queries..." << std::endl;
        
        QueryBuilder builder(QueryType::SELECT, "User");
        builder.select({"id", "name", "email"})
               .where("active", "1")
               .order_by("name", OrderDirection::ASC)
               .limit(10);
        
        std::string query = builder.build();
        std::cout << "Query: " << query << std::endl;
        
        auto result = db->execute(query);
        std::cout << "Results: " << result.size() << " rows" << std::endl;
        
        // ========================================
        // 8. Delete Data
        // ========================================
        std::cout << "\n8. Deleting data..." << std::endl;
        
        if (user_model) {
            Repository<void> user_repo(*db, *user_model);
            
            // Delete inactive users
            auto result = user_repo.remove_where({{"active", "0"}});
            std::cout << "✓ Deleted " << result.affected_rows << " inactive user(s)" << std::endl;
            
            // Count remaining
            auto remaining = user_repo.find_all();
            std::cout << "  Remaining users: " << remaining.size() << std::endl;
        }
        
        // ========================================
        // 9. Work with Other Models
        // ========================================
        std::cout << "\n9. Working with Product model..." << std::endl;
        
        const Model* product_model = schema.get_model("Product");
        if (product_model) {
            Repository<void> product_repo(*db, *product_model);
            
            // Create products
            product_repo.create({
                {"name", "Laptop"},
                {"price", "999.99"},
                {"stock", "10"},
                {"category", "Electronics"},
                {"sku", "LAP001"}
            });
            
            product_repo.create({
                {"name", "Mouse"},
                {"price", "29.99"},
                {"stock", "50"},
                {"category", "Electronics"},
                {"sku", "MOU001"}
            });
            
            // Query products
            auto products = product_repo.find_all();
            std::cout << "Products: " << products.size() << std::endl;
            for (const auto& row : products.rows) {
                std::cout << "  - " << row.at("name") 
                         << " ($" << row.at("price") << ")" << std::endl;
            }
        }
        
        // ========================================
        // 10. Export to JSON
        // ========================================
        std::cout << "\n10. Export data to JSON..." << std::endl;
        
        if (user_model) {
            Repository<void> user_repo(*db, *user_model);
            auto all_users = user_repo.find_all();
            
            std::cout << all_users.to_json() << std::endl;
        }
        
        db->disconnect();
        std::cout << "\n✓ Example complete!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
