"""
Demo: Using cool_car::MATH PDE/SPDE solvers with mytrix::DenseMatrix and xarray backends
"""
import math
import random
import numpy as np

# Assume pybind11 bindings or C++/Python bridge for mytrix and MATH are available
# Here we mock the interface for demonstration

# --- Mock mytrix DenseMatrix as numpy array ---
class DenseMatrix(np.ndarray):
    @staticmethod
    def from_shape(rows, cols, fill=0.0):
        return np.full((rows, cols), fill).view(DenseMatrix)

# --- Mock xarray as numpy ndarray wrapper ---
class xarray(np.ndarray):
    @staticmethod
    def from_shape(shape, fill=0.0):
        return np.full(shape, fill).view(xarray)

# --- Demo PDE solver usage ---
def demo_pde_mytrix():
    print("PDE (mytrix backend): 2D heat equation")
    Nx, Ny = 8, 8
    dx, dy = 1.0/(Nx-1), 1.0/(Ny-1)
    dt = 0.01
    steps = 10
    alpha = 0.1
    u0 = DenseMatrix.from_shape(Nx, Ny)
    for i in range(Nx):
        for j in range(Ny):
            u0[i, j] = math.sin(math.pi * i * dx) * math.sin(math.pi * j * dy)
    u = u0.copy()
    for t in range(steps):
        u_new = u.copy()
        for i in range(1, Nx-1):
            for j in range(1, Ny-1):
                u_new[i, j] = u[i, j] + alpha * dt * (
                    (u[i-1, j] - 2*u[i, j] + u[i+1, j]) / dx**2 +
                    (u[i, j-1] - 2*u[i, j] + u[i, j+1]) / dy**2
                )
        u = u_new
    print("Center value after evolution:", u[Nx//2, Ny//2])

def demo_pde_xarray():
    print("PDE (xarray backend): 3D heat equation")
    shape = (6, 6, 6)
    dx = [1.0/(n-1) for n in shape]
    dt = 0.01
    steps = 5
    alpha = 0.1
    u0 = xarray.from_shape(shape)
    for i in range(shape[0]):
        for j in range(shape[1]):
            for k in range(shape[2]):
                u0[i, j, k] = math.sin(math.pi * i * dx[0]) * math.sin(math.pi * j * dx[1]) * math.sin(math.pi * k * dx[2])
    u = u0.copy()
    for t in range(steps):
        u_new = u.copy()
        for i in range(1, shape[0]-1):
            for j in range(1, shape[1]-1):
                for k in range(1, shape[2]-1):
                    u_new[i, j, k] = u[i, j, k] + alpha * dt * (
                        (u[i-1, j, k] - 2*u[i, j, k] + u[i+1, j, k]) / dx[0]**2 +
                        (u[i, j-1, k] - 2*u[i, j, k] + u[i, j+1, k]) / dx[1]**2 +
                        (u[i, j, k-1] - 2*u[i, j, k] + u[i, j, k+1]) / dx[2]**2
                    )
        u = u_new
    print("Center value after evolution:", u[shape[0]//2, shape[1]//2, shape[2]//2])

def demo_spde_mytrix():
    print("SPDE (mytrix backend): 1D stochastic heat equation")
    N = 16
    dx = 1.0/(N-1)
    dt = 0.01
    steps = 10
    alpha = 0.1
    sigma = 0.05
    u0 = DenseMatrix.from_shape(N, 1)
    for i in range(N):
        u0[i, 0] = math.sin(math.pi * i * dx)
    u = u0.copy()
    rng = random.Random(42)
    for t in range(steps):
        u_new = u.copy()
        for i in range(1, N-1):
            dW = rng.gauss(0, 1) * math.sqrt(dt)
            u_new[i, 0] = u[i, 0] + alpha * dt / (dx*dx) * (u[i-1, 0] - 2*u[i, 0] + u[i+1, 0]) + sigma * dW
        u = u_new
    print("Middle value after evolution:", u[N//2, 0])

def demo_spde_xarray():
    print("SPDE (xarray backend): 2D stochastic heat equation")
    shape = (8, 8)
    dx = [1.0/(n-1) for n in shape]
    dt = 0.01
    steps = 5
    alpha = 0.1
    sigma = 0.05
    u0 = xarray.from_shape(shape)
    for i in range(shape[0]):
        for j in range(shape[1]):
            u0[i, j] = math.sin(math.pi * i * dx[0]) * math.sin(math.pi * j * dx[1])
    u = u0.copy()
    rng = random.Random(42)
    for t in range(steps):
        u_new = u.copy()
        for i in range(1, shape[0]-1):
            for j in range(1, shape[1]-1):
                dW = rng.gauss(0, 1) * math.sqrt(dt)
                u_new[i, j] = u[i, j] + alpha * dt * (
                    (u[i-1, j] - 2*u[i, j] + u[i+1, j]) / dx[0]**2 +
                    (u[i, j-1] - 2*u[i, j] + u[i, j+1]) / dx[1]**2
                ) + sigma * dW
        u = u_new
    print("Center value after evolution:", u[shape[0]//2, shape[1]//2])

if __name__ == "__main__":
    demo_pde_mytrix()
    demo_pde_xarray()
    demo_spde_mytrix()
    demo_spde_xarray()
