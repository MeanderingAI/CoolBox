#include "coolbox_jbridge.h"
#include "tracker/kalman_filter.h"
#include <Eigen/Dense>

namespace {
Eigen::MatrixXd eigen_matrix(const double* p,size_t rows,size_t cols) { auto v=coolbox_jbridge::rows_of(p,rows,cols); Eigen::MatrixXd m(rows,cols); for(size_t r=0;r<rows;++r) for(size_t c=0;c<cols;++c) m(r,c)=v[r][c]; return m; }
mytrix::Vector vector_of(const double* p,size_t n) { if(!p&&n) throw std::invalid_argument("input vector must not be null"); Eigen::VectorXd v(n); for(size_t i=0;i<n;++i) v(static_cast<Eigen::Index>(i))=p[i]; return mytrix::Vector(v); }
mytrix::Matrix matrix_of(const double* p,size_t rows,size_t cols) { return mytrix::Matrix(eigen_matrix(p,rows,cols)); }
}
extern "C" {
void* coolbox_tracker_kalman_new(double dt,const double* a,size_t ar,size_t ac,const double* cmat,size_t cr,size_t cc,const double* q,size_t qr,size_t qc,const double* r,size_t rr,size_t rc,const double* p,size_t pr,size_t pc,char* e,size_t cap) { return coolbox_jbridge::invoke_handle(e,cap,[&]{ return new KalmanFilter(dt,matrix_of(a,ar,ac),matrix_of(cmat,cr,cc),matrix_of(q,qr,qc),matrix_of(r,rr,rc),matrix_of(p,pr,pc)); }); }
void coolbox_tracker_kalman_free(void* h) { delete static_cast<KalmanFilter*>(h); }
int coolbox_tracker_kalman_init(void* h,const double* x,size_t n,char* e,size_t c) { return coolbox_jbridge::invoke(e,c,[&]{ coolbox_jbridge::checked<KalmanFilter>(h,"KalmanFilter").init(vector_of(x,n)); }); }
int coolbox_tracker_kalman_predict(void* h,char* e,size_t c) { return coolbox_jbridge::invoke(e,c,[&]{ coolbox_jbridge::checked<KalmanFilter>(h,"KalmanFilter").predict(); }); }
int coolbox_tracker_kalman_update(void* h,const double* y,size_t n,char* e,size_t c) { return coolbox_jbridge::invoke(e,c,[&]{ coolbox_jbridge::checked<KalmanFilter>(h,"KalmanFilter").update(vector_of(y,n)); }); }
double* coolbox_tracker_kalman_state(void* h,size_t* n,char* e,size_t c) { try { const auto& v=coolbox_jbridge::checked<KalmanFilter>(h,"KalmanFilter").state(); std::vector<double> out(v.data.data(),v.data.data()+v.data.size()); return coolbox_jbridge::malloc_copy(out,n); } catch(const std::exception& x) { coolbox_jbridge::write_error(e,c,x.what()); return nullptr; } }
double* coolbox_tracker_kalman_covariance(void* h,size_t* rows,size_t* cols,char* e,size_t c) { try { const auto& m=coolbox_jbridge::checked<KalmanFilter>(h,"KalmanFilter").covariance().data; std::vector<double> out; for(Eigen::Index i=0;i<m.rows();++i) for(Eigen::Index j=0;j<m.cols();++j) out.push_back(m(i,j)); if(rows)*rows=m.rows(); if(cols)*cols=m.cols(); return coolbox_jbridge::malloc_copy(out,nullptr); } catch(const std::exception& x) { coolbox_jbridge::write_error(e,c,x.what()); return nullptr; } }
}
