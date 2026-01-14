
/**
 * @mainpage SVM Library
 *
 * @section usage_examples Usage Examples
 *
 * @subsection cpp_example C++ Example
 * @code{.cpp}
 * #include "ML/support_vector_machine/support_vector_machine.h"
 * LinearKernel kernel;
 * SVM svm(kernel);
 * svm.fit(X, y); // X: Eigen::MatrixXd, y: Eigen::VectorXd
 * double pred = svm.predict(sample);
 * @endcode
 *
 * @subsection python_example Python Example
 * @code{.python}
 * from ml_core.svm import SVM, LinearKernel
 * kernel = LinearKernel()
 * svm = SVM(kernel)
 * svm.fit(X, y)
 * pred = svm.predict(sample)
 * @endcode
 *
 * @subsection js_example JavaScript Example (WASM/Emscripten)
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
 * @subsection js_example_sync JavaScript Example (Synchronous, MODULARIZE=0)
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
#include <Eigen/Dense>
#include "ML/support_vector_machine/kernel.h"

class SVM {
public:
    SVM(const Kernel& kernel);
    ~SVM() = default;

    void fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y);
    double predict(const Eigen::VectorXd& sample) const;

private:
    Eigen::MatrixXd support_vectors_;
    Eigen::VectorXd support_vector_labels_;
    Eigen::VectorXd alphas_;
    double bias_;
    const Kernel& kernel_;
};

#endif // SVM_H