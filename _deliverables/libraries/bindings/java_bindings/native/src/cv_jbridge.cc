#include "coolbox_jbridge.h"
#include <computer_vision/image.h>
#include <computer_vision/transforms.h>
#include <computer_vision/pipeline.h>
#include <computer_vision/layers.h>
#include <DL/layers/include/tensor.h>
#include <DL/layers/include/layer.h>
#include <memory>
#include <algorithm>

using namespace ml::cv;
using ml::deep_learning::Layer;
namespace {
Image& image(void* p) { return coolbox_jbridge::checked<Image>(p, "image"); }
std::shared_ptr<Transform>& transform(void* p) { return coolbox_jbridge::checked<std::shared_ptr<Transform>>(p,"transform"); }
TransformPipeline& pipeline(void* p) { return coolbox_jbridge::checked<TransformPipeline>(p,"pipeline"); }
std::shared_ptr<Layer>* layer_ptr(std::shared_ptr<Layer> p) { return new std::shared_ptr<Layer>(std::move(p)); }
std::vector<float> fvec(const float* p,size_t n) { if(n && !p) throw std::invalid_argument("input array must not be null"); return n?std::vector<float>(p,p+n):std::vector<float>(); }
}
extern "C" {
void* coolbox_cv_image_new(int h,int w,int fmt,char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]{if(fmt<0||fmt>2)throw std::invalid_argument("invalid image format");return new Image(h,w,static_cast<ImageFormat>(fmt));});}
void* coolbox_cv_image_new_data(int h,int w,int ch,const float* data,size_t n,char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]{return new Image(h,w,ch,fvec(data,n));});}
void coolbox_cv_image_free(void* p){delete static_cast<Image*>(p);}
int coolbox_cv_image_height(void* p){return image(p).height();}
int coolbox_cv_image_width(void* p){return image(p).width();}
int coolbox_cv_image_channels(void* p){return image(p).channels();}
int coolbox_cv_image_format(void* p){return static_cast<int>(image(p).format());}
size_t coolbox_cv_image_size(void* p){return image(p).size();}
int coolbox_cv_image_data(void* p,float* out,size_t n,char* e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{auto& d=image(p).data();if(!out||n<d.size())throw std::invalid_argument("image data buffer too small");std::copy(d.begin(),d.end(),out);});}
int coolbox_cv_image_at(void* p,int r,int col,int ch,float* out,char* e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(!out)throw std::invalid_argument("image value output is null");*out=image(p).at(r,col,ch);});}
int coolbox_cv_image_set(void* p,int r,int col,int ch,float value,char* e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{image(p).at(r,col,ch)=value;});}
int coolbox_cv_image_fill(void* p,float value,char* e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{image(p).fill(value);});}
int coolbox_cv_image_fill_channels(void* p,const float* v,size_t n,char* e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{image(p).fill(fvec(v,n));});}
void* coolbox_cv_image_clone(void* p,char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]{return new Image(image(p).clone());});}
void* coolbox_cv_image_grayscale(void* p,char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]{return new Image(image(p).to_grayscale());});}
void* coolbox_cv_image_rgb(void* p,char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]{return new Image(image(p).to_rgb());});}
int coolbox_cv_image_mean(void* p,float* out,size_t n,char* e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{auto v=image(p).mean();if(!out||n<v.size())throw std::invalid_argument("mean output buffer too small");std::copy(v.begin(),v.end(),out);});}
int coolbox_cv_image_std(void* p,float* out,size_t n,char* e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{auto v=image(p).std();if(!out||n<v.size())throw std::invalid_argument("std output buffer too small");std::copy(v.begin(),v.end(),out);});}
int coolbox_cv_image_min_max(void* p,float* lo,float* hi,char* e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(!lo||!hi)throw std::invalid_argument("min/max outputs are null");auto v=image(p).min_max();*lo=v.first;*hi=v.second;});}
void* coolbox_cv_transform_new(int kind,const int* ip,size_t ni,const float* fp,size_t nf,const float* mean,const float* stdv,size_t count,int mode,char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]()->void*{auto I=[&](size_t i){if(!ip||i>=ni)throw std::invalid_argument("missing transform parameter");return ip[i];};auto F=[&](size_t i){if(!fp||i>=nf)throw std::invalid_argument("missing transform parameter");return fp[i];};if(mode<0||mode>2)throw std::invalid_argument("invalid interpolation mode");auto m=static_cast<InterpolationMode>(mode);std::shared_ptr<Transform> t;switch(kind){case 0:t=std::make_shared<Resize>(I(0),I(1),m);break;case 1:t=std::make_shared<CenterCrop>(I(0),I(1));break;case 2:t=std::make_shared<RandomCrop>(I(0),I(1),static_cast<unsigned>(I(2)));break;case 3:t=std::make_shared<HorizontalFlip>();break;case 4:t=std::make_shared<VerticalFlip>();break;case 5:t=std::make_shared<RandomHorizontalFlip>(F(0),static_cast<unsigned>(I(0)));break;case 6:t=std::make_shared<Normalize>(fvec(mean,count),fvec(stdv,count));break;case 7:t=std::make_shared<Standardize>();break;case 8:t=std::make_shared<Rotate>(F(0),m);break;case 9:t=std::make_shared<RandomRotation>(F(0),F(1),m,static_cast<unsigned>(I(0)));break;case 10:t=std::make_shared<AdjustBrightness>(F(0));break;case 11:t=std::make_shared<AdjustContrast>(F(0));break;case 12:t=std::make_shared<GaussianBlur>(I(0),F(0));break;case 13:t=std::make_shared<Pad>(I(0),I(1),I(2),I(3),F(0));break;default:throw std::invalid_argument("unknown transform kind");}return new std::shared_ptr<Transform>(std::move(t));});}
void coolbox_cv_transform_free(void* p){delete static_cast<std::shared_ptr<Transform>*>(p);}
void* coolbox_cv_transform_apply(void* p,void* im,char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]{return new Image(transform(p)->apply(image(im)));});}
void* coolbox_cv_pipeline_new(char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[]{return new TransformPipeline();});}
void coolbox_cv_pipeline_free(void* p){delete static_cast<TransformPipeline*>(p);}
int coolbox_cv_pipeline_add(void* p,void* t,char* e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{pipeline(p).add(transform(t)->clone());});}
void* coolbox_cv_pipeline_apply(void* p,void* im,char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]{return new Image(pipeline(p).apply(image(im)));});}
size_t coolbox_cv_pipeline_size(void* p){return pipeline(p).size();}
void coolbox_cv_pipeline_clear(void* p){pipeline(p).clear();}
void* coolbox_cv_pipeline_imagenet(int size,char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]{return new TransformPipeline(create_imagenet_pipeline(size));});}
void* coolbox_cv_pipeline_training(int size,int flip,int rotation,int brightness,int contrast,char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]{return new TransformPipeline(create_training_augmentation_pipeline(size,flip!=0,rotation!=0,brightness!=0,contrast!=0));});}
void* coolbox_cv_pipeline_inference(int size,const float* mean,const float* stdv,size_t n,char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]{return new TransformPipeline(create_inference_pipeline(size,fvec(mean,n),fvec(stdv,n)));});}
void* coolbox_cv_image_to_tensor(void* p,char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]{return new ml::deep_learning::Tensor(image_to_tensor(image(p)));});}
void* coolbox_cv_tensor_to_image(void* p,int fmt,char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]{if(!p||fmt<0||fmt>2)throw std::invalid_argument("invalid tensor or image format");return new Image(tensor_to_image(*static_cast<ml::deep_learning::Tensor*>(p),static_cast<ImageFormat>(fmt)));});}
void* coolbox_cv_layer_new(int kind,const int* v,size_t n,float eps,float momentum,char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]()->void*{auto at=[&](size_t i){if(!v||i>=n)throw std::invalid_argument("missing layer parameter");return v[i];};std::shared_ptr<Layer> p;switch(kind){case 0:p=std::make_shared<Conv2DLayer>(at(0),at(1),at(2),at(3),at(4));break;case 1:p=std::make_shared<MaxPool2DLayer>(at(0),at(1));break;case 2:p=std::make_shared<AvgPool2DLayer>(at(0),at(1));break;case 3:p=std::make_shared<BatchNorm2DLayer>(at(0),eps,momentum);break;case 4:p=std::make_shared<FlattenLayer>();break;case 5:p=std::make_shared<GlobalAvgPool2DLayer>();break;default:throw std::invalid_argument("unknown vision layer kind");}return layer_ptr(std::move(p));});}
int coolbox_cv_layer_property(void* p,int kind){if(!p)return 0;auto l=*static_cast<std::shared_ptr<Layer>*>(p);if(kind==0){if(auto x=dynamic_cast<Conv2DLayer*>(l.get()))return x->in_channels();}if(kind==1){if(auto x=dynamic_cast<Conv2DLayer*>(l.get()))return x->out_channels();}if(kind==2){if(auto x=dynamic_cast<Conv2DLayer*>(l.get()))return x->kernel_size();if(auto x=dynamic_cast<MaxPool2DLayer*>(l.get()))return x->kernel_size();if(auto x=dynamic_cast<AvgPool2DLayer*>(l.get()))return x->kernel_size();}if(kind==3){if(auto x=dynamic_cast<Conv2DLayer*>(l.get()))return x->stride();if(auto x=dynamic_cast<MaxPool2DLayer*>(l.get()))return x->stride();if(auto x=dynamic_cast<AvgPool2DLayer*>(l.get()))return x->stride();}if(kind==4){if(auto x=dynamic_cast<Conv2DLayer*>(l.get()))return x->padding();}return 0;}
int coolbox_cv_layer_set_training(void* p,int training,char* e,size_t c){return coolbox_jbridge::invoke(e,c,[&]{if(!p)throw std::invalid_argument("layer is closed");auto l=*static_cast<std::shared_ptr<Layer>*>(p);auto* b=dynamic_cast<BatchNorm2DLayer*>(l.get());if(!b)throw std::invalid_argument("layer does not support training mode");b->set_training(training!=0);});}
int coolbox_cv_layer_is_training(void* p){if(!p)return 0;auto l=*static_cast<std::shared_ptr<Layer>*>(p);auto* b=dynamic_cast<BatchNorm2DLayer*>(l.get());return b&&b->is_training();}
}
extern "C" void* coolbox_cv_pipeline_clone(void* p,char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[&]{return new TransformPipeline(pipeline(p).clone());});}
extern "C" void* coolbox_cv_image_default_new(char* e,size_t c){return coolbox_jbridge::invoke_handle(e,c,[]{return new Image();});}
