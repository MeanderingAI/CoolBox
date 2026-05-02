
/**
 * @page svm_main SVM Library
 *
 * @section usage_examples_svm Usage Examples
 *
 * @subsection cpp_example_svm C++ Example
 * @code{.cpp}
 * #include "support_vector_machine.h"
 * LinearKernel kernel;
 * SVM svm(kernel);
 * svm.fit(X, y); // X: Eigen::MatrixXd, y: Eigen::VectorXd
 * double pred = svm.predict(sample);
 * @endcode
 *
 * @subsection python_example_svm Python Example
 * @code{.python}
 * from ml_core.svm import SVM, LinearKernel
 * kernel = LinearKernel()
 * svm = SVM(kernel)
 * svm.fit(X, y)
 * pred = svm.predict(sample)
 * @endcode
 *
 * @subsection js_example_svm JavaScript Example (WASM/Emscripten)
 * @code{.js}
 * // Async usage (MODULARIZE=1, default):
 * createSVMModule().then(Module => {
 *     const SVM = Module.SVM;
 *     const kernel = new Module.LinearKernel();
 *     const svm = new SVM(kernel);
 *     svm.fit(X, y);
 *     const pred = svm.predict(sample);
 * });
 * @endcode
 *
 * @subsection js_example_sync_svm JavaScript Example (Synchronous, MODULARIZE=0)
 * @code{.js}
 * // If svm.js is loaded and exposes 'Module' globally:
 * const SVM = Module.SVM;
 * const kernel = new Module.LinearKernel();
 * const svm = new SVM(kernel);
 * svm.fit(X, y);
 * const pred = svm.predict(sample);
 * // Note: If built with MODULARIZE=1 (default), you must use createSVMModule().then(...)
 * // If built with MODULARIZE=0, you can use the Module object directly after script load.
 * @endcode
 */
#ifndef SVM_H
#define SVM_H

#include <vector>
#include <iostream>
#include "matrix_dense.h"
#include "kernel.h"

class SVM {
public:
    SVM(const Kernel& kernel);
    ~SVM() = default;

    enum class SolverType { GradientDescent, SMO };
    void fit(const matrix::DenseMatrix& X, const matrix::DenseMatrix& y, SolverType solver = SolverType::SMO);
    double predict(const matrix::DenseMatrix& sample) const;

private:
    matrix::DenseMatrix support_vectors_;
    std::vector<double> support_vector_labels_;
    std::vector<double> alphas_;
    double bias_;
    const Kernel& kernel_;
};

#endif // SVM_H