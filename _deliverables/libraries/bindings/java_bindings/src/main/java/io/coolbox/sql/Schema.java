package io.coolbox.sql;

import com.sun.jna.Pointer;
import io.coolbox.internal.Matrices;
import io.coolbox.internal.NativeHandle;
import java.util.ArrayList;
import java.util.List;

/** Parsed Prisma schema, equivalent to Python {@code sql.Schema}. */
public final class Schema extends NativeHandle {
    Schema(Pointer pointer) { super(pointer, SqlNative.INSTANCE::coolbox_sql_schema_free); }
    public List<Model> models() {
        List<Model> result = new ArrayList<>();
        for (long i=0;i<SqlNative.INSTANCE.coolbox_sql_schema_model_count(handle());i++) result.add(new Model(this,i));
        return List.copyOf(result);
    }
    public Model getModel(String name) {
        return models().stream().filter(m -> m.name().equals(name)).findFirst()
                .orElseThrow(() -> new IllegalArgumentException("Unknown model: " + name));
    }
    public String getProvider() { return Matrices.takeString(SqlNative.INSTANCE.coolbox_sql_schema_provider(handle())); }
}
