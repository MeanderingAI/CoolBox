#ifndef CATENARY_SUPPORT_VECTOR_MACHINE_H
#define CATENARY_SUPPORT_VECTOR_MACHINE_H

#include <cstddef>
#include <vector>

struct CatenarySVMParameters {
    unsigned int epochs = 200;
    double learning_rate = 0.05;
    double regularization = 0.01;
    double reach_margin = 0.0;
};

struct CatenarySVMStageModel {
    std::vector<double> weights;
    double bias = 0.0;

    double score(const std::vector<double>& sample) const;
};

class CatenarySupportVectorMachine {
public:
    using StageFeatures = std::vector<std::vector<double>>;
    using TrainingFeatures = std::vector<StageFeatures>;
    using CostMatrix = std::vector<std::vector<double>>;

    explicit CatenarySupportVectorMachine(const CatenarySVMParameters& parameters = CatenarySVMParameters());

    void fit(const TrainingFeatures& stage_features, const CostMatrix& costs);

    std::size_t predict_stop_stage(const StageFeatures& sample_stages) const;
    std::vector<double> decision_scores(const StageFeatures& sample_stages) const;
    double empirical_cost(const TrainingFeatures& stage_features, const CostMatrix& costs) const;

    std::size_t stage_count() const;
    const CatenarySVMStageModel& stage_model(std::size_t stage_index) const;

private:
    CatenarySVMParameters parameters_;
    std::vector<CatenarySVMStageModel> stages_;
};

#endif // CATENARY_SUPPORT_VECTOR_MACHINE_H