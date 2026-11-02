#include "coolbox_jbridge.h"
#include "bayesian_network/bayesian_network.h"
#include <Eigen/Dense>

namespace {
Eigen::MatrixXd matrix_of(const double* p,size_t rows,size_t cols) { auto v=coolbox_jbridge::rows_of(p,rows,cols); Eigen::MatrixXd m(rows,cols); for(size_t r=0;r<rows;++r) for(size_t c=0;c<cols;++c) m(r,c)=v[r][c]; return m; }
std::map<int,int> map_of(const int* keys,const int* values,size_t n) { if(n&&(!keys||!values)) throw std::invalid_argument("map arrays must not be null"); std::map<int,int> m; for(size_t i=0;i<n;++i) m[keys[i]]=values[i]; return m; }
}
extern "C" {
void* coolbox_bayes_network_new(char* e,size_t c) { return coolbox_jbridge::invoke_handle(e,c,[]{ return new BayesianNetwork(); }); }
void coolbox_bayes_network_free(void* h) { delete static_cast<BayesianNetwork*>(h); }
int coolbox_bayes_add_node(void* h,const char* name,const char** states,size_t n,int* out,char* e,size_t c) { return coolbox_jbridge::invoke(e,c,[&]{ if(!name||!out||(n&&!states)) throw std::invalid_argument("node name, states and output are required"); std::vector<std::string> v; v.reserve(n); for(size_t i=0;i<n;++i) v.emplace_back(states[i]?states[i]:""); *out=coolbox_jbridge::checked<BayesianNetwork>(h,"BayesianNetwork").add_node(name,v); }); }
int coolbox_bayes_add_edge(void* h,int parent,int child,char* e,size_t c) { return coolbox_jbridge::invoke(e,c,[&]{ coolbox_jbridge::checked<BayesianNetwork>(h,"BayesianNetwork").add_edge(parent,child); }); }
int coolbox_bayes_set_cpt(void* h,int node,const double* values,size_t rows,size_t cols,char* e,size_t c) { return coolbox_jbridge::invoke(e,c,[&]{ coolbox_jbridge::checked<BayesianNetwork>(h,"BayesianNetwork").set_cpt(node,matrix_of(values,rows,cols)); }); }
int coolbox_bayes_joint_probability(void* h,const int* keys,const int* vals,size_t n,double* out,char* e,size_t c) { return coolbox_jbridge::invoke(e,c,[&]{ if(!out) throw std::invalid_argument("output must not be null"); *out=coolbox_jbridge::checked<BayesianNetwork>(h,"BayesianNetwork").calculate_joint_probability(map_of(keys,vals,n)); }); }
int coolbox_bayes_infer(void* h,int query_node,int query_state,const int* keys,const int* vals,size_t n,double* out,char* e,size_t c) { return coolbox_jbridge::invoke(e,c,[&]{ if(!out) throw std::invalid_argument("output must not be null"); *out=coolbox_jbridge::checked<BayesianNetwork>(h,"BayesianNetwork").infer(query_node,query_state,map_of(keys,vals,n)); }); }
}
