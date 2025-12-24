#include <gtest/gtest.h>
#include "databases/sql/schema_parser.h"
#include "databases/sql/query_builder.h"
#include "databases/sql/database.h"
#include <fstream>
#include <sstream>

using namespace ml::sql;

// ============================================================================
// SchemaParser Tests
// ============================================================================

TEST(SchemaParserTest, ParseSimpleModel) {
    std::string schema = R"(
        model User {
            id    Int    @id @default(autoincrement())
            name  String
            email String @unique
        }
    )";
    
    SchemaParser parser;
    auto models = parser.parse(schema);
    
    EXPECT_EQ(models.size(), 1u);
    EXPECT_EQ(models[0].name, "User");
    EXPECT_EQ(models[0].fields.size(), 3u);
}

TEST(SchemaParserTest, ParseFieldTypes) {
    std::string schema = R"(
        model TestTypes {
            id      Int
            name    String
            score   Float
            active  Boolean
            data    DateTime
        }
    )";
    
    SchemaParser parser;
    auto models = parser.parse(schema);
    
    ASSERT_EQ(models.size(), 1u);
    EXPECT_EQ(models[0].fields.size(), 5u);
    
    EXPECT_EQ(models[0].fields[0].type, "Int");
    EXPECT_EQ(models[0].fields[1].type, "String");
    EXPECT_EQ(models[0].fields[2].type, "Float");
    EXPECT_EQ(models[0].fields[3].type, "Boolean");
    EXPECT_EQ(models[0].fields[4].type, "DateTime");
}

TEST(SchemaParserTest, ParseAttributes) {
    std::string schema = R"(
        model User {
            id    Int    @id @default(autoincrement())
            email String @unique
        }
    )";
    
    SchemaParser parser;
    auto models = parser.parse(schema);
    
    ASSERT_EQ(models.size(), 1u);
    
    // Check id field has attributes
    const auto& idField = models[0].fields[0];
    EXPECT_FALSE(idField.attributes.empty());
    EXPECT_TRUE(std::find(idField.attributes.begin(), idField.attributes.end(), "@id") 
                != idField.attributes.end());
    
    // Check email field has unique attribute
    const auto& emailField = models[0].fields[1];
    EXPECT_TRUE(std::find(emailField.attributes.begin(), emailField.attributes.end(), "@unique") 
                != emailField.attributes.end());
}

TEST(SchemaParserTest, ParseOptionalFields) {
    std::string schema = R"(
        model User {
            id       Int     @id
            name     String
            nickname String?
        }
    )";
    
    SchemaParser parser;
    auto models = parser.parse(schema);
    
    ASSERT_EQ(models.size(), 1u);
    EXPECT_EQ(models[0].fields.size(), 3u);
    
    EXPECT_FALSE(models[0].fields[0].is_optional);
    EXPECT_FALSE(models[0].fields[1].is_optional);
    EXPECT_TRUE(models[0].fields[2].is_optional);
}

TEST(SchemaParserTest, ParseMultipleModels) {
    std::string schema = R"(
        model User {
            id   Int    @id
            name String
        }
        
        model Post {
            id      Int    @id
            title   String
            content String
        }
    )";
    
    SchemaParser parser;
    auto models = parser.parse(schema);
    
    EXPECT_EQ(models.size(), 2u);
    EXPECT_EQ(models[0].name, "User");
    EXPECT_EQ(models[1].name, "Post");
}

TEST(SchemaParserTest, ParseRelations) {
    std::string schema = R"(
        model User {
            id    Int    @id
            posts Post[]
        }
        
        model Post {
            id       Int  @id
            authorId Int
            author   User @relation(fields: [authorId], references: [id])
        }
    )";
    
    SchemaParser parser;
    auto models = parser.parse(schema);
    
    EXPECT_EQ(models.size(), 2u);
    
    // Check User has Post[] field
    bool hasPostsField = false;
    for (const auto& field : models[0].fields) {
        if (field.name == "posts" && field.type == "Post[]") {
            hasPostsField = true;
            break;
        }
    }
    EXPECT_TRUE(hasPostsField);
}

TEST(SchemaParserTest, EmptySchema) {
    std::string schema = "";
    
    SchemaParser parser;
    auto models = parser.parse(schema);
    
    EXPECT_TRUE(models.empty());
}

// ============================================================================
// QueryBuilder Tests
// ============================================================================

TEST(QueryBuilderTest, CreateTable) {
    Model model;
    model.name = "User";
    
    Field idField;
    idField.name = "id";
    idField.type = "Int";
    idField.attributes.push_back("@id");
    
    Field nameField;
    nameField.name = "name";
    nameField.type = "String";
    
    model.fields.push_back(idField);
    model.fields.push_back(nameField);
    
    QueryBuilder builder;
    std::string sql = builder.create_table(model);
    
    EXPECT_TRUE(sql.find("CREATE TABLE") != std::string::npos);
    EXPECT_TRUE(sql.find("User") != std::string::npos);
    EXPECT_TRUE(sql.find("id") != std::string::npos);
    EXPECT_TRUE(sql.find("name") != std::string::npos);
    EXPECT_TRUE(sql.find("PRIMARY KEY") != std::string::npos);
}

TEST(QueryBuilderTest, InsertQuery) {
    QueryBuilder builder;
    
    std::map<std::string, std::string> values;
    values["name"] = "John Doe";
    values["email"] = "john@example.com";
    
    std::string sql = builder.insert("User", values);
    
    EXPECT_TRUE(sql.find("INSERT INTO") != std::string::npos);
    EXPECT_TRUE(sql.find("User") != std::string::npos);
    EXPECT_TRUE(sql.find("name") != std::string::npos);
    EXPECT_TRUE(sql.find("email") != std::string::npos);
}

TEST(QueryBuilderTest, SelectQuery) {
    QueryBuilder builder;
    
    std::string sql = builder.select("User");
    
    EXPECT_TRUE(sql.find("SELECT") != std::string::npos);
    EXPECT_TRUE(sql.find("FROM User") != std::string::npos);
    EXPECT_TRUE(sql.find("*") != std::string::npos);
}

TEST(QueryBuilderTest, SelectWithColumns) {
    QueryBuilder builder;
    
    std::vector<std::string> columns = {"id", "name", "email"};
    std::string sql = builder.select("User", columns);
    
    EXPECT_TRUE(sql.find("SELECT") != std::string::npos);
    EXPECT_TRUE(sql.find("id") != std::string::npos);
    EXPECT_TRUE(sql.find("name") != std::string::npos);
    EXPECT_TRUE(sql.find("email") != std::string::npos);
}

TEST(QueryBuilderTest, SelectWithWhere) {
    QueryBuilder builder;
    
    std::string sql = builder.select("User", {}, "id = 1");
    
    EXPECT_TRUE(sql.find("SELECT") != std::string::npos);
    EXPECT_TRUE(sql.find("WHERE") != std::string::npos);
    EXPECT_TRUE(sql.find("id = 1") != std::string::npos);
}

TEST(QueryBuilderTest, UpdateQuery) {
    QueryBuilder builder;
    
    std::map<std::string, std::string> values;
    values["name"] = "Jane Doe";
    values["email"] = "jane@example.com";
    
    std::string sql = builder.update("User", values, "id = 1");
    
    EXPECT_TRUE(sql.find("UPDATE") != std::string::npos);
    EXPECT_TRUE(sql.find("User") != std::string::npos);
    EXPECT_TRUE(sql.find("SET") != std::string::npos);
    EXPECT_TRUE(sql.find("WHERE") != std::string::npos);
    EXPECT_TRUE(sql.find("id = 1") != std::string::npos);
}

TEST(QueryBuilderTest, DeleteQuery) {
    QueryBuilder builder;
    
    std::string sql = builder.delete_from("User", "id = 1");
    
    EXPECT_TRUE(sql.find("DELETE FROM") != std::string::npos);
    EXPECT_TRUE(sql.find("User") != std::string::npos);
    EXPECT_TRUE(sql.find("WHERE") != std::string::npos);
    EXPECT_TRUE(sql.find("id = 1") != std::string::npos);
}

TEST(QueryBuilderTest, SelectWithOrderBy) {
    QueryBuilder builder;
    
    std::string sql = builder.select("User", {}, "", "name ASC");
    
    EXPECT_TRUE(sql.find("ORDER BY") != std::string::npos);
    EXPECT_TRUE(sql.find("name ASC") != std::string::npos);
}

TEST(QueryBuilderTest, SelectWithLimit) {
    QueryBuilder builder;
    
    std::string sql = builder.select("User", {}, "", "", 10);
    
    EXPECT_TRUE(sql.find("LIMIT") != std::string::npos);
    EXPECT_TRUE(sql.find("10") != std::string::npos);
}

TEST(QueryBuilderTest, ComplexSelectQuery) {
    QueryBuilder builder;
    
    std::vector<std::string> columns = {"id", "name"};
    std::string sql = builder.select("User", columns, "age > 18", "name DESC", 5);
    
    EXPECT_TRUE(sql.find("SELECT") != std::string::npos);
    EXPECT_TRUE(sql.find("id") != std::string::npos);
    EXPECT_TRUE(sql.find("name") != std::string::npos);
    EXPECT_TRUE(sql.find("WHERE age > 18") != std::string::npos);
    EXPECT_TRUE(sql.find("ORDER BY name DESC") != std::string::npos);
    EXPECT_TRUE(sql.find("LIMIT 5") != std::string::npos);
}

// ============================================================================
// Database Tests
// ============================================================================

TEST(DatabaseTest, OpenInMemory) {
    Database db(":memory:");
    EXPECT_TRUE(db.is_open());
}

TEST(DatabaseTest, CreateTable) {
    Database db(":memory:");
    
    std::string sql = R"(
        CREATE TABLE users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            email TEXT UNIQUE
        )
    )";
    
    EXPECT_TRUE(db.execute(sql));
}

TEST(DatabaseTest, InsertAndSelect) {
    Database db(":memory:");
    
    // Create table
    db.execute(R"(
        CREATE TABLE users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT,
            email TEXT
        )
    )");
    
    // Insert data
    db.execute("INSERT INTO users (name, email) VALUES ('Alice', 'alice@test.com')");
    db.execute("INSERT INTO users (name, email) VALUES ('Bob', 'bob@test.com')");
    
    // Select data
    auto results = db.query("SELECT * FROM users");
    
    EXPECT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0]["name"], "Alice");
    EXPECT_EQ(results[1]["name"], "Bob");
}

TEST(DatabaseTest, PreparedStatement) {
    Database db(":memory:");
    
    db.execute(R"(
        CREATE TABLE users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT,
            age INTEGER
        )
    )");
    
    // Insert using prepared statement
    std::string insertSql = "INSERT INTO users (name, age) VALUES (?, ?)";
    EXPECT_TRUE(db.execute_prepared(insertSql, {"Charlie", "30"}));
    
    // Query
    auto results = db.query("SELECT * FROM users WHERE name = 'Charlie'");
    
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0]["name"], "Charlie");
    EXPECT_EQ(results[0]["age"], "30");
}

TEST(DatabaseTest, Transaction) {
    Database db(":memory:");
    
    db.execute("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)");
    
    db.begin_transaction();
    db.execute("INSERT INTO users (id, name) VALUES (1, 'Alice')");
    db.execute("INSERT INTO users (id, name) VALUES (2, 'Bob')");
    db.commit();
    
    auto results = db.query("SELECT COUNT(*) as count FROM users");
    EXPECT_EQ(results[0]["count"], "2");
}

TEST(DatabaseTest, Rollback) {
    Database db(":memory:");
    
    db.execute("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)");
    db.execute("INSERT INTO users (id, name) VALUES (1, 'Alice')");
    
    db.begin_transaction();
    db.execute("INSERT INTO users (id, name) VALUES (2, 'Bob')");
    db.rollback();
    
    auto results = db.query("SELECT COUNT(*) as count FROM users");
    EXPECT_EQ(results[0]["count"], "1");
}

TEST(DatabaseTest, UpdateRecord) {
    Database db(":memory:");
    
    db.execute("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, email TEXT)");
    db.execute("INSERT INTO users (id, name, email) VALUES (1, 'Alice', 'alice@old.com')");
    
    // Update
    db.execute("UPDATE users SET email = 'alice@new.com' WHERE id = 1");
    
    auto results = db.query("SELECT email FROM users WHERE id = 1");
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0]["email"], "alice@new.com");
}

TEST(DatabaseTest, DeleteRecord) {
    Database db(":memory:");
    
    db.execute("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)");
    db.execute("INSERT INTO users (id, name) VALUES (1, 'Alice')");
    db.execute("INSERT INTO users (id, name) VALUES (2, 'Bob')");
    
    // Delete
    db.execute("DELETE FROM users WHERE id = 1");
    
    auto results = db.query("SELECT COUNT(*) as count FROM users");
    EXPECT_EQ(results[0]["count"], "1");
    
    results = db.query("SELECT name FROM users");
    EXPECT_EQ(results[0]["name"], "Bob");
}

TEST(DatabaseTest, QueryWithJoin) {
    Database db(":memory:");
    
    db.execute(R"(
        CREATE TABLE users (
            id INTEGER PRIMARY KEY,
            name TEXT
        )
    )");
    
    db.execute(R"(
        CREATE TABLE posts (
            id INTEGER PRIMARY KEY,
            user_id INTEGER,
            title TEXT,
            FOREIGN KEY(user_id) REFERENCES users(id)
        )
    )");
    
    db.execute("INSERT INTO users (id, name) VALUES (1, 'Alice')");
    db.execute("INSERT INTO posts (id, user_id, title) VALUES (1, 1, 'Hello World')");
    
    auto results = db.query(R"(
        SELECT users.name, posts.title 
        FROM posts 
        JOIN users ON posts.user_id = users.id
    )");
    
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0]["name"], "Alice");
    EXPECT_EQ(results[0]["title"], "Hello World");
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(SqlIntegrationTest, SchemaToDatabase) {
    std::string schema = R"(
        model User {
            id    Int    @id @default(autoincrement())
            name  String
            email String @unique
        }
    )";
    
    SchemaParser parser;
    auto models = parser.parse(schema);
    
    ASSERT_EQ(models.size(), 1u);
    
    QueryBuilder builder;
    std::string createTableSql = builder.create_table(models[0]);
    
    Database db(":memory:");
    EXPECT_TRUE(db.execute(createTableSql));
    
    // Insert using query builder
    std::map<std::string, std::string> values;
    values["name"] = "John Doe";
    values["email"] = "john@example.com";
    
    std::string insertSql = builder.insert("User", values);
    EXPECT_TRUE(db.execute(insertSql));
    
    // Query
    auto results = db.query("SELECT * FROM User");
    EXPECT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0]["name"], "John Doe");
}

TEST(SqlIntegrationTest, CRUDOperations) {
    Database db(":memory:");
    QueryBuilder builder;
    
    // Create
    db.execute(R"(
        CREATE TABLE products (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT,
            price REAL,
            in_stock INTEGER
        )
    )");
    
    // Insert
    std::map<std::string, std::string> values;
    values["name"] = "Laptop";
    values["price"] = "999.99";
    values["in_stock"] = "1";
    
    std::string insertSql = builder.insert("products", values);
    db.execute(insertSql);
    
    // Read
    std::string selectSql = builder.select("products");
    auto results = db.query(selectSql);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0]["name"], "Laptop");
    
    // Update
    std::map<std::string, std::string> updateValues;
    updateValues["price"] = "899.99";
    std::string updateSql = builder.update("products", updateValues, "name = 'Laptop'");
    db.execute(updateSql);
    
    results = db.query("SELECT price FROM products WHERE name = 'Laptop'");
    EXPECT_EQ(results[0]["price"], "899.99");
    
    // Delete
    std::string deleteSql = builder.delete_from("products", "name = 'Laptop'");
    db.execute(deleteSql);
    
    results = db.query("SELECT COUNT(*) as count FROM products");
    EXPECT_EQ(results[0]["count"], "0");
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
