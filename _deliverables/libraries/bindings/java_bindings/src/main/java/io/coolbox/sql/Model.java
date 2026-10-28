package io.coolbox.sql;

/** Prisma model metadata, equivalent to Python {@code sql.Model}. */
public final class Model {
    private final Schema schema;
    private final long index;
    Model(Schema schema,long index) { this.schema=schema;this.index=index; }
    public String name() { return SqlSupport.string(SqlNative.INSTANCE.coolbox_sql_model_name(schema.handle(),index)); }
    public String tableName() { return SqlSupport.string(SqlNative.INSTANCE.coolbox_sql_model_table_name(schema.handle(),index)); }
    public java.util.List<Field> fields() {
        var result=new java.util.ArrayList<Field>();
        for(long i=0;i<SqlNative.INSTANCE.coolbox_sql_model_field_count(schema.handle(),index);i++){
            int t=SqlNative.INSTANCE.coolbox_sql_field_type(schema.handle(),index,i);
            FieldType[] types=FieldType.values();
            if(t<0||t>=types.length) throw new IllegalStateException("Unknown SQL field type: "+t);
            result.add(new Field(SqlSupport.string(SqlNative.INSTANCE.coolbox_sql_field_name(schema.handle(),index,i)),types[t],
                flag(i,0),flag(i,1),flag(i,2),flag(i,3),flag(i,4),SqlSupport.string(SqlNative.INSTANCE.coolbox_sql_field_default(schema.handle(),index,i)),flag(i,5)));
        }
        return java.util.List.copyOf(result);
    }
    private boolean flag(long field,int flag){return SqlNative.INSTANCE.coolbox_sql_field_flag(schema.handle(),index,field,flag)!=0;}
    public java.util.List<Field> getPrimaryKeys(){return fields().stream().filter(Field::id).toList();}
    public java.util.List<Field> getDataFields(){return fields().stream().filter(f->!f.relation()).toList();}
    public java.util.List<Field> getRelationFields(){return fields().stream().filter(Field::relation).toList();}
    Schema schema(){return schema;}
    long index(){return index;}
}
