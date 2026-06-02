import ml_core

# Test PDE 1D
mat = ml_core.pde.DenseMatrix(11, 1)
for i in range(11):
    mat[i, 0] = 1.0
out = ml_core.pde.solve_heat_eq(mat, 0.1, 0.1, 0.01, 10)
print("PDE 1D result center:", out[5, 0])

# Test PDE 2D
mat2 = ml_core.pde.DenseMatrix(8, 8)
for i in range(8):
    for j in range(8):
        mat2[i, j] = 1.0
out2 = ml_core.pde.solve_heat_eq_2d(mat2, 0.1, 0.1, 0.1, 0.01, 5)
print("PDE 2D result center:", out2[4, 4])

# Test SPDE 1D
mat3 = ml_core.pde.DenseMatrix(11, 1)
for i in range(11):
    mat3[i, 0] = 1.0
out3 = ml_core.spde.solve_stochastic_heat_eq(mat3, 0.1, 0.05, 0.1, 0.01, 10, 42)
print("SPDE 1D result center:", out3[5, 0])

# Test SPDE 2D
mat4 = ml_core.pde.DenseMatrix(8, 8)
for i in range(8):
    for j in range(8):
        mat4[i, j] = 1.0
out4 = ml_core.spde.solve_stochastic_heat_eq_2d(mat4, 0.1, 0.05, 0.1, 0.1, 0.01, 5, 42)
print("SPDE 2D result center:", out4[4, 4])
