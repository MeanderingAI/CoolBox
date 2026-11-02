#include "coolbox_jbridge.h"
#include "pde_solver.hpp"
#include "spde.hpp"
#include "matrix_dense.h"
#include <random>

namespace {
mytrix::DenseMatrix from_flat(const double* values, int rows, int cols) {
    if (!values || rows <= 0 || cols <= 0) throw std::invalid_argument("matrix must be non-empty");
    return mytrix::DenseMatrix(std::vector<double>(values, values + static_cast<size_t>(rows) * cols), rows, cols);
}
void copy_out(const mytrix::DenseMatrix& matrix, double* values) {
    if (!values) throw std::invalid_argument("matrix output is null");
    for (int r=0;r<matrix.rows();++r) for(int c=0;c<matrix.cols();++c) values[static_cast<size_t>(r)*matrix.cols()+c]=matrix(r,c);
}
}
extern "C" {
void* coolbox_pde_matrix_new(int rows, int cols) {
    try { if(rows<0||cols<0) return nullptr; return new mytrix::DenseMatrix(rows,cols); } catch (...) { return nullptr; }
}
void* coolbox_pde_matrix_from_data(const double* data, int rows, int cols, char* error, size_t capacity) {
    return coolbox_jbridge::invoke_handle(error,capacity,[&]{ return new mytrix::DenseMatrix(from_flat(data,rows,cols)); });
}
void coolbox_pde_matrix_free(void* handle) { delete static_cast<mytrix::DenseMatrix*>(handle); }
int coolbox_pde_matrix_rows(void* handle) { return handle ? static_cast<mytrix::DenseMatrix*>(handle)->rows() : 0; }
int coolbox_pde_matrix_cols(void* handle) { return handle ? static_cast<mytrix::DenseMatrix*>(handle)->cols() : 0; }
int coolbox_pde_matrix_get(void* handle, int row, int col, double* out, char* error, size_t capacity) {
    return coolbox_jbridge::invoke(error,capacity,[&]{ auto& m=coolbox_jbridge::checked<mytrix::DenseMatrix>(handle,"matrix"); if(!out||row<0||col<0||row>=m.rows()||col>=m.cols()) throw std::out_of_range("matrix index is out of range"); *out=m(row,col); });
}
int coolbox_pde_matrix_set(void* handle, int row, int col, double value, char* error, size_t capacity) {
    return coolbox_jbridge::invoke(error,capacity,[&]{ auto& m=coolbox_jbridge::checked<mytrix::DenseMatrix>(handle,"matrix"); if(row<0||col<0||row>=m.rows()||col>=m.cols()) throw std::out_of_range("matrix index is out of range"); m(row,col)=value; });
}
int coolbox_pde_matrix_copy(void* handle, double* out, size_t count, char* error, size_t capacity) {
    return coolbox_jbridge::invoke(error,capacity,[&]{ auto& m=coolbox_jbridge::checked<mytrix::DenseMatrix>(handle,"matrix"); if(!out||count<static_cast<size_t>(m.rows())*m.cols()) throw std::invalid_argument("matrix output is too small"); copy_out(m,out); });
}
int coolbox_pde_solve_heat_1d(void* input, double alpha, double dx, double dt, int steps, double* output, char* error, size_t capacity) {
    return coolbox_jbridge::invoke(error,capacity,[&]{ auto& m=coolbox_jbridge::checked<mytrix::DenseMatrix>(input,"matrix"); auto result=cool_car::MATH::PDESolver::solve_heat_eq(m,alpha,dx,dt,steps); copy_out(result,output); });
}
int coolbox_pde_solve_heat_2d(void* input, double alpha, double dx, double dy, double dt, int steps, double* output, char* error, size_t capacity) {
    return coolbox_jbridge::invoke(error,capacity,[&]{ auto& m=coolbox_jbridge::checked<mytrix::DenseMatrix>(input,"matrix"); auto result=cool_car::MATH::PDESolver::solve_heat_eq_2d(m,alpha,dx,dy,dt,steps); copy_out(result,output); });
}
int coolbox_pde_solve_stochastic_heat_1d(void* input, double alpha, double sigma, double dx, double dt, int steps, int seed, double* output, char* error, size_t capacity) {
    return coolbox_jbridge::invoke(error,capacity,[&]{ auto& m=coolbox_jbridge::checked<mytrix::DenseMatrix>(input,"matrix"); std::mt19937 rng(seed); auto result=cool_car::MATH::SPDESolver::solve_stochastic_heat_eq(m,alpha,sigma,dx,dt,steps,rng); copy_out(result,output); });
}
int coolbox_pde_solve_stochastic_heat_2d(void* input, double alpha, double sigma, double dx, double dy, double dt, int steps, int seed, double* output, char* error, size_t capacity) {
    return coolbox_jbridge::invoke(error,capacity,[&]{ auto& m=coolbox_jbridge::checked<mytrix::DenseMatrix>(input,"matrix"); std::mt19937 rng(seed); auto result=cool_car::MATH::SPDESolver::solve_stochastic_heat_eq_2d(m,alpha,sigma,dx,dy,dt,steps,rng); copy_out(result,output); });
}
}
