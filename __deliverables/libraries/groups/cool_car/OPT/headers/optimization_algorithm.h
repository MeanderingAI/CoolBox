#ifndef OPTIMIZATION_ALGORITHM_H
#define OPTIMIZATION_ALGORITHM_H

#include <functional>
#include <vector>

namespace opt {

using ObjectiveFunction = std::function<double(const std::vector<double>&)>;

class OptimizationAlgorithm {
public:
    virtual ~OptimizationAlgorithm() = default;

    virtual std::vector<double> optimize(
        const ObjectiveFunction& objective,
        const std::vector<double>& initial_state) = 0;

    virtual const std::vector<double>& best_solution() const = 0;
    virtual double best_score() const = 0;
};

} // namespace opt

#endif // OPTIMIZATION_ALGORITHM_H
