// Marked Point Process bindings — wraps Eigen types for JS.
#include <emscripten/bind.h>
#include "marked_point_process.h"

using namespace emscripten;
using namespace ml;

class MPPWrapper {
    MarkedPointProcess mpp_;
public:
    MPPWrapper(int num_marks = 2, double learning_rate = 0.01, int max_iterations = 1000)
        : mpp_(num_marks, learning_rate, max_iterations) {}

    void fit(const std::vector<std::vector<double>>& event_times,
             const std::vector<std::vector<int>>& event_marks) {
        mpp_.fit(event_times, event_marks);
    }
};

EMSCRIPTEN_BINDINGS(marked_point_process_module) {
    register_vector<double>("VectorDouble_MPP");
    register_vector<int>("VectorInt_MPP");
    register_vector<std::vector<double>>("VectorVectorDouble_MPP");
    register_vector<std::vector<int>>("VectorVectorInt_MPP");

    class_<MPPWrapper>("MarkedPointProcess")
        .constructor<int, double, int>()
        .function("fit", &MPPWrapper::fit)
    ;
}
