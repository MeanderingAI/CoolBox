package io.coolbox.sql;

import com.sun.jna.Memory;
import com.sun.jna.Pointer;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.NativeHandle;
import java.nio.charset.StandardCharsets;

/** Schema migration helper equivalent to Python {@code sql.Migrator}. */
public final class Migrator extends NativeHandle {
    private final Schema schema;
    public Migrator(Database database,Schema schema){this(create(database,schema),schema);}
    private Migrator(Pointer p,Schema schema){super(p,SqlNative.INSTANCE::coolbox_sql_migrator_free);this.schema=schema;}
    private static Pointer create(Database d,Schema s){ErrorBuffer e=new ErrorBuffer();return e.check(SqlNative.INSTANCE.coolbox_sql_migrator_new(d.handle(),s.handle(),e.pointer(),e.capacity()));}
    public void migrateUp(){call(0);}
    public void migrateDown(){call(1);}
    public void reset(){call(2);}
    public String generateMigrationSql(){Memory out=new Memory(65536);ErrorBuffer e=new ErrorBuffer();e.check(SqlNative.INSTANCE.coolbox_sql_migrator_call(handle(),3,out,out.size(),e.pointer(),e.capacity()));return out.getString(0,StandardCharsets.UTF_8.name());}
    private void call(int op){ErrorBuffer e=new ErrorBuffer();e.check(SqlNative.INSTANCE.coolbox_sql_migrator_call(handle(),op,null,0,e.pointer(),e.capacity()));}
}
