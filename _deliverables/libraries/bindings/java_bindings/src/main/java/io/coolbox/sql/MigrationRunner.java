package io.coolbox.sql;

import com.sun.jna.Memory;
import com.sun.jna.Pointer;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.NativeHandle;
import java.nio.charset.StandardCharsets;

/** Filesystem migration runner equivalent to Python {@code sql.MigrationRunner}. */
public final class MigrationRunner extends NativeHandle {
    public MigrationRunner(Database database,String directory){super(create(database,directory),SqlNative.INSTANCE::coolbox_sql_runner_free);}
    private static Pointer create(Database d,String path){ErrorBuffer e=new ErrorBuffer();return e.check(SqlNative.INSTANCE.coolbox_sql_runner_new(d.handle(),path,e.pointer(),e.capacity()));}
    public void migrateUp(){call(0,1,false);}
    public void migrateDown(){migrateDown(1);}
    public void migrateDown(long steps){if(steps<1)throw new IllegalArgumentException("steps must be positive");call(1,steps,false);}
    public String currentVersion(){return call(2,1,true);}
    private String call(int op,long steps,boolean output){Memory text=output?new Memory(4096):null;ErrorBuffer e=new ErrorBuffer();e.check(SqlNative.INSTANCE.coolbox_sql_runner_call(handle(),op,steps,text,text==null?0:text.size(),e.pointer(),e.capacity()));return text==null?null:text.getString(0,StandardCharsets.UTF_8.name());}
}
