package io.coolbox.sql;

import com.sun.jna.Pointer;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.NativeHandle;
import java.util.Map;

/** Model-bound CRUD repository equivalent to Python {@code sql.Repository}. */
public final class Repository extends NativeHandle {
    private final Schema schema;
    public Repository(Database db,Model model){this(db,model.schema(),model.index());}
    private Repository(Database db,Schema schema,long model){this(create(db,schema,model),schema);}
    private Repository(Pointer p,Schema schema){super(p,SqlNative.INSTANCE::coolbox_sql_repository_free);this.schema=schema;}
    private static Pointer create(Database db,Schema schema,long model){ErrorBuffer e=new ErrorBuffer();return e.check(SqlNative.INSTANCE.coolbox_sql_repository_new(db.handle(),schema.handle(),model,e.pointer(),e.capacity()));}
    public ResultSet findAll(){return call(0,null,Map.of(),Map.of());}
    public ResultSet findById(String id){return call(1,id,Map.of(),Map.of());}
    public ResultSet findWhere(Map<String,String> conditions){return call(2,null,Map.of(),conditions);}
    public ResultSet create(Map<String,String> data){return call(3,null,data,Map.of());}
    public ResultSet update(String id,Map<String,String> data){return call(4,id,data,Map.of());}
    public ResultSet updateWhere(Map<String,String> conditions,Map<String,String> data){return call(5,null,data,conditions);}
    public ResultSet remove(String id){return call(6,id,Map.of(),Map.of());}
    public ResultSet removeWhere(Map<String,String> conditions){return call(7,null,Map.of(),conditions);}
    public void createTable(){closeResult(call(8,null,Map.of(),Map.of()));}
    public void dropTable(){closeResult(call(9,null,Map.of(),Map.of()));}
    private static void closeResult(ResultSet result){result.close();}
    private ResultSet call(int op,String id,Map<String,String> data,Map<String,String> conditions){ErrorBuffer e=new ErrorBuffer();String[] keys=SqlSupport.keys(data),values=SqlSupport.values(data),ck=SqlSupport.keys(conditions),cv=SqlSupport.values(conditions);Pointer p=e.check(SqlNative.INSTANCE.coolbox_sql_repository_call(handle(),op,id,keys,values,keys.length,ck,cv,ck.length,e.pointer(),e.capacity()));return new ResultSet(p);}
}
