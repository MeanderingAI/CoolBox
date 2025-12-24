# Prisma SQL Library

A C++ library for parsing Prisma schema files and automatically generating SQL CRUD operations. Features include:

## Features

### 🔍 Schema Parsing
- Parse `.prisma` schema files
- Extract model definitions, fields, and attributes
- Support for field types: Int, String, Boolean, Float, DateTime, JSON, etc.
- Handle field attributes: `@id`, `@unique`, `@default`, etc.

### 🏗️ SQL Generation
- Automatic CREATE TABLE statements
- Type-safe field definitions
- Primary keys and constraints
- Support for SQLite, PostgreSQL, MySQL

### 🔄 CRUD Operations
- **Create**: INSERT queries with type safety
- **Read**: SELECT with WHERE conditions, ordering, pagination
- **Update**: UPDATE with conditional filtering
- **Delete**: DELETE with WHERE clauses

### 🗄️ Database Support
- **SQLite**: Full support with prepared statements
- **PostgreSQL**: Coming soon
- **MySQL**: Coming soon

### 🎯 Repository Pattern
- ORM-like interface for models
- Type-safe CRUD operations
- Transaction support
- Batch operations

### 🔧 Query Builder
- Fluent API for building queries
- WHERE conditions with operators (=, !=, >, <, LIKE, IN, BETWEEN)
- JOIN support
- ORDER BY and LIMIT/OFFSET

## Quick Start

### C++ Example

```cpp
#include "sql/schema_parser.h"
#include "sql/database.h"

using namespace ml::sql;

// Parse Prisma schema
Schema schema = SchemaParser::parse_file("schema.prisma");

// Create database
auto db = Database::create("sqlite");
db->connect("myapp.db");

// Migrate (create tables)
Migrator migrator(*db, schema);
migrator.migrate_up();

// Get model and create repository
const Model* user_model = schema.get_model("User");
Repository<void> users(*db, *user_model);

// Create
users.create({
    {"email", "alice@example.com"},
    {"name", "Alice"},
    {"age", "30"}
});

// Read
auto all_users = users.find_all();
auto user = users.find_by_id("1");
auto active = users.find_where({{"active", "1"}});

// Update
users.update("1", {{"age", "31"}});

// Delete
users.remove("1");
```

### Python Example

```python
import ml_core

# Parse schema
schema = ml_core.sql_parse_schema("schema.prisma")

# Create database
db = ml_core.sql_create_database("sqlite")
db.connect("myapp.db")

# Migrate
migrator = ml_core.Migrator(db, schema)
migrator.migrate_up()

# Get repository
user_model = schema.get_model("User")
users = ml_core.Repository(db, user_model)

# CRUD operations
users.create({"email": "alice@example.com", "name": "Alice"})
all_users = users.find_all()
user = users.find_by_id("1")
users.update("1", {"age": "31"})
users.remove("1")
```

## Prisma Schema Example

```prisma
datasource db {
  provider = "sqlite"
  url      = "file:./dev.db"
}

model User {
  id        Int      @id @default(auto)
  email     String   @unique
  name      String?
  age       Int
  active    Boolean  @default(true)
  createdAt DateTime @default(now)
}

model Post {
  id        Int      @id @default(auto)
  title     String
  content   String?
  published Boolean  @default(false)
  authorId  Int
  createdAt DateTime @default(now)
}
```

## Generated SQL

From the User model above, the library generates:

```sql
CREATE TABLE User (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  email TEXT NOT NULL UNIQUE,
  name TEXT,
  age INTEGER NOT NULL,
  active INTEGER NOT NULL DEFAULT 1,
  createdAt TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
)
```

## Query Builder

```cpp
QueryBuilder builder(QueryType::SELECT, "User");
builder.select({"id", "name", "email"})
       .where("active", Operator::EQUALS, "1")
       .where("age", Operator::GREATER_THAN, "18")
       .order_by("name", OrderDirection::ASC)
       .limit(10)
       .offset(0);

std::string query = builder.build();
// SELECT id, name, email FROM User 
// WHERE active = '1' AND age > '18' 
// ORDER BY name ASC LIMIT 10 OFFSET 0
```

## CRUD Generator

```cpp
CRUDGenerator crud(user_model, "sqlite");

// Generate all operations
crud.generate_create_table();
crud.generate_find_all();
crud.generate_find_by_id("?");
crud.generate_insert(data);
crud.generate_update(id, data);
crud.generate_delete(id);
```

## Repository Pattern

```cpp
Repository<void> repo(db, model);

// Find operations
auto all = repo.find_all();
auto one = repo.find_by_id("123");
auto filtered = repo.find_where({{"status", "active"}});

// Create
auto result = repo.create({{"name", "Alice"}});
long long id = result.last_insert_id;

// Update
repo.update("123", {{"name", "Bob"}});
repo.update_where({{"status", "pending"}}, {{"status", "active"}});

// Delete
repo.remove("123");
repo.remove_where({{"status", "deleted"}});

// Schema
repo.create_table();
repo.drop_table();
```

## Transactions

```cpp
db.begin_transaction();

try {
    users.create({{"name", "Alice"}});
    posts.create({{"title", "Hello", "authorId", "1"}});
    db.commit();
} catch (...) {
    db.rollback();
}
```

## Migration

```cpp
Migrator migrator(db, schema);

// Create all tables
migrator.migrate_up();

// Drop all tables
migrator.migrate_down();

// Reset (drop + create)
migrator.reset();

// Generate SQL script
std::string sql = migrator.generate_migration_sql();
```

## Field Types

| Prisma Type | SQLite | PostgreSQL | MySQL |
|------------|---------|-----------|-------|
| Int | INTEGER | INTEGER | INT |
| BigInt | INTEGER | BIGINT | BIGINT |
| String | TEXT | VARCHAR(255) | VARCHAR(255) |
| Boolean | INTEGER | BOOLEAN | BOOLEAN |
| Float | REAL | REAL | FLOAT |
| Double | REAL | DOUBLE PRECISION | DOUBLE |
| DateTime | TEXT | TIMESTAMP | DATETIME |
| JSON | TEXT | JSONB | JSON |
| Bytes | BLOB | BYTEA | BLOB |
| Decimal | REAL | DECIMAL | DECIMAL |

## Field Attributes

- `@id` - Primary key
- `@default(auto)` - Auto-increment
- `@unique` - Unique constraint
- `@default(value)` - Default value
- `?` - Optional (nullable)
- `[]` - Array type

## API Reference

### Classes

- **Schema**: Parsed Prisma schema
- **Model**: Model definition with fields
- **Field**: Field definition with type and attributes
- **SchemaParser**: Parse .prisma files
- **QueryBuilder**: Build SQL queries fluently
- **CRUDGenerator**: Generate CRUD SQL from models
- **Database**: Database connection interface
- **SQLiteDatabase**: SQLite implementation
- **Repository<T>**: ORM-like model interface
- **Migrator**: Schema migration tool

### Enums

- **FieldType**: INT, STRING, BOOLEAN, etc.
- **QueryType**: SELECT, INSERT, UPDATE, DELETE
- **Operator**: EQUALS, GREATER_THAN, LIKE, IN, etc.
- **OrderDirection**: ASC, DESC

## Examples

See:
- [prisma_sql_example.cpp](cpp/prisma_sql_example.cpp) - Full C++ demonstration
- [prisma_sql_example.py](prisma_sql_example.py) - Python usage
- [schema.prisma](schema.prisma) - Example schema

Run examples:
```bash
cd examples/cpp
make run-prisma
```

## Building

The library is built automatically with the main project:

```bash
./clean&build.sh
```

Dependencies:
- C++17 compiler
- SQLite3
- JSON library (included)

## Integration

### CMake

```cmake
find_package(SQLite3 REQUIRED)

add_executable(myapp main.cpp)
target_link_libraries(myapp sql json SQLite::SQLite3)
```

### Python

```python
import ml_core

# All SQL classes available under ml_core namespace
schema = ml_core.sql_parse_schema("schema.prisma")
db = ml_core.sql_create_database("sqlite")
```

## Limitations

- Relationships (foreign keys) are parsed but not yet auto-generated
- PostgreSQL and MySQL implementations pending
- Migrations are create/drop only (no ALTER TABLE yet)
- No schema diffing or incremental migrations

## Roadmap

- [ ] Foreign key generation
- [ ] PostgreSQL support
- [ ] MySQL support
- [ ] ALTER TABLE migrations
- [ ] Schema versioning
- [ ] Migration rollback
- [ ] Index generation
- [ ] Enum support
- [ ] Composite primary keys
- [ ] View support

## License

Same as main project (see [LICENSE](../../LICENSE))
