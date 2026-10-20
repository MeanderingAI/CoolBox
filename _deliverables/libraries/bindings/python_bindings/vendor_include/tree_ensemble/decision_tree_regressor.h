#ifndef DECISION_TREE_REGRESSOR_H
#define DECISION_TREE_REGRESSOR_H

#include <cstddef>
#include <memory>
#include <vector>

/**
 * @brief Numeric regression tree using squared-error splits and mean-valued leaves.
 *
 * Splits send feature values <= the threshold to the left child. Predictions
 * outside the training range use the corresponding boundary leaf.
 */
class DecisionTreeRegressor {
public:
    DecisionTreeRegressor();
    ~DecisionTreeRegressor();
    DecisionTreeRegressor(const DecisionTreeRegressor&) = delete;
    DecisionTreeRegressor& operator=(const DecisionTreeRegressor&) = delete;

    /**
     * @brief Fit finite numeric features and targets with a nonnegative depth limit.
     *
     * A depth of zero produces a single leaf containing the target mean.
     * @throws std::invalid_argument For empty, mismatched, ragged, or non-finite
     * data, zero features, or negative depth. A failed fit preserves the old model.
     * @throws std::overflow_error If target statistics exceed the numeric range.
     */
    void fit(const std::vector<std::vector<double>>& X,
             const std::vector<double>& y, int max_depth);

    /**
     * @brief Predict a numeric target for a single sample.
     * @throws std::logic_error If fit has not completed successfully.
     * @throws std::invalid_argument For a wrong feature count or non-finite values.
     */
    double predict(const std::vector<double>& sample) const;

private:
    struct RegressionNode;
    std::unique_ptr<RegressionNode> root_;
    std::size_t num_features_ = 0;

    static std::unique_ptr<RegressionNode> build_tree(
        const std::vector<std::vector<double>>& X, const std::vector<double>& y,
        const std::vector<std::size_t>& indices, int remaining_depth);
};

#endif // DECISION_TREE_REGRESSOR_H
