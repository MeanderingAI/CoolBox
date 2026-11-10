#include "coolbox_jbridge.h"
#include <dl_bridge.h>
#include <DL/layers/include/layer.h>
#include <DL/loss/include/loss.h>
#include <DL/optimizer/include/optimizer.h>
#include <DL/wrapper/include/neural_network.h>
#include <memory>
#include <vector>

using ml::deep_learning::NeuralNetwork;
using ml::deep_learning::Layer;
using ml::deep_learning::DenseLayer;
using ml::deep_learning::ReLULayer;
using ml::deep_learning::SigmoidLayer;
using ml::deep_learning::TanhLayer;
using ml::deep_learning::SoftmaxLayer;
using ml::deep_learning::DropoutLayer;
using ml::deep_learning::MSELoss;
using ml::deep_learning::BCELoss;
using ml::deep_learning::CategoricalCrossEntropyLoss;
using ml::deep_learning::Adam;

namespace {
std::shared_ptr<Layer> dense(size_t in, size_t out) { return std::make_shared<DenseLayer>(in, out); }
void add_hidden(NeuralNetwork& n, int input, const int* dims, size_t count, bool tanh = false) {
    size_t prev = static_cast<size_t>(input);
    for (size_t i = 0; i < count; ++i) {
        if (!dims || dims[i] <= 0) throw std::invalid_argument("hidden dimensions must be positive");
        n.add_layer(dense(prev, static_cast<size_t>(dims[i])));
        n.add_layer(tanh ? std::static_pointer_cast<Layer>(std::make_shared<TanhLayer>())
                         : std::static_pointer_cast<Layer>(std::make_shared<ReLULayer>()));
        prev = static_cast<size_t>(dims[i]);
    }
}
void add_loss_adam(NeuralNetwork& n, int loss, double lr = 0.001) {
    if (loss == 0) n.set_loss(std::make_shared<MSELoss>());
    else if (loss == 1) n.set_loss(std::make_shared<BCELoss>());
    else n.set_loss(std::make_shared<CategoricalCrossEntropyLoss>());
    n.set_optimizer(std::make_shared<Adam>(lr));
}
template<class F> void* make_network(char* e, size_t c, F f) {
    return coolbox_jbridge::invoke_handle(e, c, [&]() -> void* { return new NeuralNetwork(f()); });
}
}
extern "C" {
void* coolbox_dl_template_binary_classifier(int input_dim, const int* hidden, size_t count, char* e, size_t c) {
    return make_network(e,c,[&]{ NeuralNetwork n; if(input_dim<=0) throw std::invalid_argument("input_dim must be positive"); add_hidden(n,input_dim,hidden,count); size_t prev=count?static_cast<size_t>(hidden[count-1]):static_cast<size_t>(input_dim); n.add_layer(dense(prev,1)); n.add_layer(std::make_shared<SigmoidLayer>()); add_loss_adam(n,1); return n; });
}
void* coolbox_dl_template_multiclass_classifier(int input_dim,int classes,const int* hidden,size_t count,char* e,size_t c) {
    return make_network(e,c,[&]{ NeuralNetwork n; if(input_dim<=0||classes<=0) throw std::invalid_argument("dimensions must be positive"); add_hidden(n,input_dim,hidden,count); size_t prev=count?static_cast<size_t>(hidden[count-1]):static_cast<size_t>(input_dim); n.add_layer(dense(prev,classes)); n.add_layer(std::make_shared<SoftmaxLayer>()); add_loss_adam(n,2); return n; });
}
void* coolbox_dl_template_image_classifier(int classes,int channels,int height,int width,const char* arch,char* e,size_t c) {
    (void)arch; return make_network(e,c,[&]{ NeuralNetwork n; if(classes<=0||channels<=0||height<=0||width<=0) throw std::invalid_argument("image dimensions must be positive"); size_t in=static_cast<size_t>(channels)*height*width; n.add_layer(dense(in,256)); n.add_layer(std::make_shared<ReLULayer>()); n.add_layer(std::make_shared<DropoutLayer>(.25)); n.add_layer(dense(256,128)); n.add_layer(std::make_shared<ReLULayer>()); n.add_layer(dense(128,classes)); n.add_layer(std::make_shared<SoftmaxLayer>()); add_loss_adam(n,2); return n; });
}
void* coolbox_dl_template_regressor(int in,int out,const int* hidden,size_t count,char* e,size_t c) {
    return make_network(e,c,[&]{ NeuralNetwork n; if(in<=0||out<=0) throw std::invalid_argument("dimensions must be positive"); add_hidden(n,in,hidden,count); size_t prev=count?static_cast<size_t>(hidden[count-1]):static_cast<size_t>(in); n.add_layer(dense(prev,out)); add_loss_adam(n,0); return n; });
}
void* coolbox_dl_template_embedding_network(int in,int embedding,const int* hidden,size_t count,char* e,size_t c) {
    return make_network(e,c,[&]{ NeuralNetwork n; if(in<=0||embedding<=0) throw std::invalid_argument("dimensions must be positive"); add_hidden(n,in,hidden,count); size_t prev=count?static_cast<size_t>(hidden[count-1]):static_cast<size_t>(in); n.add_layer(dense(prev,embedding)); return n; });
}
void* coolbox_dl_template_sequence_classifier(int in,int classes,int hidden,int layers,char* e,size_t c) {
    return make_network(e,c,[&]{ NeuralNetwork n; if(in<=0||classes<=0||hidden<=0||layers<0) throw std::invalid_argument("dimensions must be positive"); size_t prev=in; for(int i=0;i<layers;++i){n.add_layer(dense(prev,hidden));n.add_layer(std::make_shared<TanhLayer>());prev=hidden;} n.add_layer(dense(prev,classes));n.add_layer(std::make_shared<SoftmaxLayer>());add_loss_adam(n,2);return n; });
}
void* coolbox_dl_template_simple_autoencoder(int in,int latent,const int* dims,size_t count,char* e,size_t c) {
    return make_network(e,c,[&]{ NeuralNetwork n; if(in<=0||latent<=0) throw std::invalid_argument("dimensions must be positive"); add_hidden(n,in,dims,count); size_t prev=count?static_cast<size_t>(dims[count-1]):static_cast<size_t>(in);n.add_layer(dense(prev,latent));prev=latent;for(size_t i=count;i>0;--i){if(dims[i-1]<=0)throw std::invalid_argument("hidden dimensions must be positive");n.add_layer(dense(prev,dims[i-1]));n.add_layer(std::make_shared<ReLULayer>());prev=dims[i-1];}n.add_layer(dense(prev,in));n.add_layer(std::make_shared<SigmoidLayer>());add_loss_adam(n,0);return n; });
}
void* coolbox_dl_template_variational_autoencoder(int in,int latent,const int* dims,size_t count,char* e,size_t c) {
    return make_network(e,c,[&]{ NeuralNetwork n; if(in<=0||latent<=0) throw std::invalid_argument("dimensions must be positive"); add_hidden(n,in,dims,count); size_t prev=count?static_cast<size_t>(dims[count-1]):static_cast<size_t>(in);n.add_layer(dense(prev,latent));prev=latent;for(size_t i=count;i>0;--i){n.add_layer(dense(prev,dims[i-1]));n.add_layer(std::make_shared<ReLULayer>());prev=dims[i-1];}n.add_layer(dense(prev,in));add_loss_adam(n,0);return n; });
}
void* coolbox_dl_template_simple_gan(int latent,int output,const int* gd,size_t gc,const int* dd,size_t dc,char* e,size_t c) {
    (void)dd;(void)dc;return make_network(e,c,[&]{NeuralNetwork n;if(latent<=0||output<=0)throw std::invalid_argument("dimensions must be positive");add_hidden(n,latent,gd,gc);size_t prev=gc?static_cast<size_t>(gd[gc-1]):static_cast<size_t>(latent);n.add_layer(dense(prev,output));n.add_layer(std::make_shared<TanhLayer>());return n;});
}
void* coolbox_dl_template_language_model(int vocab,int context,int embed,int heads,int layers,int ff,char* e,size_t c) {
    (void)context;(void)heads;return make_network(e,c,[&]{NeuralNetwork n;if(vocab<=0||embed<=0||layers<0||ff<=0)throw std::invalid_argument("dimensions must be positive");size_t prev=embed;for(int i=0;i<layers;++i){n.add_layer(dense(prev,ff));n.add_layer(std::make_shared<ReLULayer>());n.add_layer(std::make_shared<DropoutLayer>(.1));n.add_layer(dense(ff,embed));prev=embed;}n.add_layer(dense(prev,vocab));n.add_layer(std::make_shared<SoftmaxLayer>());add_loss_adam(n,2,.0001);return n;});
}
}
#include <DL/layers/include/layer.h>
#include <DL/optimizer/include/optimizer.h>

extern "C" {
void* coolbox_dl_optimizer_new_full(int kind,double learning_rate,double p1,double p2,double p3,char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]()->void*{using namespace ml::deep_learning;std::shared_ptr<Optimizer> o;if(kind==0)o=std::make_shared<SGD>(learning_rate,p1);else if(kind==1)o=std::make_shared<Adam>(learning_rate,p1,p2,p3);else if(kind==2)o=std::make_shared<RMSprop>(learning_rate,p1,p2);else throw std::invalid_argument("unknown optimizer kind");return new std::shared_ptr<Optimizer>(std::move(o));});}
int coolbox_dl_dense_values(void* p,int bias,double* out,size_t n,char* e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(!p)throw std::invalid_argument("layer is closed");auto base=*static_cast<std::shared_ptr<ml::deep_learning::Layer>*>(p);auto dense=dynamic_cast<ml::deep_learning::DenseLayer*>(base.get());if(!dense)throw std::invalid_argument("layer is not dense");const auto& v=bias?dense->bias().data():dense->weights().data();if(!out||n<v.size())throw std::invalid_argument("dense parameter buffer too small");std::copy(v.begin(),v.end(),out);});}
int coolbox_dl_dense_shape(void* p,int bias,size_t* rows,size_t* cols,char* e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(!p||!rows||!cols)throw std::invalid_argument("invalid dense layer output");auto base=*static_cast<std::shared_ptr<ml::deep_learning::Layer>*>(p);auto dense=dynamic_cast<ml::deep_learning::DenseLayer*>(base.get());if(!dense)throw std::invalid_argument("layer is not dense");const auto& s=bias?dense->bias().shape():dense->weights().shape();*rows=s.empty()?0:s[0];*cols=s.size()<2?1:s[1];});}
}
 
extern "C" int coolbox_dl_loss_name(void* handle,char* output,size_t capacity,char* error,size_t error_capacity){return coolbox_jbridge::invoke(error,error_capacity,[&]{if(!handle||!output||capacity==0)throw std::invalid_argument("invalid loss name output");auto loss=*static_cast<std::shared_ptr<ml::deep_learning::Loss>*>(handle);std::string name=loss->name();size_t n=std::min(capacity-1,name.size());std::memcpy(output,name.data(),n);output[n]='\0';});}
