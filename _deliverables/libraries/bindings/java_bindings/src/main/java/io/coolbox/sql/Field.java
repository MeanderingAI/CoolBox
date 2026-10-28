package io.coolbox.sql;

/** Immutable Prisma field metadata, equivalent to Python {@code sql.Field}. */
public record Field(String name, FieldType type, boolean optional, boolean array, boolean id,
                    boolean unique, boolean autoIncrement, String defaultValue, boolean relation) {
    public String toSqlType() { return toSqlType("sqlite"); }
    public String toSqlType(String provider) {
        String db=provider.toLowerCase(java.util.Locale.ROOT);
        return switch(type){
            case INT -> db.equals("mysql")||db.equals("mariadb")?"INT":"INTEGER";
            case BIGINT -> "BIGINT";
            case STRING -> db.equals("postgresql")||db.equals("mysql")||db.equals("mariadb")?"VARCHAR(255)":"TEXT";
            case BOOLEAN -> db.equals("sqlite")?"INTEGER":"BOOLEAN";
            case FLOAT -> db.equals("sqlite")?"REAL":db.equals("postgresql")?"REAL":"FLOAT";
            case DOUBLE -> db.equals("sqlite")?"REAL":db.equals("postgresql")?"DOUBLE PRECISION":"DOUBLE";
            case DATETIME -> db.equals("sqlite")?"TEXT":db.equals("postgresql")?"TIMESTAMP":"DATETIME";
            case JSON -> db.equals("sqlite")?"TEXT":db.equals("postgresql")?"JSONB":"JSON";
            case BYTES -> db.equals("postgresql")?"BYTEA":db.equals("mysql")||db.equals("mariadb")?"BLOB":db.equals("sqlite")?"BLOB":"TEXT";
            case DECIMAL -> db.equals("sqlite")?"REAL":db.equals("mysql")||db.equals("mariadb")?"DECIMAL(10, 2)":"DECIMAL";
        };
    }
}
