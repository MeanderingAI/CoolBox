#include "coolbox_jbridge.h"
#include "support_vector_machine/support_vector_machine.h"
#include "support_vector_machine/linear_kernel.h"
#include "support_vector_machine/rbf_kernel.h"
#include "support_vector_machine/polynomial_kernel.h"
#include "support_vector_machine/sigmoid_kernel.h"
#include <Eigen/Dense>
#include <memory>

namespace {
struct KernelBox { std::shared_ptr<Kernel> value; };
struct SvmBox { std::shared_ptr<Kernel> keep_alive; SVM model; size_t features=0; bool fitted=false; explicit SvmBox(std::shared_ptr<Kernel> k) : keep_alive(std::move(k)), model(*keep_alive) {} };
Eigen::VectorXd vector_of(const double* p, size_t n) { if (!p && n) throw std::invalid_argument("input vector must not be null"); Eigen::VectorXd v(n); for (size_t i=0;i<n;++i) v(static_cast<Eigen::Index>(i))=p[i]; return v; }
Eigen::MatrixXd matrix_of(const double* p, size_t rows, size_t cols) { auto v=coolbox_jbridge::rows_of(p,rows,cols); Eigen::MatrixXd m(rows,cols); for(size_t r=0;r<rows;++r) for(size_t c=0;c<cols;++c) m(r,c)=v[r][c]; return m; }
}
extern "C" {
void* coolbox_svm_linear_kernel_new(char* e,size_t c) { return coolbox_jbridge::invoke_handle(e,c,[]{ return new KernelBox{std::make_shared<LinearKernel>()}; }); }
void* coolbox_svm_rbf_kernel_new(double gamma,char* e,size_t c) { return coolbox_jbridge::invoke_handle(e,c,[&]{ return new KernelBox{std::make_shared<RBFKernel>(gamma)}; }); }
void* coolbox_svm_polynomial_kernel_new(int degree,double gamma,double coef0,char* e,size_t c) { return coolbox_jbridge::invoke_handle(e,c,[&]{ return new KernelBox{std::make_shared<PolynomialKernel>(gamma,coef0,degree)}; }); }
void* coolbox_svm_sigmoid_kernel_new(double gamma,double coef0,char* e,size_t c) { return coolbox_jbridge::invoke_handle(e,c,[&]{ return new KernelBox{std::make_shared<SigmoidKernel>(gamma,coef0)}; }); }
void coolbox_svm_kernel_free(void* h) { delete static_cast<KernelBox*>(h); }
int coolbox_svm_kernel_calculate(void* h,const double* x,size_t nx,const double* y,size_t ny,double* out,char* e,size_t c) { return coolbox_jbridge::invoke(e,c,[&]{ if(!out) throw std::invalid_argument("output must not be null"); auto& k=coolbox_jbridge::checked<KernelBox>(h,"kernel"); if(nx!=ny) throw std::invalid_argument("kernel vectors must have equal lengths"); *out=k.value->calculate(vector_of(x,nx),vector_of(y,ny)); }); }
void* coolbox_svm_new(void* kernel,char* e,size_t c) { return coolbox_jbridge::invoke_handle(e,c,[&]{ auto& k=coolbox_jbridge::checked<KernelBox>(kernel,"kernel"); return new SvmBox(k.value); }); }
void coolbox_svm_free(void* h) { delete static_cast<SvmBox*>(h); }
int coolbox_svm_fit(void* h,const double* x,size_t rows,size_t cols,const double* y,size_t ny,char* e,size_t c) { return coolbox_jbridge::invoke(e,c,[&]{ auto& s=coolbox_jbridge::checked<SvmBox>(h,"SVM"); if(ny!=rows) throw std::invalid_argument("X and y must have the same number of rows"); s.model.fit(matrix_of(x,rows,cols),vector_of(y,ny)); s.features=cols; s.fitted=true; }); }
int coolbox_svm_predict(void* h,const double* x,size_t n,double* out,char* e,size_t c) { return coolbox_jbridge::invoke(e,c,[&]{ if(!out) throw std::invalid_argument("output must not be null"); auto& s=coolbox_jbridge::checked<SvmBox>(h,"SVM"); if(!s.fitted) throw std::logic_error("SVM has not been fitted"); if(n!=s.features) throw std::invalid_argument("sample feature count does not match fitted model"); *out=s.model.predict(vector_of(x,n)); }); }
}

