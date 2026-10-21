package coolbox

import (
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestSQLRepositoryBindings(t *testing.T) {
	schema, err := ParsePrismaSchema(`
model User {
  id Int @id(auto)
  name String
}
`)
	if err != nil {
		t.Fatal(err)
	}
	defer schema.Close()

	database, err := NewSQLDatabase("sqlite", ":memory:")
	if err != nil {
		t.Fatal(err)
	}
	defer database.Close()
	if err := database.MigrateUp(schema); err != nil {
		t.Fatal(err)
	}
	repository, err := database.Repository(schema, "User")
	if err != nil {
		t.Fatal(err)
	}
	defer repository.Close()
	if _, err := repository.Create(map[string]string{"name": "Ada"}); err != nil {
		t.Fatal(err)
	}
	rows, err := repository.FindAll()
	if err != nil {
		t.Fatal(err)
	}
	if !strings.Contains(string(rows), `"name":"Ada"`) {
		t.Fatalf("unexpected ORM result: %s", rows)
	}
}

func TestVersionedMigrationsCanDowngrade(t *testing.T) {
	directory := t.TempDir()
	writeMigration := func(name, contents string) {
		t.Helper()
		if err := os.WriteFile(filepath.Join(directory, name), []byte(contents), 0600); err != nil {
			t.Fatal(err)
		}
	}
	writeMigration("0001_create_users.up.sql",
		"CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL);\n")
	writeMigration("0001_create_users.down.sql", "DROP TABLE users;\n")
	writeMigration("0002_create_posts.up.sql",
		"CREATE TABLE posts (id INTEGER PRIMARY KEY, title TEXT NOT NULL);\n")
	writeMigration("0002_create_posts.down.sql", "DROP TABLE posts;\n")

	database, err := NewSQLDatabase("sqlite", ":memory:")
	if err != nil {
		t.Fatal(err)
	}
	defer database.Close()
	if err := database.MigrateDirectoryUp(directory); err != nil {
		t.Fatal(err)
	}
	if version, err := database.CurrentMigration(directory); err != nil || version != "0002" {
		t.Fatalf("current migration = %q, error = %v", version, err)
	}
	upPath := filepath.Join(directory, "0002_create_posts.up.sql")
	original, err := os.ReadFile(upPath)
	if err != nil {
		t.Fatal(err)
	}
	writeMigration("0002_create_posts.up.sql",
		"CREATE TABLE posts (id INTEGER PRIMARY KEY, title TEXT, body TEXT);\n")
	if err := database.MigrateDirectoryUp(directory); err == nil {
		t.Fatal("expected edited applied migration to be rejected")
	}
	if err := os.WriteFile(upPath, original, 0600); err != nil {
		t.Fatal(err)
	}
	if err := database.MigrateDirectoryDown(directory, 1); err != nil {
		t.Fatal(err)
	}
	if version, err := database.CurrentMigration(directory); err != nil || version != "0001" {
		t.Fatalf("current migration after downgrade = %q, error = %v", version, err)
	}
}
