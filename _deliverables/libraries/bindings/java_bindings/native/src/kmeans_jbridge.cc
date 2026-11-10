#include "coolbox_jbridge.h"
#include "kmeans/kmeans.h"
#include <Eigen/Dense>

namespace {
using ml::kmeans::KMeans;
Eigen::MatrixXd matrix_of(const double* p,size_t rows,size_t cols) { auto v=coolbox_jbridge::rows_of(p,rows,cols); Eigen::MatrixXd m(rows,cols); for(size_t r=0;r<rows;++r) for(size_t c=0;c<cols;++c) m(r,c)=v[r][c]; return m; }
double* matrix_out(const Eigen::MatrixXd& m,size_t* rows,size_t* cols) { std::vector<double> v; for(Eigen::Index r=0;r<m.rows();++r) for(Eigen::Index c=0;c<m.cols();++c) v.push_back(m(r,c)); if(rows)*rows=m.rows(); if(cols)*cols=m.cols(); return coolbox_jbridge::malloc_copy(v,nullptr); }
}
extern "C" {
void* coolbox_kmeans_new(int clusters,int max_iter,int init,int seed,char* e,size_t c) { return coolbox_jbridge::invoke_handle(e,c,[&]{ if(init!=0&&init!=1) throw std::invalid_argument("init must be RANDOM (0) or KMEANSPP (1)"); return new KMeans(clusters,max_iter,static_cast<ml::kmeans::InitMethod>(init),seed); }); }
void coolbox_kmeans_free(void* h) { delete static_cast<KMeans*>(h); }
int coolbox_kmeans_fit(void* h,const double* x,size_t rows,size_t cols,char* e,size_t c) { return coolbox_jbridge::invoke(e,c,[&]{ coolbox_jbridge::checked<KMeans>(h,"KMeans").fit(matrix_of(x,rows,cols)); }); }
int* coolbox_kmeans_predict(void* h,const double* x,size_t rows,size_t cols,size_t* n,char* e,size_t c) { try { return coolbox_jbridge::malloc_copy(coolbox_jbridge::checked<KMeans>(h,"KMeans").predict(matrix_of(x,rows,cols)),n); } catch(const std::exception& x) { coolbox_jbridge::write_error(e,c,x.what()); return nullptr; } }
int* coolbox_kmeans_fit_predict(void* h,const double* x,size_t rows,size_t cols,size_t* n,char* e,size_t c) { try { return coolbox_jbridge::malloc_copy(coolbox_jbridge::checked<KMeans>(h,"KMeans").fit_predict(matrix_of(x,rows,cols)),n); } catch(const std::exception& x) { coolbox_jbridge::write_error(e,c,x.what()); return nullptr; } }
double* coolbox_kmeans_centroids(void* h,size_t* rows,size_t* cols,char* e,size_t c) { try { return matrix_out(coolbox_jbridge::checked<KMeans>(h,"KMeans").get_centroids(),rows,cols); } catch(const std::exception& x) { coolbox_jbridge::write_error(e,c,x.what()); return nullptr; } }
int* coolbox_kmeans_labels(void* h,size_t* n,char* e,size_t c) { try { return coolbox_jbridge::malloc_copy(coolbox_jbridge::checked<KMeans>(h,"KMeans").get_labels(),n); } catch(const std::exception& x) { coolbox_jbridge::write_error(e,c,x.what()); return nullptr; } }
double coolbox_kmeans_inertia(void* h,char* e,size_t c) { try { return coolbox_jbridge::checked<KMeans>(h,"KMeans").get_inertia(); } catch(const std::exception& x) { coolbox_jbridge::write_error(e,c,x.what()); return 0; } }
double* coolbox_kmeans_inertia_history(void* h,size_t* n,char* e,size_t c) { try { return coolbox_jbridge::malloc_copy(coolbox_jbridge::checked<KMeans>(h,"KMeans").get_inertia_history(),n); } catch(const std::exception& x) { coolbox_jbridge::write_error(e,c,x.what()); return nullptr; } }
int coolbox_kmeans_n_iter(void* h,int* out,char* e,size_t c) { return coolbox_jbridge::invoke(e,c,[&]{ if(!out) throw std::invalid_argument("output must not be null"); *out=coolbox_jbridge::checked<KMeans>(h,"KMeans").get_n_iter(); }); }
int coolbox_kmeans_is_fitted(void* h) { return h&&static_cast<KMeans*>(h)->is_fitted(); }
}
