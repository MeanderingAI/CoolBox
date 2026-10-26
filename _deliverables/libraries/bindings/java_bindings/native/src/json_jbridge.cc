#include "coolbox_jbridge.h"
#include "json.h"
#include <map>
using namespace dataformats::json;
struct JsonMap { std::map<std::string,std::string> values; };
struct JsonArray { std::vector<std::string> values; };
static std::map<std::string,std::string> jmap(const char* const* keys,const char* const* vals,size_t n){std::map<std::string,std::string> m;for(size_t i=0;i<n;++i)m[coolbox_jbridge::str(keys[i])]=coolbox_jbridge::str(vals[i]);return m;}
extern "C" {
char* coolbox_json_encode(const char* const* keys,const char* const* vals,size_t n,char* error,size_t cap){return static_cast<char*>(coolbox_jbridge::invoke_handle(error,cap,[&]{return coolbox_jbridge::dup_string(simple::encode(jmap(keys,vals,n)));}));}
void* coolbox_json_decode(const char* text,char* error,size_t cap){return coolbox_jbridge::invoke_handle(error,cap,[&]{return new JsonMap{simple::decode(coolbox_jbridge::str(text))};});}
void coolbox_json_map_free(void* p){delete static_cast<JsonMap*>(p);}
size_t coolbox_json_map_size(void* p){return coolbox_jbridge::checked<JsonMap>(p,"JSON map").values.size();}
char* coolbox_json_map_key(void* p,size_t i){auto& v=coolbox_jbridge::checked<JsonMap>(p,"JSON map").values;if(i>=v.size())return nullptr;auto it=v.begin();std::advance(it,i);return coolbox_jbridge::dup_string(it->first);}
char* coolbox_json_map_value(void* p,size_t i){auto& v=coolbox_jbridge::checked<JsonMap>(p,"JSON map").values;if(i>=v.size())return nullptr;auto it=v.begin();std::advance(it,i);return coolbox_jbridge::dup_string(it->second);}
char* coolbox_json_encode_array(const char* const* vals,size_t n,char* error,size_t cap){return static_cast<char*>(coolbox_jbridge::invoke_handle(error,cap,[&]{std::vector<std::string> v;for(size_t i=0;i<n;++i)v.push_back(coolbox_jbridge::str(vals[i]));return coolbox_jbridge::dup_string(simple::encode_array(v));}));}
void* coolbox_json_decode_array(const char* text,char* error,size_t cap){return coolbox_jbridge::invoke_handle(error,cap,[&]{return new JsonArray{simple::decode_array(coolbox_jbridge::str(text))};});}
void coolbox_json_array_free(void* p){delete static_cast<JsonArray*>(p);}
size_t coolbox_json_array_size(void* p){return coolbox_jbridge::checked<JsonArray>(p,"JSON array").values.size();}
char* coolbox_json_array_value(void* p,size_t i){auto& v=coolbox_jbridge::checked<JsonArray>(p,"JSON array").values;return i<v.size()?coolbox_jbridge::dup_string(v[i]):nullptr;}
}

