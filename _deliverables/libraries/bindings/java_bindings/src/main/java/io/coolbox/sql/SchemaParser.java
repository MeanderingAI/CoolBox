package io.coolbox.sql;

import com.sun.jna.Pointer;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.NativeHandle;

/** Prisma schema parser equivalent to Python {@code sql.SchemaParser}. */
public final class SchemaParser {
    private final String definition;
    public SchemaParser(String definition) { this.definition = definition; }
    public Schema parse() {
        ErrorBuffer error = new ErrorBuffer();
        return new Schema(error.check(SqlNative.INSTANCE.coolbox_sql_schema_parse(definition,error.pointer(),error.capacity())));
    }
    public static Schema parseFile(String path) {
        ErrorBuffer error = new ErrorBuffer();
        return new Schema(error.check(SqlNative.INSTANCE.coolbox_sql_schema_parse_file(path,error.pointer(),error.capacity())));
    }
}
