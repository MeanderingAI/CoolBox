package io.coolbox.sql;

import com.sun.jna.Pointer;
import io.coolbox.internal.Matrices;
import io.coolbox.internal.NativeHandle;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/** Query result equivalent to Python {@code sql.ResultSet}. */
public final class ResultSet extends NativeHandle {
    ResultSet(Pointer p){super(p,SqlNative.INSTANCE::coolbox_sql_result_free);}
    public List<String> columns(){long n=SqlNative.INSTANCE.coolbox_sql_result_column_count(handle());List<String> out=new ArrayList<>();for(long i=0;i<n;i++)out.add(SqlSupport.string(SqlNative.INSTANCE.coolbox_sql_result_column(handle(),i)));return List.copyOf(out);}
    public List<Map<String,String>> rows(){long n=SqlNative.INSTANCE.coolbox_sql_result_row_count(handle());List<String> cols=columns();List<Map<String,String>> out=new ArrayList<>();for(long r=0;r<n;r++){Map<String,String> row=new LinkedHashMap<>();for(int c=0;c<cols.size();c++)row.put(cols.get(c),SqlSupport.string(SqlNative.INSTANCE.coolbox_sql_result_value(handle(),r,c)));out.add(Map.copyOf(row));}return List.copyOf(out);}
    public int affectedRows(){return SqlNative.INSTANCE.coolbox_sql_result_affected_rows(handle());}
    public long lastInsertId(){return SqlNative.INSTANCE.coolbox_sql_result_last_insert_id(handle());}
    public boolean empty(){return SqlNative.INSTANCE.coolbox_sql_result_row_count(handle())==0;}
    public long size(){return SqlNative.INSTANCE.coolbox_sql_result_row_count(handle());}
    public Map<String,String> getSingle(){List<Map<String,String>> r=rows();if(r.size()!=1)throw new IllegalStateException("Expected exactly one row, got "+r.size());return r.get(0);}
    public Map<String,String> getFirst(){List<Map<String,String>> r=rows();return r.isEmpty()?Map.of():r.get(0);}
    public String toJson(){return Matrices.takeString(SqlNative.INSTANCE.coolbox_sql_result_json(handle()));}
}
