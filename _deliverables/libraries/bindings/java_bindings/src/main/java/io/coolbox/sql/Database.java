package io.coolbox.sql;

import com.sun.jna.Pointer;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.NativeHandle;
import java.util.List;

/** SQL database equivalent to Python {@code sql.Database}. */
public final class Database extends NativeHandle {
    private Database(Pointer p){super(p,SqlNative.INSTANCE::coolbox_sql_database_free);}
    public static Database create(String provider){ErrorBuffer e=new ErrorBuffer();return new Database(e.check(SqlNative.INSTANCE.coolbox_sql_database_new(provider,e.pointer(),e.capacity())));}
    public void connect(String connectionString){ErrorBuffer e=new ErrorBuffer();e.check(SqlNative.INSTANCE.coolbox_sql_database_connect(handle(),connectionString,e.pointer(),e.capacity()));}
    public void disconnect(){SqlNative.INSTANCE.coolbox_sql_database_disconnect(handle());}
    public boolean isConnected(){return SqlNative.INSTANCE.coolbox_sql_database_connected(handle())!=0;}
    public String getProvider(){return SqlSupport.string(SqlNative.INSTANCE.coolbox_sql_database_provider(handle()));}
    public ResultSet execute(String query){return execute(query,List.of());}
    public ResultSet execute(String query,String... params){return execute(query,java.util.Arrays.asList(params));}
    public ResultSet execute(String query,List<String> params){
        ErrorBuffer e=new ErrorBuffer();String[] values=params.toArray(String[]::new);
        Pointer result=e.check(SqlNative.INSTANCE.coolbox_sql_execute(handle(),query,values,values.length,e.pointer(),e.capacity()));
        return new ResultSet(result);
    }
    public boolean beginTransaction(){return SqlNative.INSTANCE.coolbox_sql_transaction(handle(),0)!=0;}
    public boolean commit(){return SqlNative.INSTANCE.coolbox_sql_transaction(handle(),1)!=0;}
    public boolean rollback(){return SqlNative.INSTANCE.coolbox_sql_transaction(handle(),2)!=0;}
    public void createTableFromModel(Model model){requireSchema(model);ErrorBuffer e=new ErrorBuffer();e.check(SqlNative.INSTANCE.coolbox_sql_create_table(handle(),model.schema().handle(),model.index(),e.pointer(),e.capacity()));}
    public void dropTable(String tableName){ErrorBuffer e=new ErrorBuffer();e.check(SqlNative.INSTANCE.coolbox_sql_drop_table(handle(),tableName,e.pointer(),e.capacity()));}
    private static void requireSchema(Model model){if(model==null)throw new IllegalArgumentException("model is required");}
}
