#include "coolbox_jbridge.h"
#include <distributed/message_passing.h>
#include <distributed/distributed_trainer.h>
#include <algorithm>
#include <memory>
#include <vector>
#include <map>

namespace {
using namespace distributed;
std::shared_ptr<DistributedContext>& context(void* p){return coolbox_jbridge::checked<std::shared_ptr<DistributedContext>>(p,"distributed context");}
std::shared_ptr<DistributedTrainer>& trainer(void* p){return coolbox_jbridge::checked<std::shared_ptr<DistributedTrainer>>(p,"distributed trainer");}
std::vector<std::vector<double>> matrix(const double* p,size_t rows,size_t cols){return coolbox_jbridge::rows_of(p,rows,cols);}
double* matrix_out(const std::vector<std::vector<double>>& m,size_t* r,size_t* c){return coolbox_jbridge::malloc_matrix(m,r,c);}
}
extern "C" {
void* coolbox_dist_message_new(int type,int src,int dst,char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]()->void*{if(type<0||type>7)throw std::invalid_argument("invalid message type");return new distributed::Message(static_cast<MessageType>(type),src,dst);});}
void coolbox_dist_message_free(void*p){delete static_cast<Message*>(p);}
int coolbox_dist_message_properties(void*p,int* type,int* src,int* dst,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{auto&m=coolbox_jbridge::checked<Message>(p,"message");if(!type||!src||!dst)throw std::invalid_argument("message output is null");*type=(int)m.type;*src=m.source_rank;*dst=m.dest_rank;});}
size_t coolbox_dist_message_data_size(void*p){return coolbox_jbridge::checked<Message>(p,"message").data.size();}
int coolbox_dist_message_get_data(void*p,double*out,size_t n,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{auto&v=coolbox_jbridge::checked<Message>(p,"message").data;if(!out||n<v.size())throw std::invalid_argument("message data buffer too small");std::copy(v.begin(),v.end(),out);});}
int coolbox_dist_message_set_data(void*p,const double*v,size_t n,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{coolbox_jbridge::checked<Message>(p,"message").data=coolbox_jbridge::vec(v,n);});}
int coolbox_dist_message_set_metadata(void*p,const char*k,const char*v,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(!k)throw std::invalid_argument("metadata key is null");coolbox_jbridge::checked<Message>(p,"message").metadata[k]=coolbox_jbridge::str(v);});}
size_t coolbox_dist_message_metadata_count(void*p){return coolbox_jbridge::checked<Message>(p,"message").metadata.size();}
char* coolbox_dist_message_metadata_key(void*p,size_t i,char*e,size_t c){return static_cast<char*>(coolbox_jbridge::invoke_handle(e,c,[&]()->void*{auto&m=coolbox_jbridge::checked<Message>(p,"message").metadata;if(i>=m.size())throw std::out_of_range("metadata index out of range");auto it=m.begin();std::advance(it,i);return coolbox_jbridge::dup_string(it->first);}));}
char* coolbox_dist_message_metadata_value(void*p,const char*k,char*e,size_t c){return static_cast<char*>(coolbox_jbridge::invoke_handle(e,c,[&]()->void*{if(!k)throw std::invalid_argument("metadata key is null");auto&m=coolbox_jbridge::checked<Message>(p,"message").metadata;auto i=m.find(k);return coolbox_jbridge::dup_string(i==m.end()?std::string():i->second);}));}
void* coolbox_dist_context_new(int world,int rank,char*e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]()->void*{return new std::shared_ptr<DistributedContext>(std::make_shared<DistributedContext>(world,rank));});}
void coolbox_dist_context_free(void*p){delete static_cast<std::shared_ptr<DistributedContext>*>(p);}
int coolbox_dist_context_rank(void*p){return context(p)->rank();}
int coolbox_dist_context_world_size(void*p){return context(p)->world_size();}
int coolbox_dist_context_is_master(void*p){return context(p)->is_master();}
int coolbox_dist_context_send(void*p,void*m,int dest,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{context(p)->send(coolbox_jbridge::checked<Message>(m,"message"),dest);});}
void* coolbox_dist_context_receive(void*p,int src,char*e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]{return new Message(context(p)->receive(src));});}
void* coolbox_dist_context_all_reduce(void*p,const double*v,size_t n,int op,size_t*outn,char*e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]()->void*{if(op<0||op>4)throw std::invalid_argument("invalid reduction operation");return coolbox_jbridge::malloc_copy(context(p)->all_reduce(coolbox_jbridge::vec(v,n),static_cast<ReduceOp>(op)),outn);});}
void* coolbox_dist_context_ring_all_reduce(void*p,const double*v,size_t n,size_t*outn,char*e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]()->void*{return coolbox_jbridge::malloc_copy(context(p)->ring_all_reduce(coolbox_jbridge::vec(v,n)),outn);});}
int coolbox_dist_context_broadcast(void*p,double*v,size_t n,int root,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(n&&!v)throw std::invalid_argument("data is null");std::vector<double>x=v?std::vector<double>(v,v+n):std::vector<double>();context(p)->broadcast(x,root);if(x.size()>n)throw std::runtime_error("broadcast changed buffer size");std::copy(x.begin(),x.end(),v);});}
int coolbox_dist_context_scatter(void*p,const double*v,size_t n,double*out,size_t cap,size_t*outn,int root,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{auto x=coolbox_jbridge::vec(v,n);std::vector<double>y;context(p)->scatter(x,y,root);if(outn)*outn=y.size();if(y.size()>cap||(!out&&y.size()))throw std::invalid_argument("scatter output buffer too small");std::copy(y.begin(),y.end(),out);});}
int coolbox_dist_context_gather(void*p,const double*v,size_t n,double*out,size_t cap,size_t*outn,int root,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{auto x=coolbox_jbridge::vec(v,n);std::vector<double>y;context(p)->gather(x,y,root);if(outn)*outn=y.size();if(y.size()>cap||(!out&&y.size()))throw std::invalid_argument("gather output buffer too small");std::copy(y.begin(),y.end(),out);});}
int coolbox_dist_context_barrier(void*p,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{context(p)->barrier();});}
void* coolbox_dist_partitioner_new(size_t total,int world,char*e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]{return new DataPartitioner(total,world);});}
void coolbox_dist_partitioner_free(void*p){delete static_cast<DataPartitioner*>(p);}
int coolbox_dist_partitioner_partition(void*p,int rank,size_t*start,size_t*end,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(!start||!end)throw std::invalid_argument("partition outputs are null");auto x=coolbox_jbridge::checked<DataPartitioner>(p,"partitioner").get_partition(rank);*start=x.first;*end=x.second;});}
void* coolbox_dist_partitioner_indices(void*p,int rank,size_t*n,char*e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]()->void*{return coolbox_jbridge::malloc_copy(coolbox_jbridge::checked<DataPartitioner>(p,"partitioner").get_indices(rank),n);});}
size_t coolbox_dist_partitioner_size(void*p,int rank){return coolbox_jbridge::checked<DataPartitioner>(p,"partitioner").partition_size(rank);}
void* coolbox_dist_parameter_server_new(int world,char*e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]{return new ParameterServer(world);});}
void coolbox_dist_parameter_server_free(void*p){delete static_cast<ParameterServer*>(p);}
int coolbox_dist_server_set(void*p,const char*k,const double*v,size_t n,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(!k)throw std::invalid_argument("key is null");coolbox_jbridge::checked<ParameterServer>(p,"parameter server").set_parameters(k,coolbox_jbridge::vec(v,n));});}
void* coolbox_dist_server_get(void*p,const char*k,size_t*n,char*e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]()->void*{if(!k)throw std::invalid_argument("key is null");return coolbox_jbridge::malloc_copy(coolbox_jbridge::checked<ParameterServer>(p,"parameter server").get_parameters(k),n);});}
int coolbox_dist_server_update(void*p,const char*k,const double*v,size_t n,double lr,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(!k)throw std::invalid_argument("key is null");coolbox_jbridge::checked<ParameterServer>(p,"parameter server").update_parameters(k,coolbox_jbridge::vec(v,n),lr);});}
int coolbox_dist_server_accumulate(void*p,const char*k,const double*v,size_t n,int rank,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(!k)throw std::invalid_argument("key is null");coolbox_jbridge::checked<ParameterServer>(p,"parameter server").accumulate_gradient(k,coolbox_jbridge::vec(v,n),rank);});}
int coolbox_dist_server_apply(void*p,const char*k,double lr,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(!k)throw std::invalid_argument("key is null");coolbox_jbridge::checked<ParameterServer>(p,"parameter server").apply_gradients(k,lr);});}
int coolbox_dist_server_clear(void*p,const char*k,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(!k)throw std::invalid_argument("key is null");coolbox_jbridge::checked<ParameterServer>(p,"parameter server").clear_gradients(k);});}
void* coolbox_dist_trainer_neural_new(void*ctx,int strategy,int in,const int*hidden,size_t hn,int out,double lr,char*e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]()->void*{if(strategy<0||strategy>4)throw std::invalid_argument("invalid training strategy");std::vector<int> h=hidden?std::vector<int>(hidden,hidden+hn):std::vector<int>();return new std::shared_ptr<DistributedTrainer>(std::make_shared<DistributedNeuralNetTrainer>(context(ctx),static_cast<TrainingStrategy>(strategy),in,std::move(h),out,lr));});}
void* coolbox_dist_trainer_kmeans_new(void*ctx,int strategy,int k,int iterations,char*e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]()->void*{if(strategy<0||strategy>4)throw std::invalid_argument("invalid training strategy");return new std::shared_ptr<DistributedTrainer>(std::make_shared<DistributedKMeansTrainer>(context(ctx),k,iterations));});}
void coolbox_dist_trainer_free(void*p){delete static_cast<std::shared_ptr<DistributedTrainer>*>(p);}
int coolbox_dist_trainer_epoch(void*p,const double*x,size_t xr,size_t xc,const double*y,size_t yr,size_t yc,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(xr!=yr)throw std::invalid_argument("training data and labels must have the same row count");trainer(p)->train_epoch(matrix(x,xr,xc),matrix(y,yr,yc));});}
void* coolbox_dist_trainer_parameters(void*p,size_t*n,char*e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]()->void*{return coolbox_jbridge::malloc_copy(trainer(p)->get_parameters(),n);});}
int coolbox_dist_trainer_set_parameters(void*p,const double*v,size_t n,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{trainer(p)->set_parameters(coolbox_jbridge::vec(v,n));});}
void* coolbox_dist_trainer_predict(void*p,const double*v,size_t n,size_t*outn,char*e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]()->void*{return coolbox_jbridge::malloc_copy(trainer(p)->predict(coolbox_jbridge::vec(v,n)),outn);});}
int coolbox_dist_trainer_sync(void*p,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{trainer(p)->synchronize_model();});}
void* coolbox_dist_trainer_aggregate(void*p,const double*v,size_t n,int method,size_t*outn,char*e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]()->void*{if(method<0||method>2)throw std::invalid_argument("invalid aggregation method");return coolbox_jbridge::malloc_copy(trainer(p)->aggregate_gradients(coolbox_jbridge::vec(v,n),static_cast<AggregationMethod>(method)),outn);});}
int coolbox_dist_trainer_local_loss(void*p,double*out,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(!out)throw std::invalid_argument("loss output is null");auto n=std::dynamic_pointer_cast<DistributedNeuralNetTrainer>(trainer(p));if(!n)throw std::invalid_argument("not a neural network trainer");*out=n->get_local_loss();});}
int coolbox_dist_trainer_global_loss(void*p,double*out,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(!out)throw std::invalid_argument("loss output is null");auto n=std::dynamic_pointer_cast<DistributedNeuralNetTrainer>(trainer(p));if(!n)throw std::invalid_argument("not a neural network trainer");*out=n->get_global_loss();});}
void* coolbox_dist_trainer_centroids(void*p,size_t*r,size_t*cols,char*e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]()->void*{auto n=std::dynamic_pointer_cast<DistributedKMeansTrainer>(trainer(p));if(!n)throw std::invalid_argument("not a k-means trainer");return matrix_out(n->get_centroids(),r,cols);});}
void* coolbox_dist_partition_data(const double*x,size_t rows,size_t cols,int world,size_t*out_rows,size_t*out_cols,char*e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]()->void*{auto parts=distributed::utils::partition_data(matrix(x,rows,cols),world);std::vector<std::vector<double>> all;for(auto&part:parts)for(auto&row:part)all.push_back(row);return matrix_out(all,out_rows,out_cols);});}
int coolbox_dist_compute_accuracy(const double*p,size_t pr,size_t pc,const double*l,size_t lr,size_t lc,void*ctx,double*out,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(pr!=lr||pc!=lc||!out)throw std::invalid_argument("predictions and labels must have the same shape");*out=distributed::utils::compute_distributed_accuracy(matrix(p,pr,pc),matrix(l,lr,lc),context(ctx));});}
}

extern "C" int coolbox_dist_message_set_properties(void*p,int type,int src,int dst,char*e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(type<0||type>7)throw std::invalid_argument("invalid message type");auto&m=coolbox_jbridge::checked<Message>(p,"message");m.type=static_cast<MessageType>(type);m.source_rank=src;m.dest_rank=dst;});}
