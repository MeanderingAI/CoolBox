#include "coolbox_jbridge.h"
#include "schema_parser.h"
#include "database.h"
#include <memory>
#include <map>

using namespace ml::sql;
struct JSqlSchema { std::shared_ptr<Schema> value; };
struct JSqlDatabase { std::shared_ptr<Database> value; };
struct JSqlResult { ResultSet value; };
struct JSqlRepository { std::shared_ptr<Schema> schema; std::shared_ptr<Database> database; std::unique_ptr<Repository<void>> value; };
struct JSqlMigrator { std::shared_ptr<Schema> schema; std::shared_ptr<Database> database; std::unique_ptr<Migrator> value; };
struct JSqlRunner { std::shared_ptr<Database> database; std::unique_ptr<MigrationRunner> value; };
static std::map<std::string,std::string> jsql_map(const char* const* keys,const char* const* values,size_t count) {
    std::map<std::string,std::string> out;
    for(size_t i=0;i<count;++i) out[coolbox_jbridge::str(keys[i])] = coolbox_jbridge::str(values[i]);
    return out;
}
extern "C" {
void* coolbox_sql_schema_parse(const char* text,char* error,size_t cap) { return coolbox_jbridge::invoke_handle(error,cap,[&]{return new JSqlSchema{std::make_shared<Schema>(SchemaParser(coolbox_jbridge::str(text)).parse())};}); }
void* coolbox_sql_schema_parse_file(const char* path,char* error,size_t cap) { return coolbox_jbridge::invoke_handle(error,cap,[&]{return new JSqlSchema{std::make_shared<Schema>(SchemaParser::parse_file(coolbox_jbridge::str(path)))};}); }
void coolbox_sql_schema_free(void* p) { delete static_cast<JSqlSchema*>(p); }
size_t coolbox_sql_schema_model_count(void* p) { return coolbox_jbridge::checked<JSqlSchema>(p,"schema").value->models().size(); }
char* coolbox_sql_schema_provider(void* p) { return coolbox_jbridge::dup_string(coolbox_jbridge::checked<JSqlSchema>(p,"schema").value->get_provider()); }
char* coolbox_sql_model_name(void* p,size_t i) { auto& v=coolbox_jbridge::checked<JSqlSchema>(p,"schema").value->models(); if(i>=v.size()) return nullptr; return coolbox_jbridge::dup_string(v[i].name); }
char* coolbox_sql_model_table_name(void* p,size_t i) { auto& v=coolbox_jbridge::checked<JSqlSchema>(p,"schema").value->models(); if(i>=v.size()) return nullptr; return coolbox_jbridge::dup_string(v[i].table_name); }
size_t coolbox_sql_model_field_count(void* p,size_t i) { auto& v=coolbox_jbridge::checked<JSqlSchema>(p,"schema").value->models(); return i<v.size()?v[i].fields.size():0; }
char* coolbox_sql_field_name(void* p,size_t m,size_t f) { auto& v=coolbox_jbridge::checked<JSqlSchema>(p,"schema").value->models(); if(m>=v.size()||f>=v[m].fields.size()) return nullptr; return coolbox_jbridge::dup_string(v[m].fields[f].name); }
int coolbox_sql_field_type(void* p,size_t m,size_t f) { auto& v=coolbox_jbridge::checked<JSqlSchema>(p,"schema").value->models(); if(m>=v.size()||f>=v[m].fields.size()) return -1; return static_cast<int>(v[m].fields[f].type); }
int coolbox_sql_field_flag(void* p,size_t m,size_t f,int flag) { auto& v=coolbox_jbridge::checked<JSqlSchema>(p,"schema").value->models(); if(m>=v.size()||f>=v[m].fields.size()) return 0; const auto& x=v[m].fields[f]; switch(flag){case 0:return x.is_optional;case 1:return x.is_array;case 2:return x.is_id;case 3:return x.is_unique;case 4:return x.auto_increment;default:return x.is_relation;} }
char* coolbox_sql_field_default(void* p,size_t m,size_t f) { auto& v=coolbox_jbridge::checked<JSqlSchema>(p,"schema").value->models(); if(m>=v.size()||f>=v[m].fields.size()) return nullptr; return coolbox_jbridge::dup_string(v[m].fields[f].default_value); }
char* coolbox_sql_field_sql_type(void* p,size_t m,size_t f,const char* provider) { auto& v=coolbox_jbridge::checked<JSqlSchema>(p,"schema").value->models(); if(m>=v.size()||f>=v[m].fields.size()) return nullptr; return coolbox_jbridge::dup_string(v[m].fields[f].to_sql_type(coolbox_jbridge::str(provider))); }
void* coolbox_sql_database_new(const char* provider,char* error,size_t cap) { return coolbox_jbridge::invoke_handle(error,cap,[&]{return new JSqlDatabase{std::shared_ptr<Database>(Database::create(coolbox_jbridge::str(provider)).release())};}); }
void coolbox_sql_database_free(void* p) { delete static_cast<JSqlDatabase*>(p); }
int coolbox_sql_database_connect(void* p,const char* connection,char* error,size_t cap) { return coolbox_jbridge::invoke(error,cap,[&]{auto& d=*coolbox_jbridge::checked<JSqlDatabase>(p,"database").value; if(!d.connect(coolbox_jbridge::str(connection))) throw std::runtime_error("database connection failed");}); }
void coolbox_sql_database_disconnect(void* p) { coolbox_jbridge::checked<JSqlDatabase>(p,"database").value->disconnect(); }
int coolbox_sql_database_connected(void* p) { return coolbox_jbridge::checked<JSqlDatabase>(p,"database").value->is_connected(); }
char* coolbox_sql_database_provider(void* p) { return coolbox_jbridge::dup_string(coolbox_jbridge::checked<JSqlDatabase>(p,"database").value->get_provider()); }
void* coolbox_sql_execute(void* p,const char* query,const char* const* params,size_t count,char* error,size_t cap) { return coolbox_jbridge::invoke_handle(error,cap,[&]{return new JSqlResult{coolbox_jbridge::checked<JSqlDatabase>(p,"database").value->execute(coolbox_jbridge::str(query),[&]{std::vector<std::string> v;for(size_t i=0;i<count;++i)v.push_back(coolbox_jbridge::str(params[i]));return v;}())};}); }
int coolbox_sql_transaction(void* p,int op) { auto& d=*coolbox_jbridge::checked<JSqlDatabase>(p,"database").value; return op==0?d.begin_transaction():(op==1?d.commit():d.rollback()); }
int coolbox_sql_create_table(void* db,void* schema,size_t i,char* error,size_t cap) { return coolbox_jbridge::invoke(error,cap,[&]{auto& m=coolbox_jbridge::checked<JSqlSchema>(schema,"schema").value->models(); if(i>=m.size()) throw std::out_of_range("model index out of range"); if(!coolbox_jbridge::checked<JSqlDatabase>(db,"database").value->create_table_from_model(m[i])) throw std::runtime_error("create table failed");}); }
int coolbox_sql_drop_table(void* db,const char* table,char* error,size_t cap) { return coolbox_jbridge::invoke(error,cap,[&]{if(!coolbox_jbridge::checked<JSqlDatabase>(db,"database").value->drop_table(coolbox_jbridge::str(table))) throw std::runtime_error("drop table failed");}); }
void coolbox_sql_result_free(void* p) { delete static_cast<JSqlResult*>(p); }
size_t coolbox_sql_result_column_count(void* p) { return coolbox_jbridge::checked<JSqlResult>(p,"result").value.columns.size(); }
size_t coolbox_sql_result_row_count(void* p) { return coolbox_jbridge::checked<JSqlResult>(p,"result").value.rows.size(); }
int coolbox_sql_result_affected_rows(void* p) { return coolbox_jbridge::checked<JSqlResult>(p,"result").value.affected_rows; }
long long coolbox_sql_result_last_insert_id(void* p) { return coolbox_jbridge::checked<JSqlResult>(p,"result").value.last_insert_id; }
char* coolbox_sql_result_column(void* p,size_t i) { auto& v=coolbox_jbridge::checked<JSqlResult>(p,"result").value.columns; return i<v.size()?coolbox_jbridge::dup_string(v[i]):nullptr; }
char* coolbox_sql_result_value(void* p,size_t r,size_t c) { auto& v=coolbox_jbridge::checked<JSqlResult>(p,"result").value; if(r>=v.rows.size()||c>=v.columns.size())return nullptr; return coolbox_jbridge::dup_string(v.rows[r].at(v.columns[c])); }
char* coolbox_sql_result_json(void* p) { return coolbox_jbridge::dup_string(coolbox_jbridge::checked<JSqlResult>(p,"result").value.to_json()); }
void* coolbox_sql_repository_new(void* db,void* schema,size_t i,char* error,size_t cap) { return coolbox_jbridge::invoke_handle(error,cap,[&]{auto sh=coolbox_jbridge::checked<JSqlSchema>(schema,"schema").value; auto& models=sh->models(); if(i>=models.size()) throw std::out_of_range("model index out of range"); auto out=new JSqlRepository; out->schema=sh; out->database=coolbox_jbridge::checked<JSqlDatabase>(db,"database").value; out->value.reset(new Repository<void>(*out->database,models[i])); return out;}); }
void coolbox_sql_repository_free(void* p) { delete static_cast<JSqlRepository*>(p); }
void* coolbox_sql_repository_call(void* p,int op,const char* id,const char* const* keys,const char* const* values,size_t count,const char* const* ck,const char* const* cv,size_t ccount,char* error,size_t cap) { return coolbox_jbridge::invoke_handle(error,cap,[&]{auto& repo=*coolbox_jbridge::checked<JSqlRepository>(p,"repository").value; auto data=jsql_map(keys,values,count); auto conditions=jsql_map(ck,cv,ccount); ResultSet result; switch(op){case 0:result=repo.find_all();break;case 1:result=repo.find_by_id(coolbox_jbridge::str(id));break;case 2:result=repo.find_where(conditions);break;case 3:result=repo.create(data);break;case 4:result=repo.update(coolbox_jbridge::str(id),data);break;case 5:result=repo.update_where(conditions,data);break;case 6:result=repo.remove(coolbox_jbridge::str(id));break;case 7:result=repo.remove_where(conditions);break;case 8:if(!repo.create_table())throw std::runtime_error("create table failed");break;case 9:if(!repo.drop_table())throw std::runtime_error("drop table failed");break;default:throw std::invalid_argument("unknown repository operation");} return new JSqlResult{std::move(result)};}); }
void* coolbox_sql_migrator_new(void* db,void* schema,char* error,size_t cap) { return coolbox_jbridge::invoke_handle(error,cap,[&]{auto sh=coolbox_jbridge::checked<JSqlSchema>(schema,"schema").value; auto dh=coolbox_jbridge::checked<JSqlDatabase>(db,"database").value; auto out=new JSqlMigrator; out->schema=sh;out->database=dh;out->value.reset(new Migrator(*dh,*sh));return out;}); }
void coolbox_sql_migrator_free(void* p) { delete static_cast<JSqlMigrator*>(p); }
int coolbox_sql_migrator_call(void* p,int op,char** output,size_t cap,char* error,size_t errcap) { return coolbox_jbridge::invoke(error,errcap,[&]{auto& m=*coolbox_jbridge::checked<JSqlMigrator>(p,"migrator").value;std::string s;switch(op){case 0:if(!m.migrate_up())throw std::runtime_error("migration up failed");break;case 1:if(!m.migrate_down())throw std::runtime_error("migration down failed");break;case 2:if(!m.reset())throw std::runtime_error("migration reset failed");break;case 3:s=m.generate_migration_sql();break;default:throw std::invalid_argument("unknown migrator operation");}if(output&&cap){size_t n=std::min(cap-1,s.size());std::memcpy(output,s.data(),n);output[n]=0;}}); }
void* coolbox_sql_runner_new(void* db,const char* directory,char* error,size_t cap) { return coolbox_jbridge::invoke_handle(error,cap,[&]{auto dh=coolbox_jbridge::checked<JSqlDatabase>(db,"database").value;auto out=new JSqlRunner;out->database=dh;out->value.reset(new MigrationRunner(*dh,coolbox_jbridge::str(directory)));return out;}); }
void coolbox_sql_runner_free(void* p) { delete static_cast<JSqlRunner*>(p); }
int coolbox_sql_runner_call(void* p,int op,size_t steps,char** output,size_t cap,char* error,size_t errcap) { return coolbox_jbridge::invoke(error,errcap,[&]{auto& r=*coolbox_jbridge::checked<JSqlRunner>(p,"migration runner").value;std::string s;if(op==0)r.migrate_up();else if(op==1)r.migrate_down(steps);else if(op==2)s=r.current_version();else throw std::invalid_argument("unknown migration runner operation");if(output&&cap){size_t n=std::min(cap-1,s.size());std::memcpy(output,s.data(),n);output[n]=0;}}); }
}
