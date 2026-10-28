#include "coolbox_jbridge.h"
#include "dimensionality_reduction/svd.h"
#include "dimensionality_reduction/pca.h"
#include "dimensionality_reduction/knn.h"
#include "dimensionality_reduction/umap.h"
#include <Eigen/Dense>

namespace {
using namespace dimensionality_reduction;
Eigen::MatrixXd matrix_of(const double* p,size_t rows,size_t cols) { auto v=coolbox_jbridge::rows_of(p,rows,cols); Eigen::MatrixXd m(rows,cols); for(size_t r=0;r<rows;++r) for(size_t c=0;c<cols;++c) m(r,c)=v[r][c]; return m; }
double* matrix_out(const Eigen::MatrixXd& m,size_t* rows,size_t* cols) { std::vector<double> v; v.reserve(static_cast<size_t>(m.rows()*m.cols())); for(Eigen::Index r=0;r<m.rows();++r) for(Eigen::Index c=0;c<m.cols();++c) v.push_back(m(r,c)); if(rows)*rows=static_cast<size_t>(m.rows()); if(cols)*cols=static_cast<size_t>(m.cols()); return coolbox_jbridge::malloc_copy(v,nullptr); }
double* vector_out(const Eigen::VectorXd& v,size_t* n) { std::vector<double> out(v.data(),v.data()+v.size()); return coolbox_jbridge::malloc_copy(out,n); }
struct KnnResults { Eigen::MatrixXi indices; Eigen::MatrixXd distances; };
int* indices_out(const Eigen::MatrixXi& m,size_t* rows,size_t* cols) { std::vector<int> v; v.reserve(static_cast<size_t>(m.rows()*m.cols())); for(Eigen::Index r=0;r<m.rows();++r) for(Eigen::Index c=0;c<m.cols();++c) v.push_back(m(r,c)); if(rows)*rows=m.rows(); if(cols)*cols=m.cols(); return coolbox_jbridge::malloc_copy(v,nullptr); }
}
extern "C" {
void* coolbox_dimred_svd_new(int full,char* e,size_t c) { return coolbox_jbridge::invoke_handle(e,c,[&]{ return new SVD(full!=0); }); }
void coolbox_dimred_svd_free(void* h) { delete static_cast<SVD*>(h); }
int coolbox_dimred_svd_compute(void* h,const double* x,size_t r,size_t c,char* e,size_t cap) { return coolbox_jbridge::invoke(e,cap,[&]{ coolbox_jbridge::checked<SVD>(h,"SVD").compute(matrix_of(x,r,c)); }); }
double* coolbox_dimred_svd_u(void* h,size_t* r,size_t* c,char* e,size_t cap) { try{return matrix_out(coolbox_jbridge::checked<SVD>(h,"SVD").get_U(),r,c);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
double* coolbox_dimred_svd_values(void* h,size_t* n,char* e,size_t cap) { try{return vector_out(coolbox_jbridge::checked<SVD>(h,"SVD").get_singular_values(),n);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
double* coolbox_dimred_svd_v(void* h,size_t* r,size_t* c,char* e,size_t cap) { try{return matrix_out(coolbox_jbridge::checked<SVD>(h,"SVD").get_V(),r,c);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
double* coolbox_dimred_svd_s(void* h,size_t* r,size_t* c,char* e,size_t cap) { try{return matrix_out(coolbox_jbridge::checked<SVD>(h,"SVD").get_S(),r,c);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
double* coolbox_dimred_svd_reconstruct(void* h,int n,size_t* r,size_t* c,char* e,size_t cap) { try{return matrix_out(coolbox_jbridge::checked<SVD>(h,"SVD").reconstruct(n),r,c);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
int coolbox_dimred_svd_rank(void* h,double t,char* e,size_t cap) { try{return coolbox_jbridge::checked<SVD>(h,"SVD").rank(t);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return -1;} }
double* coolbox_dimred_svd_variance(void* h,size_t* n,char* e,size_t cap) { try{return vector_out(coolbox_jbridge::checked<SVD>(h,"SVD").explained_variance_ratio(),n);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
int coolbox_dimred_svd_is_computed(void* h) { return h&&static_cast<SVD*>(h)->is_computed(); }
void* coolbox_dimred_pca_new(int n,int center,int scale,char* e,size_t c) { return coolbox_jbridge::invoke_handle(e,c,[&]{ return new PCA(n,center!=0,scale!=0); }); }
void coolbox_dimred_pca_free(void* h) { delete static_cast<PCA*>(h); }
int coolbox_dimred_pca_fit(void* h,const double* x,size_t r,size_t c,char* e,size_t cap) { return coolbox_jbridge::invoke(e,cap,[&]{ coolbox_jbridge::checked<PCA>(h,"PCA").fit(matrix_of(x,r,c)); }); }
double* coolbox_dimred_pca_transform(void* h,const double* x,size_t r,size_t c,size_t* ro,size_t* co,char* e,size_t cap) { try{return matrix_out(coolbox_jbridge::checked<PCA>(h,"PCA").transform(matrix_of(x,r,c)),ro,co);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
double* coolbox_dimred_pca_fit_transform(void* h,const double* x,size_t r,size_t c,size_t* ro,size_t* co,char* e,size_t cap) { try{return matrix_out(coolbox_jbridge::checked<PCA>(h,"PCA").fit_transform(matrix_of(x,r,c)),ro,co);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
double* coolbox_dimred_pca_inverse(void* h,const double* x,size_t r,size_t c,size_t* ro,size_t* co,char* e,size_t cap) { try{return matrix_out(coolbox_jbridge::checked<PCA>(h,"PCA").inverse_transform(matrix_of(x,r,c)),ro,co);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
double* coolbox_dimred_pca_components(void* h,size_t* r,size_t* c,char* e,size_t cap) { try{return matrix_out(coolbox_jbridge::checked<PCA>(h,"PCA").get_components(),r,c);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
double* coolbox_dimred_pca_explained(void* h,size_t* n,char* e,size_t cap) { try{return vector_out(coolbox_jbridge::checked<PCA>(h,"PCA").get_explained_variance(),n);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
double* coolbox_dimred_pca_ratio(void* h,size_t* n,char* e,size_t cap) { try{return vector_out(coolbox_jbridge::checked<PCA>(h,"PCA").get_explained_variance_ratio(),n);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
double* coolbox_dimred_pca_singular(void* h,size_t* n,char* e,size_t cap) { try{return vector_out(coolbox_jbridge::checked<PCA>(h,"PCA").get_singular_values(),n);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
double* coolbox_dimred_pca_mean(void* h,size_t* n,char* e,size_t cap) { try{return vector_out(coolbox_jbridge::checked<PCA>(h,"PCA").get_mean(),n);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
double* coolbox_dimred_pca_scale(void* h,size_t* n,char* e,size_t cap) { try{return vector_out(coolbox_jbridge::checked<PCA>(h,"PCA").get_scale(),n);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
int coolbox_dimred_pca_n_components(void* h) { return h?static_cast<PCA*>(h)->get_n_components():0; }
int coolbox_dimred_pca_is_fitted(void* h) { return h&&static_cast<PCA*>(h)->is_fitted(); }
void* coolbox_dimred_knn_new(int k,const char* metric,char* e,size_t c) { return coolbox_jbridge::invoke_handle(e,c,[&]{ return new KNN(k,coolbox_jbridge::str(metric)); }); }
void coolbox_dimred_knn_free(void* h) { delete static_cast<KNN*>(h); }
int coolbox_dimred_knn_fit(void* h,const double* x,size_t r,size_t c,char* e,size_t cap) { return coolbox_jbridge::invoke(e,cap,[&]{ coolbox_jbridge::checked<KNN>(h,"KNN").fit(matrix_of(x,r,c)); }); }
int coolbox_dimred_knn_neighbors(void* h,const double* x,size_t r,size_t c,int training,int** indices,double** distances,size_t* ro,size_t* co,char* e,size_t cap) { return coolbox_jbridge::invoke(e,cap,[&]{ if(!indices||!distances) throw std::invalid_argument("neighbor outputs must not be null"); auto result=training?coolbox_jbridge::checked<KNN>(h,"KNN").kneighbors():coolbox_jbridge::checked<KNN>(h,"KNN").kneighbors(matrix_of(x,r,c)); *indices=indices_out(result.first,ro,co); size_t dr=0,dc=0; *distances=matrix_out(result.second,&dr,&dc); if(dr!=*ro||dc!=*co){std::free(*indices);std::free(*distances);*indices=nullptr;*distances=nullptr;throw std::runtime_error("neighbor result dimensions differ");} }); }
double* coolbox_dimred_knn_pairwise(void* h,const double* x,size_t r,size_t c,const double* y,size_t yr,size_t yc,size_t* ro,size_t* co,char* e,size_t cap) { try{return matrix_out(coolbox_jbridge::checked<KNN>(h,"KNN").pairwise_distances(matrix_of(x,r,c),matrix_of(y,yr,yc)),ro,co);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
int coolbox_dimred_knn_get_k(void* h) { return h?static_cast<KNN*>(h)->get_k():0; }
char* coolbox_dimred_knn_get_metric(void* h,char* e,size_t cap) { try{return coolbox_jbridge::dup_string(coolbox_jbridge::checked<KNN>(h,"KNN").get_metric());}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
int coolbox_dimred_knn_is_fitted(void* h) { return h&&static_cast<KNN*>(h)->is_fitted(); }
void* coolbox_dimred_umap_new(int components,int neighbors,double min_dist,const char* metric,double learning_rate,int epochs,int seed,char* e,size_t c) { return coolbox_jbridge::invoke_handle(e,c,[&]{ return new UMAP(components,neighbors,min_dist,coolbox_jbridge::str(metric),learning_rate,epochs,seed); }); }
void coolbox_dimred_umap_free(void* h) { delete static_cast<UMAP*>(h); }
int coolbox_dimred_umap_fit(void* h,const double* x,size_t r,size_t c,char* e,size_t cap) { return coolbox_jbridge::invoke(e,cap,[&]{ coolbox_jbridge::checked<UMAP>(h,"UMAP").fit(matrix_of(x,r,c)); }); }
double* coolbox_dimred_umap_transform(void* h,const double* x,size_t r,size_t c,size_t* ro,size_t* co,char* e,size_t cap) { try{return matrix_out(coolbox_jbridge::checked<UMAP>(h,"UMAP").transform(matrix_of(x,r,c)),ro,co);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
double* coolbox_dimred_umap_fit_transform(void* h,const double* x,size_t r,size_t c,size_t* ro,size_t* co,char* e,size_t cap) { try{return matrix_out(coolbox_jbridge::checked<UMAP>(h,"UMAP").fit_transform(matrix_of(x,r,c)),ro,co);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
double* coolbox_dimred_umap_embedding(void* h,size_t* r,size_t* c,char* e,size_t cap) { try{return matrix_out(coolbox_jbridge::checked<UMAP>(h,"UMAP").get_embedding(),r,c);}catch(const std::exception& x){coolbox_jbridge::write_error(e,cap,x.what());return nullptr;} }
int coolbox_dimred_umap_is_fitted(void* h) { return h&&static_cast<UMAP*>(h)->is_fitted(); }
int coolbox_dimred_umap_n_components(void* h) { return h?static_cast<UMAP*>(h)->get_n_components():0; }
int coolbox_dimred_umap_n_neighbors(void* h) { return h?static_cast<UMAP*>(h)->get_n_neighbors():0; }
}
