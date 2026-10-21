import ml_core
import pytest


def test_sqlite_repository_round_trip(tmp_path):
    schema = ml_core.sql.SchemaParser(
        "model User {\n  id Int @id(auto)\n  name String\n}\n"
    ).parse()
    database = ml_core.sql.Database.create("sqlite")
    assert database.connect(":memory:")
    migrator = ml_core.sql.Migrator(database, schema)
    assert migrator.migrate_up()

    repository = ml_core.sql.Repository(database, schema.get_model("User"))
    repository.create({"name": "Ada"})
    result = repository.find_all()
    assert result.rows == [{"id": "1", "name": "Ada"}]

    migrations = tmp_path / "migrations"
    migrations.mkdir()
    (migrations / "0001_create_items.up.sql").write_text(
        "CREATE TABLE items (id INTEGER PRIMARY KEY);"
    )
    (migrations / "0001_create_items.down.sql").write_text("DROP TABLE items;")
    runner = ml_core.sql.MigrationRunner(database, str(migrations))
    runner.migrate_up()
    assert runner.current_version() == "0001"
    up_file = migrations / "0001_create_items.up.sql"
    original = up_file.read_text()
    up_file.write_text("CREATE TABLE items (id INTEGER PRIMARY KEY, name TEXT);")
    with pytest.raises(RuntimeError, match="modified"):
        runner.migrate_up()
    up_file.write_text(original)
    runner.migrate_down()
    assert runner.current_version() == ""
