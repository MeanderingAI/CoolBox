#include <emscripten/bind.h>
#include "gabor_patches.h"

using namespace emscripten;
using namespace ml;

// Helper to convert Eigen matrix to a flat JS array
val matrix_to_js(const Eigen::MatrixXd& mat) {
    val arr = val::array();
    for (int r = 0; r < mat.rows(); ++r) {
        val row = val::array();
        for (int c = 0; c < mat.cols(); ++c) {
            row.call<void>("push", mat(r, c));
        }
        arr.call<void>("push", row);
    }
    return arr;
}

EMSCRIPTEN_BINDINGS(gabor_patches_module) {
    // GaborParams
    value_object<GaborParams<double>>("GaborParams")
        .field("lambda", &GaborParams<double>::lambda)
        .field("theta", &GaborParams<double>::theta)
        .field("psi", &GaborParams<double>::psi)
        .field("sigma", &GaborParams<double>::sigma)
        .field("gamma", &GaborParams<double>::gamma)
    ;

    // Free functions (return as JS 2D arrays)
    function("gabor_kernel", optional_override([](const GaborParams<double>& p, int size, bool normalize) {
        return matrix_to_js(gabor_kernel(p, size, normalize));
    }));

    function("gabor_kernel_imaginary", optional_override([](const GaborParams<double>& p, int size, bool normalize) {
        return matrix_to_js(gabor_kernel_imaginary(p, size, normalize));
    }));

    function("gabor_energy", optional_override([](const GaborParams<double>& p, int size) {
        return matrix_to_js(gabor_energy(p, size));
    }));

    // GaborFilterBank
    class_<GaborFilterBank<double>>("GaborFilterBank")
        .constructor<>()
        .function("add_orientations", &GaborFilterBank<double>::add_orientations)
        .function("add_orientation", &GaborFilterBank<double>::add_orientation)
        .function("add_wavelength", &GaborFilterBank<double>::add_wavelength)
        .function("set_sigma", &GaborFilterBank<double>::set_sigma)
        .function("set_gamma", &GaborFilterBank<double>::set_gamma)
        .function("set_psi", &GaborFilterBank<double>::set_psi)
        .function("set_kernel_size", &GaborFilterBank<double>::set_kernel_size)
        .function("build", &GaborFilterBank<double>::build)
        .function("num_kernels", &GaborFilterBank<double>::num_kernels)
        .function("num_orientations", &GaborFilterBank<double>::num_orientations)
        .function("num_wavelengths", &GaborFilterBank<double>::num_wavelengths)
        .function("is_built", &GaborFilterBank<double>::is_built)
        .function("to_string", &GaborFilterBank<double>::to_string)
    ;
}
