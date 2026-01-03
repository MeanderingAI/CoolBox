"""
Prisma SQL Library Example (Python)

Demonstrates:
- Parsing Prisma schema files
- Generating CRUD operations
- Database operations with SQLite
- Repository pattern in Python

Note: After namespace reorganization:
- networking.json for JSON
- networking.rest_api for REST API
- ml.sql for SQL/database
- distributed for distributed training
"""

import sys
sys.path.insert(0, '../python_bindings/build/lib.macosx-11.0-arm64-3.9')

import ml_core

def main():
    print("=== Prisma SQL Library Example (Python) ===\n")
    
    # Parse Prisma schema
    print("1. Parsing Prisma schema...")
    schema = ml_core.sql_parse_schema("schema.prisma")
    
    print(f"Provider: {schema.get_provider()}")
    print(f"Models: {len(schema.models())}")
    
    for model in schema.models():
        print(f"  - {model.name} ({len(model.fields)} fields)")
    
    # Get User model
    user_model = schema.get_model("User")
    if not user_model:
        print("User model not found!")
        return
    
    # Generate CRUD operations
    print("\n2. Generating CRUD operations...")
    crud_gen = ml_core.CRUDGenerator(user_model, "sqlite")
    
    print("\nCREATE TABLE:")
    print(crud_gen.generate_create_table())
    
    print("\nSample queries:")
    print(f"Find all: {crud_gen.generate_find_all()}")
    print(f"Find by ID: {crud_gen.generate_find_by_id('1')}")
    
    # Create database
    print("\n3. Creating database...")
    db = ml_core.sql_create_database("sqlite")
    
    if not db.connect("test_python.db"):
        print("Failed to connect!")
        return
    
    # Migrate schema
    print("Migrating schema...")
    migrator = ml_core.Migrator(db, schema)
    
    if migrator.reset():
        print("✓ Migration complete")
    else:
        print("✗ Migration failed")
        return
    
    # Create repository
    print("\n4. Inserting data...")
    user_repo = ml_core.Repository(db, user_model)
    
    # Insert users
    user1 = {
        "email": "alice@example.com",
        "name": "Alice",
        "age": "30",
        "active": "1"
    }
    
    user2 = {
        "email": "bob@example.com",
        "name": "Bob",
        "age": "25",
        "active": "1"
    }
    
    result1 = user_repo.create(user1)
    result2 = user_repo.create(user2)
    
    print(f"✓ Inserted 2 users (last ID: {result2.last_insert_id})")
    
    # Query data
    print("\n5. Querying data...")
    all_users = user_repo.find_all()
    
    print(f"All users ({all_users.size()}):")
    for row in all_users.rows:
        print(f"  ID: {row['id']}, Name: {row['name']}, Email: {row['email']}")
    
    # Find by ID
    user = user_repo.find_by_id("1")
    if not user.empty():
        row = user.get_first()
        print(f"\nUser with ID=1: {row['name']} ({row['email']})")
    
    # Update
    print("\n6. Updating data...")
    result = user_repo.update("2", {"age": "26"})
    print(f"✓ Updated {result.affected_rows} row(s)")
    
    # Query builder
    print("\n7. Advanced query...")
    builder = ml_core.QueryBuilder(ml_core.QueryType.SELECT, "User")
    builder.select(["id", "name", "email"])
    builder.where("active", "1")
    builder.order_by("name", ml_core.OrderDirection.ASC)
    builder.limit(10)
    
    query = builder.build()
    print(f"Query: {query}")
    
    result = db.execute(query)
    print(f"Results: {result.size()} rows")
    
    # Export to JSON
    print("\n8. Export to JSON...")
    all_users = user_repo.find_all()
    print(all_users.to_json())
    
    db.disconnect()
    print("\n✓ Example complete!")

if __name__ == "__main__":
    main()
