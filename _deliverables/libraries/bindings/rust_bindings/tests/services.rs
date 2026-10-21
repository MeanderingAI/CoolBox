use coolbox_rs::{SqlDatabase, SqlSchema};
use std::collections::BTreeMap;
use std::sync::Arc;

#[test]
fn sqlite_orm_round_trip() {
    let schema = SqlSchema::parse(
        "model User {\n  id Int @id(auto)\n  name String\n}\n",
    )
    .expect("parse Prisma schema");
    assert_eq!(schema.models().expect("read model names"), vec!["User"]);

    let database = Arc::new(SqlDatabase::new("sqlite").expect("create SQLite database"));
    database.connect(":memory:").expect("connect SQLite");
    database.migrate_up(&schema).expect("run migration");

    let repository = database.repository(&schema, "User").expect("create repository");
    repository
        .create(&BTreeMap::from([("name".to_string(), "Ada".to_string())]))
        .expect("create row");
    let rows = repository.find_all().expect("read rows");
    assert!(rows.contains("\"name\":\"Ada\""), "{rows}");
}

#[test]
fn migration_history_supports_reverting_one_version() {
    use std::fs;
    use std::time::{SystemTime, UNIX_EPOCH};

    let unique = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("system time")
        .as_nanos();
    let directory = std::env::temp_dir().join(format!("coolbox-migrations-{unique}"));
    fs::create_dir_all(&directory).expect("create migration directory");
    fs::write(
        directory.join("0001_create_users.up.sql"),
        "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL);",
    )
    .expect("write up migration");
    fs::write(directory.join("0001_create_users.down.sql"), "DROP TABLE users;")
        .expect("write down migration");
    fs::write(
        directory.join("0002_create_posts.up.sql"),
        "CREATE TABLE posts (id INTEGER PRIMARY KEY, title TEXT NOT NULL);",
    )
    .expect("write up migration");
    fs::write(directory.join("0002_create_posts.down.sql"), "DROP TABLE posts;")
        .expect("write down migration");

    let database = SqlDatabase::new("sqlite").expect("create database");
    database.connect(":memory:").expect("connect database");
    let path = directory.to_str().expect("UTF-8 migration path");
    database.migrate_directory_up(path).expect("migrate up");
    assert_eq!(database.current_migration(path).expect("version"), "0002");
    database
        .migrate_directory_down(path, 1)
        .expect("migrate down one");
    assert_eq!(database.current_migration(path).expect("version"), "0001");

    fs::remove_dir_all(directory).expect("remove migration files");
}
