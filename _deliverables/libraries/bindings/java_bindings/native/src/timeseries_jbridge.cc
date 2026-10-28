#include "coolbox_jbridge.h"
#include "time_series.h"
#include "arma.h"
#include "arima.h"
#include <memory>

using namespace coolbox_jbridge;
namespace { std::vector<std::string> strings(const char* const* p,size_t n){std::vector<std::string> v;if(n&&!p)throw std::invalid_argument("string input is null");for(size_t i=0;i<n;++i)v.emplace_back(str(p[i]));return v;} }
using ml::time_series::TimeSeries;
using ml::time_series::MultivariatTimeSeries;

extern "C" {
void* coolbox_timeseries_new(const double* values, size_t count, const char* const* timestamps, size_t timestamp_count, char* error, size_t error_capacity) {
    return invoke_handle(error, error_capacity, [&]() -> void* {
        if (count && !values) throw std::invalid_argument("values must not be null");
        if (timestamp_count && (!timestamps || timestamp_count != count)) throw std::invalid_argument("timestamps must match values");
        std::vector<std::string> stamps;
        for (size_t i=0;i<timestamp_count;++i) stamps.emplace_back(str(timestamps[i]));
        return new TimeSeries(vec(values,count), stamps);
    });
}
void coolbox_timeseries_free(void* h) { delete static_cast<TimeSeries*>(h); }
int coolbox_timeseries_at(const void* h, size_t i, double* out, char* e, size_t n) { return invoke(e,n,[&]{ if(!out) throw std::invalid_argument("output is null"); *out=checked<const TimeSeries>(const_cast<void*>(h),"time series").at(i); }); }
char* coolbox_timeseries_timestamp(const void* h, size_t i, char* e, size_t n) { try { return dup_string(checked<const TimeSeries>(const_cast<void*>(h),"time series").timestamp_at(i)); } catch(const std::exception& x){ write_error(e,n,x.what()); return nullptr; } }
size_t coolbox_timeseries_window_count(const void* h, size_t width, size_t stride, char* e, size_t n) { try { if(!stride) throw std::invalid_argument("stride must be positive"); return checked<const TimeSeries>(const_cast<void*>(h),"time series").create_windows(width,stride).size(); } catch(const std::exception& x){ write_error(e,n,x.what()); return 0; } }
int coolbox_timeseries_windows(const void* h, size_t width, size_t stride, double* out, size_t cap, char* e, size_t n) { return invoke(e,n,[&]{ auto w=checked<const TimeSeries>(const_cast<void*>(h),"time series").create_windows(width,stride); size_t need=w.size()*width; if(cap<need || (need&&!out)) throw std::invalid_argument("window output buffer too small"); size_t k=0; for(auto& r:w) for(double x:r) out[k++]=x; }); }
int coolbox_timeseries_decomposition(const void* h, size_t period, int component, double* out, size_t cap, char* e, size_t n) { return invoke(e,n,[&]{ auto d=ml::time_series::seasonal_decompose(checked<const TimeSeries>(const_cast<void*>(h),"time series"),period); const std::vector<double>* v=component==0?&d.trend.values():component==1?&d.seasonal.values():component==2?&d.residual.values():nullptr; if(!v) throw std::invalid_argument("invalid decomposition component"); if(cap<v->size() || (!v->empty()&&!out)) throw std::invalid_argument("output buffer too small"); std::copy(v->begin(),v->end(),out); }); }
double* coolbox_timeseries_outliers(const void* h, int method, double parameter, size_t* count, char* e, size_t n) { try { auto& ts=checked<const TimeSeries>(const_cast<void*>(h),"time series"); auto v=method==0?ml::time_series::detect_outliers_zscore(ts,parameter):method==1?ml::time_series::detect_outliers_iqr(ts,parameter):throw std::invalid_argument("invalid outlier method"); return malloc_copy(v,count); } catch(const std::exception& x){write_error(e,n,x.what());return nullptr;} }
void* coolbox_timeseries_interpolate(const void* h, const size_t* indices, size_t count, char* e, size_t n) { return invoke_handle(e,n,[&]()->void*{ if(count&&!indices) throw std::invalid_argument("indices must not be null"); std::vector<size_t> ix; if(count) ix.assign(indices,indices+count); return new TimeSeries(ml::time_series::interpolate_missing(checked<const TimeSeries>(const_cast<void*>(h),"time series"),ix)); }); }
void* coolbox_multits_new(const double* values, size_t features, size_t samples, const char* const* names, size_t name_count, const char* const* timestamps, size_t timestamp_count, char* e, size_t n) { return invoke_handle(e,n,[&]()->void*{ if(name_count && name_count!=features) throw std::invalid_argument("feature names must match feature count"); if(timestamp_count && timestamp_count!=samples) throw std::invalid_argument("timestamps must match sample count"); return new MultivariatTimeSeries(rows_of(values,features,samples),strings(names,name_count),strings(timestamps,timestamp_count)); }); }
void coolbox_multits_free(void* h) { delete static_cast<MultivariatTimeSeries*>(h); }
size_t coolbox_multits_features(const void* h){return checked<const MultivariatTimeSeries>(const_cast<void*>(h),"multivariate series").num_features();}
size_t coolbox_multits_samples(const void* h){return checked<const MultivariatTimeSeries>(const_cast<void*>(h),"multivariate series").num_samples();}
int coolbox_multits_data(const void* h,double* out,size_t cap,char* e,size_t n){return invoke(e,n,[&]{auto& d=checked<const MultivariatTimeSeries>(const_cast<void*>(h),"multivariate series").data();size_t need=0;for(auto& r:d)need+=r.size();if(cap<need||(need&&!out))throw std::invalid_argument("output buffer too small");size_t k=0;for(auto& r:d)for(double x:r)out[k++]=x;});}
int coolbox_multits_feature(const void* h,size_t i,double* out,size_t cap,char* e,size_t n){return invoke(e,n,[&]{auto& v=checked<const MultivariatTimeSeries>(const_cast<void*>(h),"multivariate series").feature(i);if(cap<v.size()||(!v.empty()&&!out))throw std::invalid_argument("output buffer too small");std::copy(v.begin(),v.end(),out);});}
double* coolbox_multits_sample(const void* h,size_t i,size_t* count,char* e,size_t n){try{return malloc_copy(checked<const MultivariatTimeSeries>(const_cast<void*>(h),"multivariate series").sample(i),count);}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
int coolbox_multits_at(const void* h,size_t f,size_t s,double* out,char* e,size_t n){return invoke(e,n,[&]{if(!out)throw std::invalid_argument("output is null");*out=checked<const MultivariatTimeSeries>(const_cast<void*>(h),"multivariate series").at(f,s);});}
double* coolbox_multits_stats(const void* h,int stddev,size_t* count,char* e,size_t n){try{auto& x=checked<const MultivariatTimeSeries>(const_cast<void*>(h),"multivariate series");return malloc_copy(stddev?x.stds():x.means(),count);}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}
void* coolbox_multits_transform(const void* h,int scale,char* e,size_t n){return invoke_handle(e,n,[&]()->void*{auto& x=checked<const MultivariatTimeSeries>(const_cast<void*>(h),"multivariate series");return new MultivariatTimeSeries(scale?x.min_max_scale():x.normalize());});}
size_t coolbox_multits_window_count(const void* h,size_t width,size_t stride,char* e,size_t n){try{if(!stride)throw std::invalid_argument("stride must be positive");return checked<const MultivariatTimeSeries>(const_cast<void*>(h),"multivariate series").create_windows(width,stride).size();}catch(const std::exception& x){write_error(e,n,x.what());return 0;}}
int coolbox_multits_windows(const void* h,size_t width,size_t stride,double* out,size_t cap,char* e,size_t n){return invoke(e,n,[&]{auto& x=checked<const MultivariatTimeSeries>(const_cast<void*>(h),"multivariate series");auto w=x.create_windows(width,stride);size_t need=w.size()*x.num_features()*width;if(cap<need||(need&&!out))throw std::invalid_argument("output buffer too small");size_t k=0;for(auto& win:w)for(auto& row:win)for(double v:row)out[k++]=v;});}
struct JavaArHandle { ml::time_series::AutoRegressiveModel model; explicit JavaArHandle(size_t order):model(order){} bool fitted=false; };
void* coolbox_timeseries_ar_new(size_t order){try{return new JavaArHandle(order);}catch(...){return nullptr;}}
void coolbox_timeseries_ar_free(void* h){delete static_cast<JavaArHandle*>(h);}
int coolbox_timeseries_ar_fit(void* h,const double* values,size_t count,char* e,size_t n){return invoke(e,n,[&]{if(count&&!values)throw std::invalid_argument("observations are null");auto& x=checked<JavaArHandle>(h,"AR model");x.model.fit(TimeSeries(vec(values,count)));x.fitted=true;});}
int coolbox_timeseries_ar_forecast(void* h,size_t steps,double* out,size_t cap,char* e,size_t n){return invoke(e,n,[&]{auto& x=checked<JavaArHandle>(h,"AR model");if(!x.fitted)throw std::logic_error("forecaster has not been fitted");auto v=x.model.forecast(steps);if(cap<v.size()||(!v.empty()&&!out))throw std::invalid_argument("output buffer too small");std::copy(v.begin(),v.end(),out);});}
double* coolbox_timeseries_ar_coefficients(const void* h,size_t* count,char* e,size_t n){try{auto& x=checked<const JavaArHandle>(const_cast<void*>(h),"AR model");if(!x.fitted)throw std::logic_error("forecaster has not been fitted");return malloc_copy(x.model.coefficients(),count);}catch(const std::exception& x){write_error(e,n,x.what());return nullptr;}}}


