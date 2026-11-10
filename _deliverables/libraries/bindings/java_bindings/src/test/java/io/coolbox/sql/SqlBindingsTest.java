package io.coolbox.sql;

import org.junit.jupiter.api.Test;
import java.nio.file.Files;
import java.util.Map;
import static org.junit.jupiter.api.Assertions.*;

class SqlBindingsTest {
    @Test void parsesSchemaAndRunsSqliteCrudAndMigrations() throws Exception {
        var directory=Files.createTempDirectory("coolbox-sql");
        var schema=new SchemaParser("datasource db {\n provider = \"sqlite\"\n url = \"file:test.db\"\n}\nmodel User {\n id Int @id\n name String\n}").parse();
        assertEquals("sqlite",schema.getProvider());
        var model=schema.getModel("User");
        assertEquals(2,model.fields().size());
        assertEquals("INTEGER",model.fields().get(0).toSqlType());
        assertEquals(1,model.getPrimaryKeys().size());
        var database=Database.create("sqlite");
        try(database;schema){
            database.connect(directory.resolve("test.db").toString());
            assertTrue(database.isConnected());
            try(var migrator=new Migrator(database,schema)){
                assertTrue(migrator.generateMigrationSql().contains("CREATE TABLE"));
                migrator.migrateUp();
            }
            var migrations=Files.createDirectories(directory.resolve("migrations"));
            try(var runner=new MigrationRunner(database,migrations.toString())){runner.migrateUp();assertEquals("",runner.currentVersion());}
            try(var inserted=database.execute("INSERT INTO User (name) VALUES (?)","Ada")){assertEquals(1,inserted.affectedRows());}
            try(var selected=database.execute("SELECT name FROM User")){assertEquals("Ada",selected.getSingle().get("name"));}
            try(var repository=new Repository(database,model)){
                try(var created=repository.create(Map.of("name","Grace"))){assertEquals(1,created.affectedRows());}
                try(var rows=repository.findAll()){assertEquals(2,rows.size());}
                try(var byName=repository.findWhere(Map.of("name","Ada"))){assertEquals(1,byName.size());}
            }
            assertThrows(Exception.class,()->database.execute("SELECT * FROM missing_table"));
            assertTrue(database.beginTransaction());assertTrue(database.rollback());
            database.disconnect();assertFalse(database.isConnected());
        }
    }
}
