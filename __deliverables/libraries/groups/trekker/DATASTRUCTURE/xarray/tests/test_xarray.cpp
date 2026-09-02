// test_xarray.cpp
// Basic tests for xarray
#include "../headers/xarray.h"
#include <cassert>
#include <iostream>

int main() {
    // 1D array
    xarray<int, 1> arr1d(std::array<size_t, 1>{5});
    for (size_t i = 0; i < 5; ++i) {
        std::array<size_t, 1> idx = {i};
        arr1d[idx] = static_cast<int>(i * 2);
    }
    for (size_t i = 0; i < 5; ++i) {
        std::array<size_t, 1> idx = {i};
        assert(arr1d[idx] == static_cast<int>(i * 2));
    }

    // 2D array
    xarray<double, 2> arr2d(std::array<size_t, 2>{3, 4}, 1.5);
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            std::array<size_t, 2> idx = {i, j};
            assert(arr2d[idx] == 1.5);
        }
    }
    {
        std::array<size_t, 2> idx = {2, 3};
        arr2d[idx] = 7.7;
        assert(arr2d[idx] == 7.7);
    }

    // 3D array
    xarray<int, 3> arr3d(std::array<size_t, 3>{2, 2, 2});
    {
        std::array<size_t, 3> idx = {1, 1, 1};
        arr3d[idx] = 42;
        assert(arr3d[idx] == 42);
    }

    std::cout << "All xarray tests passed!\n";
    return 0;
}
