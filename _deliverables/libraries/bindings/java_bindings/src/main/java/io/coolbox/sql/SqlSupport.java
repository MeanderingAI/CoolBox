package io.coolbox.sql;

import com.sun.jna.Pointer;
import io.coolbox.internal.Matrices;

final class SqlSupport {
    private SqlSupport() {}
    static String string(Pointer p){ return Matrices.takeString(p); }
    static String[] keys(java.util.Map<String,String> map){ return map.keySet().toArray(String[]::new); }
    static String[] values(java.util.Map<String,String> map){ return map.values().toArray(String[]::new); }
}
