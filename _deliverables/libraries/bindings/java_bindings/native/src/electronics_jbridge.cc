#include "coolbox_jbridge.h"
#include "battery.h"
#include "circuit_solver.h"
#include "json.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <limits>
#include <memory>
#include <sstream>
#include <utility>

using namespace battery;
using namespace circuitry;
using dataformats::json::Object;
using dataformats::json::Parser;
using dataformats::json::Value;
struct JChemDefaults { ChemistryDefaults value; };
struct JSolution { CircuitSolution value; };

static std::string text_of(const Object& object,const std::string& key){auto value=object.get(key);if(value.is_string())return value.as_string();if(value.is_number())return std::to_string(value.as_number());return {};}
static double number_of(const Object& object,const std::string& key,double fallback=0){auto value=object.get(key);if(value.is_number())return value.as_number();if(value.is_string()){std::istringstream input(value.as_string());double n=fallback;if(input>>n)return n;}return fallback;}
static double parse_number(const std::string& value){std::istringstream input(value);double n=0;if(!(input>>n))throw std::invalid_argument("component value must begin with a number");return n;}
struct CircuitItem { std::string type,label;int first=-1,second=-1;double resistance=0,emf=0,internal=0;int source=-1; };
struct DisjointSet { std::vector<int> parent; explicit DisjointSet(size_t n):parent(n){for(size_t i=0;i<n;++i)parent[i]=static_cast<int>(i);}int find(int x){return parent[x]==x?x:parent[x]=find(parent[x]);}void join(int a,int b){a=find(a);b=find(b);if(a!=b)parent[b]=a;} };
static void stamp_resistor(std::vector<std::vector<double>>& a,int ground,int first,int second,double resistance){if(resistance<=0)throw std::invalid_argument("resistance must be positive");double g=1.0/resistance;auto v=[&](int n){return n==ground?-1:(n<ground?n:n-1);};int i=v(first),j=v(second);if(i>=0)a[i][i]+=g;if(j>=0)a[j][j]+=g;if(i>=0&&j>=0){a[i][j]-=g;a[j][i]-=g;}}
static JSolution* solve_circuit(const std::string& json){
 Value root=Parser::parse(json);if(!root.is_array())throw std::invalid_argument("circuit JSON must be an array");
 std::map<std::pair<double,double>,int> point_ids;std::vector<std::pair<int,int>> endpoints;
 auto point=[&](const Object& o,const char* x,const char* y){double px=number_of(o,x,std::numeric_limits<double>::quiet_NaN()),py=number_of(o,y,std::numeric_limits<double>::quiet_NaN());if(!std::isfinite(px)||!std::isfinite(py))throw std::invalid_argument("component endpoints require numeric coordinates");auto key=std::make_pair(px,py);auto it=point_ids.find(key);if(it!=point_ids.end())return it->second;int id=static_cast<int>(point_ids.size());point_ids.emplace(key,id);return id;};
 std::vector<CircuitItem> items;
 for(const auto& value:root.as_array()){
  if(!value.is_object())throw std::invalid_argument("circuit components must be objects");auto o=value.as_object();CircuitItem item;item.type=text_of(o,"type");item.label=text_of(o,"label");item.first=point(o,"x1","y1");item.second=point(o,"x2","y2");
  if(item.type=="resistor"){item.resistance=parse_number(text_of(o,"value"));if(item.resistance<=0)throw std::invalid_argument("resistor value must be positive");}
  else if(item.type=="battery"){item.emf=parse_number(text_of(o,"value"));item.internal=text_of(o,"resistance").empty()?0:parse_number(text_of(o,"resistance"));if(item.internal<0)throw std::invalid_argument("battery resistance cannot be negative");}
  else if(item.type!="wire")throw std::invalid_argument("unsupported circuit component type: "+item.type);
  items.push_back(item);
 }
 DisjointSet sets(point_ids.size());for(const auto& item:items)if(item.type=="wire")sets.join(item.first,item.second);
 std::map<int,int> compressed;for(size_t i=0;i<point_ids.size();++i){int root_id=sets.find(static_cast<int>(i));if(!compressed.count(root_id))compressed[root_id]=static_cast<int>(compressed.size());}
 for(auto& item:items){item.first=compressed[sets.find(item.first)];item.second=compressed[sets.find(item.second)];}
 int base_nodes=static_cast<int>(compressed.size());int ground=0;for(const auto& item:items)if(item.type=="battery"){ground=item.second;break;}
 std::vector<int> source_positive(items.size(),-1),source_negative(items.size(),-1);int next_node=base_nodes;int battery_count=0;
 for(size_t i=0;i<items.size();++i)if(items[i].type=="battery"){auto& item=items[i];source_positive[i]=item.internal>0?next_node++:item.first;source_negative[i]=item.second;item.source=battery_count++;}
 int variables=next_node-1+battery_count;std::vector<std::vector<double>> matrix(variables,std::vector<double>(variables,0));std::vector<double> rhs(variables,0);
 auto variable=[&](int node){return node==ground?-1:(node<ground?node:node-1);};
 for(size_t i=0;i<items.size();++i){auto& item=items[i];if(item.type=="resistor")stamp_resistor(matrix,ground,item.first,item.second,item.resistance);else if(item.type=="battery"&&item.internal>0)stamp_resistor(matrix,ground,item.first,source_positive[i],item.internal);}
 for(size_t i=0;i<items.size();++i)if(items[i].type=="battery"){
  auto& item=items[i];int positive=source_positive[i],negative=source_negative[i];int source_index=(next_node-1)+item.source;int ip=variable(positive),in=variable(negative);
  if(ip>=0){matrix[ip][source_index]+=1;matrix[source_index][ip]+=1;}
  if(in>=0){matrix[in][source_index]-=1;matrix[source_index][in]-=1;}
  rhs[source_index]=item.emf;
 }
 std::vector<double> solution(variables,0);
 for(int col=0;col<variables;++col){int pivot=col;for(int row=col+1;row<variables;++row)if(std::abs(matrix[row][col])>std::abs(matrix[pivot][col]))pivot=row;if(std::abs(matrix[pivot][col])<1e-12)throw std::runtime_error("circuit equations are singular");std::swap(matrix[pivot],matrix[col]);std::swap(rhs[pivot],rhs[col]);double scale=matrix[col][col];for(int j=col;j<variables;++j)matrix[col][j]/=scale;rhs[col]/=scale;for(int row=0;row<variables;++row)if(row!=col){double factor=matrix[row][col];if(factor==0)continue;for(int j=col;j<variables;++j)matrix[row][j]-=factor*matrix[col][j];rhs[row]-=factor*rhs[col];}}
 solution=rhs;auto voltage=[&](int node){int v=variable(node);return v<0?0.0:solution[v];};auto* out=new JSolution();for(const auto& item:items){out->value.node_voltages[item.first]=voltage(item.first);out->value.node_voltages[item.second]=voltage(item.second);}
 for(size_t i=0;i<items.size();++i){const auto& item=items[i];ComponentResult result;result.label=item.label;result.type_name=item.type=="battery"?"Battery":(item.type=="resistor"?"Resistor":"Wire");if(item.type=="resistor"){result.resistance=item.resistance;result.voltage_drop=voltage(item.first)-voltage(item.second);result.current=result.voltage_drop/item.resistance;result.power=result.current*result.current*item.resistance;}else if(item.type=="battery"){int vi=(next_node-1)+item.source;result.emf=item.emf;result.internal_resistance=item.internal;result.current=-solution[vi];result.terminal_voltage=voltage(item.first)-voltage(item.second);result.voltage_drop=result.current*item.internal;}out->value.component_results.push_back(result);}
 return out;
}
extern "C" {
char* coolbox_electronics_chemistry_name(int chemistry) { return coolbox_jbridge::dup_string(chemistry_name(static_cast<Chemistry>(chemistry))); }
void* coolbox_electronics_defaults(int chemistry,char* error,size_t cap) { return coolbox_jbridge::invoke_handle(error,cap,[&]{if(chemistry<0||chemistry>5)throw std::invalid_argument("unknown battery chemistry");return new JChemDefaults{ChemistryDefaults::for_chemistry(static_cast<Chemistry>(chemistry))};}); }
void coolbox_electronics_defaults_free(void* p){delete static_cast<JChemDefaults*>(p);}
double coolbox_electronics_default_value(void* p,int i){auto& v=coolbox_jbridge::checked<JChemDefaults>(p,"chemistry defaults").value;switch(i){case 0:return v.nominal_voltage;case 1:return v.max_voltage;case 2:return v.min_voltage;case 3:return v.typical_capacity;default:return v.typical_internal_resistance;}}
void* coolbox_electronics_cell_new(const char* label,int chemistry,double capacity,double resistance,double soc,int custom,char* error,size_t cap){return coolbox_jbridge::invoke_handle(error,cap,[&]{if(chemistry<0||chemistry>5)throw std::invalid_argument("unknown battery chemistry");auto c=static_cast<Chemistry>(chemistry);if(custom)return new Cell(coolbox_jbridge::str(label),c,capacity,resistance,soc);return new Cell(coolbox_jbridge::str(label),c,soc);});}
void* coolbox_electronics_cell_copy(void* p){return new Cell(coolbox_jbridge::checked<Cell>(p,"cell"));}
void coolbox_electronics_cell_free(void* p){delete static_cast<Cell*>(p);}
double coolbox_electronics_cell_value(void* p,int i){auto& c=coolbox_jbridge::checked<Cell>(p,"cell");switch(i){case 0:return c.soc();case 1:return c.soc_percent();case 2:return c.open_circuit_voltage();case 3:return c.max_current();case 4:return c.nominal_voltage();case 5:return c.max_voltage();case 6:return c.min_voltage();case 7:return c.capacity_ah();case 8:return c.remaining_ah();case 9:return c.internal_resistance();case 10:return c.temperature_c();case 11:return c.cycle_count();default:throw std::invalid_argument("unknown cell value");}}
int coolbox_electronics_cell_flag(void* p,int i){auto& c=coolbox_jbridge::checked<Cell>(p,"cell");return i==0?c.is_depleted():c.is_fully_charged();}
int coolbox_electronics_cell_chemistry(void* p){return static_cast<int>(coolbox_jbridge::checked<Cell>(p,"cell").chemistry());}
char* coolbox_electronics_cell_label(void* p){return coolbox_jbridge::dup_string(coolbox_jbridge::checked<Cell>(p,"cell").label());}
char* coolbox_electronics_cell_string(void* p){return coolbox_jbridge::dup_string(coolbox_jbridge::checked<Cell>(p,"cell").to_string());}
double coolbox_electronics_cell_terminal(void* p,double current){return coolbox_jbridge::checked<Cell>(p,"cell").terminal_voltage(current);}
double coolbox_electronics_cell_transfer(void* p,double current,double seconds,int charge){return charge?coolbox_jbridge::checked<Cell>(p,"cell").charge(current,seconds):coolbox_jbridge::checked<Cell>(p,"cell").discharge(current,seconds);}
void coolbox_electronics_cell_set(void* p,int which,double value){auto& c=coolbox_jbridge::checked<Cell>(p,"cell");if(which==0)c.set_temperature(value);else c.set_internal_resistance(value);}
void* coolbox_electronics_pack_new(const char* label,int series,int parallel,void* cell,char* error,size_t cap){return coolbox_jbridge::invoke_handle(error,cap,[&]{return new BatteryPack(coolbox_jbridge::str(label),series,parallel,coolbox_jbridge::checked<Cell>(cell,"cell"));});}
void coolbox_electronics_pack_free(void* p){delete static_cast<BatteryPack*>(p);}
double coolbox_electronics_pack_value(void* p,int i){auto& b=coolbox_jbridge::checked<BatteryPack>(p,"battery pack");switch(i){case 0:return b.pack_voltage();case 1:return b.pack_capacity_ah();case 2:return b.pack_energy_wh();case 3:return b.pack_internal_resistance();case 4:return b.average_soc();case 5:return b.min_soc();case 6:return b.total_cells();case 7:return b.series_count();default:return b.parallel_count();}}
double coolbox_electronics_pack_terminal(void* p,double current){return coolbox_jbridge::checked<BatteryPack>(p,"battery pack").pack_terminal_voltage(current);}
int coolbox_electronics_pack_depleted(void* p){return coolbox_jbridge::checked<BatteryPack>(p,"battery pack").is_depleted();}
double coolbox_electronics_pack_transfer(void* p,double current,double seconds,int charge){return charge?coolbox_jbridge::checked<BatteryPack>(p,"battery pack").charge(current,seconds):coolbox_jbridge::checked<BatteryPack>(p,"battery pack").discharge(current,seconds);}
void* coolbox_electronics_pack_cell(void* p,int series,int parallel,char* error,size_t cap){return coolbox_jbridge::invoke_handle(error,cap,[&]{return new Cell(coolbox_jbridge::checked<BatteryPack>(p,"battery pack").cell(series,parallel));});}
char* coolbox_electronics_pack_label(void* p){return coolbox_jbridge::dup_string(coolbox_jbridge::checked<BatteryPack>(p,"battery pack").label());}
char* coolbox_electronics_pack_string(void* p){return coolbox_jbridge::dup_string(coolbox_jbridge::checked<BatteryPack>(p,"battery pack").to_string());}
void* coolbox_electronics_solve_circuit(const char* json,char* error,size_t cap){return coolbox_jbridge::invoke_handle(error,cap,[&]{return solve_circuit(coolbox_jbridge::str(json));});}
void coolbox_electronics_solution_free(void* p){delete static_cast<JSolution*>(p);}
size_t coolbox_electronics_node_count(void* p){return coolbox_jbridge::checked<JSolution>(p,"circuit solution").value.node_voltages.size();}
int coolbox_electronics_node_index(void* p,size_t i){auto& v=coolbox_jbridge::checked<JSolution>(p,"circuit solution").value.node_voltages;if(i>=v.size())return 0;auto it=v.begin();std::advance(it,i);return it->first;}
double coolbox_electronics_node_voltage(void* p,size_t i){auto& v=coolbox_jbridge::checked<JSolution>(p,"circuit solution").value.node_voltages;if(i>=v.size())return 0;auto it=v.begin();std::advance(it,i);return it->second;}
size_t coolbox_electronics_component_count(void* p){return coolbox_jbridge::checked<JSolution>(p,"circuit solution").value.component_results.size();}
char* coolbox_electronics_component_string(void* p,size_t i,int which){auto& v=coolbox_jbridge::checked<JSolution>(p,"circuit solution").value.component_results;if(i>=v.size())return nullptr;return coolbox_jbridge::dup_string(which==0?v[i].label:v[i].type_name);}
double coolbox_electronics_component_value(void* p,size_t i,int which){auto& v=coolbox_jbridge::checked<JSolution>(p,"circuit solution").value.component_results;if(i>=v.size())return 0;auto& c=v[i];switch(which){case 0:return c.resistance;case 1:return c.voltage_drop;case 2:return c.current;case 3:return c.power;case 4:return c.emf;case 5:return c.internal_resistance;default:return c.terminal_voltage;}}
}
