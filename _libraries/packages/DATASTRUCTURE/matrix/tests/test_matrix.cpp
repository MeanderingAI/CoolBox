#include "../headers/matrix_dense.h"
#include "../headers/matrix_sparse.h"
#include "tyst_framework.hpp"

using namespace matrix;

TYST_TEST(DenseMatrix, AddMultiplyTranspose) {
    DenseMatrix a(2, 2); a.at(0,0)=1; a.at(0,1)=2; a.at(1,0)=3; a.at(1,1)=4;
    DenseMatrix b(2, 2); b.at(0,0)=5; b.at(0,1)=6; b.at(1,0)=7; b.at(1,1)=8;
    auto sum = a.add(b);
    TYST_EXPECT_DOUBLE_EQ(static_cast<DenseMatrix*>(sum.get())->at(0,0), 6);
    auto prod = a.multiply(b);
    TYST_EXPECT_DOUBLE_EQ(static_cast<DenseMatrix*>(prod.get())->at(0,0), 19);
    auto trans = a.transpose();
    TYST_EXPECT_DOUBLE_EQ(static_cast<DenseMatrix*>(trans.get())->at(1,0), 2);
}

TYST_TEST(SparseMatrix, AddMultiplyTranspose) {
    SparseMatrix a(2,2); a.set(0,0,1); a.set(1,1,2);
    SparseMatrix b(2,2); b.set(0,0,3); b.set(1,1,4);
    auto sum = a.add(b);
    TYST_EXPECT_DOUBLE_EQ(static_cast<SparseMatrix*>(sum.get())->get(0,0), 4);
    auto prod = a.multiply(b);
    TYST_EXPECT_DOUBLE_EQ(static_cast<SparseMatrix*>(prod.get())->get(0,0), 3);
    auto trans = a.transpose();
    TYST_EXPECT_DOUBLE_EQ(static_cast<SparseMatrix*>(trans.get())->get(0,1), 0);
}
