#include "tyst_framework.hpp"

#include "scaling.h"

#include <cmath>
#include <filesystem>
#include <map>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

using namespace ml::deep_learning;
using namespace ml::deep_learning::scaling;

namespace {

Tensor make_tensor(const std::vector<size_t>& shape, double seed) {
    Tensor tensor(shape, 0.0);
    for (size_t i = 0; i < tensor.size(); ++i) {
        tensor.data()[i] = std::sin(seed + static_cast<double>(i) * 0.37);
    }
    return tensor;
}

} // namespace

// ============================================================================
// Citations (all eight papers)
// ============================================================================

TYST_TEST(ScalingCitationsTest, RegistersEveryImplementedPaper) {
    const auto& citations = scaling_citations();
    TYST_ASSERT_EQ(citations.size(), 8u);

    const std::vector<std::string> expected_ids = {
        "1910.02054", "2104.06069", "2104.07857", "2201.05596",
        "2206.01861", "2406.13768", "2406.18820", "2409.15241"
    };
    for (size_t i = 0; i < expected_ids.size(); ++i) {
        TYST_EXPECT_EQ(citations[i].arxiv_id, expected_ids[i]);
        TYST_EXPECT_FALSE(citations[i].component.empty());
    }
}

TYST_TEST(ScalingCitationsTest, LooksUpByKeyAndComponent) {
    const PaperCitation* zero = find_citation("rajbhandari_zero_2020");
    TYST_ASSERT_TRUE(zero != nullptr);
    TYST_EXPECT_EQ(zero->component, std::string("zero.h"));
    TYST_EXPECT_TRUE(find_citation("does_not_exist") == nullptr);

    TYST_EXPECT_EQ(citations_for_component("domino.h").size(), 1u);
    TYST_EXPECT_EQ(citations_for_component("nothing.h").size(), 0u);
}

TYST_TEST(ScalingCitationsTest, RendersBibtex) {
    const std::string bibtex = to_bibtex(scaling_citations());
    TYST_EXPECT_NE(bibtex.find("@article{rajbhandari_zero_2020"), std::string::npos);
    TYST_EXPECT_NE(bibtex.find("eprint = {2409.15241}"), std::string::npos);
    TYST_EXPECT_NE(bibtex.find("archivePrefix = {arXiv}"), std::string::npos);
}

// ============================================================================
// ZeRO (arXiv:1910.02054)
// ============================================================================

TYST_TEST(ZeroTest, MemoryModelMatchesThePaper) {
    const size_t psi = 1000;

    const auto baseline = estimate_zero_memory(psi, ZeroStage::kDisabled, 64);
    TYST_EXPECT_NEAR(baseline.per_device_bytes, 16.0 * psi, 1e-9);

    const auto stage1 = estimate_zero_memory(psi, ZeroStage::kOptimizerStates, 64);
    TYST_EXPECT_NEAR(stage1.per_device_bytes, 4.0 * psi + 12.0 * psi / 64.0, 1e-9);

    const auto stage2 = estimate_zero_memory(psi, ZeroStage::kGradients, 64);
    TYST_EXPECT_NEAR(stage2.per_device_bytes, 2.0 * psi + 14.0 * psi / 64.0, 1e-9);

    const auto stage3 = estimate_zero_memory(psi, ZeroStage::kParameters, 64);
    TYST_EXPECT_NEAR(stage3.per_device_bytes, 16.0 * psi / 64.0, 1e-9);
}

TYST_TEST(ZeroTest, OnlyStageThreeIncreasesCommunicationVolume) {
    TYST_EXPECT_DOUBLE_EQ(estimate_zero_memory(10, ZeroStage::kOptimizerStates, 4)
                              .communication_volume_factor, 1.0);
    TYST_EXPECT_DOUBLE_EQ(estimate_zero_memory(10, ZeroStage::kGradients, 4)
                              .communication_volume_factor, 1.0);
    TYST_EXPECT_DOUBLE_EQ(estimate_zero_memory(10, ZeroStage::kParameters, 4)
                              .communication_volume_factor, 1.5);
}

TYST_TEST(ZeroTest, LargerModelsFitAsTheWorldGrows) {
    const double device_bytes = 32.0 * 1024 * 1024 * 1024;
    const size_t small = max_model_size(device_bytes, ZeroStage::kParameters, 4);
    const size_t large = max_model_size(device_bytes, ZeroStage::kParameters, 1024);
    TYST_EXPECT_GT(large, small);
    TYST_EXPECT_EQ(to_string(ZeroStage::kParameters), std::string("P_os+g+p"));
}

TYST_TEST(ZeroTest, PartitionsParametersAcrossRanks) {
    ZeroDataParallelEngine::Config config;
    config.stage = ZeroStage::kGradients;
    ZeroDataParallelEngine engine(10, 4, config);

    TYST_EXPECT_EQ(engine.shard_for(0).count, 3u);
    TYST_EXPECT_EQ(engine.shard_for(1).count, 3u);
    TYST_EXPECT_EQ(engine.shard_for(2).count, 2u);
    TYST_EXPECT_EQ(engine.shard_for(3).count, 2u);
    TYST_EXPECT_EQ(engine.owner_of(0), 0u);
    TYST_EXPECT_EQ(engine.owner_of(9), 3u);
}

TYST_TEST(ZeroTest, ShardedStepMatchesReplicatedAdam) {
    const size_t count = 8;
    const size_t world_size = 4;

    ZeroDataParallelEngine::Config config;
    config.stage = ZeroStage::kGradients;
    config.learning_rate = 0.1;

    ZeroDataParallelEngine engine(count, world_size, config);
    Tensor parameters({count}, 1.0);

    std::vector<double> reference(count, 1.0);
    std::vector<double> m(count, 0.0);
    std::vector<double> v(count, 0.0);

    for (size_t step = 1; step <= 5; ++step) {
        std::vector<std::vector<double>> gradients(world_size, std::vector<double>(count, 0.0));
        for (size_t rank = 0; rank < world_size; ++rank) {
            for (size_t i = 0; i < count; ++i) {
                gradients[rank][i] = 0.01 * static_cast<double>(rank + 1) *
                                     static_cast<double>(i + 1);
            }
        }
        engine.step(parameters, gradients);

        const double bias1 = 1.0 - std::pow(0.9, static_cast<double>(step));
        const double bias2 = 1.0 - std::pow(0.999, static_cast<double>(step));
        for (size_t i = 0; i < count; ++i) {
            double summed = 0.0;
            for (size_t rank = 0; rank < world_size; ++rank) {
                summed += gradients[rank][i];
            }
            const double g = summed / static_cast<double>(world_size);
            m[i] = 0.9 * m[i] + 0.1 * g;
            v[i] = 0.999 * v[i] + 0.001 * g * g;
            reference[i] -= 0.1 * (m[i] / bias1) / (std::sqrt(v[i] / bias2) + 1e-8);
        }
    }

    for (size_t i = 0; i < count; ++i) {
        TYST_EXPECT_NEAR(parameters.data()[i], reference[i], 1e-12);
    }
    TYST_EXPECT_GT(engine.collectives().bytes_moved(), 0.0);
}

// ============================================================================
// ZeRO-Infinity (arXiv:2104.07857)
// ============================================================================

TYST_TEST(ZeroInfinityTest, DemotesOptimizerStateToSlowerTiers) {
    InfinityOffloadEngine engine({
        DeviceTier{MemoryTier::kGpu, 1000.0, 1.0e12},
        DeviceTier{MemoryTier::kCpu, 100000.0, 1.0e10},
        DeviceTier{MemoryTier::kNvme, 1.0e9, 3.0e9}
    });

    ZeroMemoryEstimate estimate;
    estimate.parameter_bytes = 500.0;
    estimate.gradient_bytes = 500.0;
    estimate.optimizer_bytes = 6000.0;
    estimate.per_device_bytes = 7000.0;

    const OffloadPlan plan = engine.plan(estimate);
    TYST_ASSERT_EQ(plan.placements.size(), 3u);
    TYST_EXPECT_TRUE(plan.fits);
    TYST_EXPECT_EQ(plan.placements[0].tier, MemoryTier::kGpu);
    TYST_EXPECT_EQ(plan.placements[1].tier, MemoryTier::kGpu);
    TYST_EXPECT_EQ(plan.placements[2].tier, MemoryTier::kCpu);
    TYST_EXPECT_NEAR(plan.resident_gpu_bytes, 1000.0, 1e-9);
}

TYST_TEST(ZeroInfinityTest, BandwidthCentricPartitioningScalesWithDataParallelism) {
    InfinityOffloadEngine engine({
        DeviceTier{MemoryTier::kNvme, 1.0e12, 3.0e9}
    });

    TYST_EXPECT_NEAR(engine.aggregate_bandwidth(MemoryTier::kNvme, 8), 2.4e10, 1.0);

    const auto slices = engine.bandwidth_centric_partition(1000, 3);
    TYST_ASSERT_EQ(slices.size(), 3u);
    TYST_EXPECT_EQ(std::accumulate(slices.begin(), slices.end(), static_cast<size_t>(0)), 1000u);
}

TYST_TEST(ZeroInfinityTest, MemoryCentricTilingFitsTheBudget) {
    const auto tiles = InfinityOffloadEngine::memory_centric_tiles(10, 100, 350.0);
    TYST_ASSERT_EQ(tiles.size(), 4u);
    TYST_EXPECT_EQ(tiles[0].count, 3u);
    TYST_EXPECT_EQ(tiles[3].count, 1u);
    TYST_EXPECT_EQ(tiles[3].end(), 10u);
    TYST_EXPECT_THROW(InfinityOffloadEngine::memory_centric_tiles(10, 100, 50.0),
                      std::invalid_argument);
}

TYST_TEST(ZeroInfinityTest, EfficiencyAndBandwidthModelsAreInverses) {
    const double peak_flops = 7.0e13;
    const double arithmetic_intensity = 200.0;
    const double bandwidth =
        InfinityOffloadEngine::required_bandwidth(arithmetic_intensity, peak_flops, 0.9);
    const double achieved =
        InfinityOffloadEngine::efficiency(arithmetic_intensity, bandwidth, peak_flops);
    TYST_EXPECT_NEAR(achieved, 0.9, 1e-9);

    TYST_EXPECT_DOUBLE_EQ(InfinityOffloadEngine::overlapped_seconds(2.0, 3.0, true), 3.0);
    TYST_EXPECT_DOUBLE_EQ(InfinityOffloadEngine::overlapped_seconds(2.0, 3.0, false), 5.0);
}

// ============================================================================
// 1-bit LAMB (arXiv:2104.06069)
// ============================================================================

TYST_TEST(OneBitLambTest, CompressionIsSignPlusChunkMagnitude) {
    const std::vector<double> values = {1.0, -3.0, 2.0, -2.0};
    std::vector<double> error;
    const OneBitPayload payload = compress_one_bit(values, error, 4);

    TYST_ASSERT_EQ(payload.scales.size(), 1u);
    TYST_EXPECT_NEAR(payload.scales[0], 2.0, 1e-12);

    const std::vector<double> decompressed = decompress_one_bit(payload);
    TYST_ASSERT_EQ(decompressed.size(), 4u);
    TYST_EXPECT_NEAR(decompressed[0], 2.0, 1e-12);
    TYST_EXPECT_NEAR(decompressed[1], -2.0, 1e-12);
    TYST_EXPECT_NEAR(error[0], -1.0, 1e-12);
    TYST_EXPECT_NEAR(error[1], -1.0, 1e-12);
}

TYST_TEST(OneBitLambTest, ErrorCompensationKeepsCompressionUnbiased) {
    std::vector<double> values(256, 0.25);
    std::vector<double> error;

    double accumulated = 0.0;
    for (size_t step = 0; step < 50; ++step) {
        const OneBitPayload payload = compress_one_bit(values, error, 256);
        accumulated += decompress_one_bit(payload)[0];
    }
    // The residual keeps the running mean pinned to the true value.
    TYST_EXPECT_NEAR(accumulated / 50.0, 0.25, 1e-9);
}

TYST_TEST(OneBitLambTest, CompressionRatioIsFarBelowFp32) {
    std::vector<double> values(4096, 0.5);
    std::vector<double> error;
    const OneBitPayload payload = compress_one_bit(values, error, 512);
    TYST_EXPECT_LT(compression_ratio(payload), 0.1);
}

TYST_TEST(OneBitLambTest, CompressedAllReduceMatchesTheUncompressedSum) {
    CollectiveGroup group(4);
    std::vector<std::vector<double>> per_rank(4, std::vector<double>(64, 0.0));
    for (size_t rank = 0; rank < 4; ++rank) {
        for (size_t i = 0; i < 64; ++i) {
            per_rank[rank][i] = 1.0;
        }
    }
    std::vector<std::vector<double>> errors;
    const std::vector<double> reduced = compressed_all_reduce(group, per_rank, errors, 64);

    TYST_ASSERT_EQ(reduced.size(), 64u);
    TYST_EXPECT_NEAR(reduced[0], 4.0, 1e-12);
    TYST_EXPECT_EQ(errors.size(), 4u);
}

TYST_TEST(OneBitLambTest, SwitchesToCompressionAfterWarmupAndConverges) {
    OneBitLamb::Config config;
    config.learning_rate = 0.05;
    config.warmup_steps = 10;
    config.chunk_size = 16;

    OneBitLamb optimizer(config);
    Tensor parameters({16}, 1.0);

    double initial_loss = 0.0;
    for (size_t i = 0; i < parameters.size(); ++i) {
        initial_loss += parameters.data()[i] * parameters.data()[i];
    }

    for (size_t step = 0; step < 60; ++step) {
        Tensor gradients(parameters.shape(), 0.0);
        for (size_t i = 0; i < parameters.size(); ++i) {
            gradients.data()[i] = 2.0 * parameters.data()[i];
        }
        optimizer.step(parameters, gradients);

        if (step == 4) {
            TYST_EXPECT_FALSE(optimizer.in_compression_phase());
            TYST_EXPECT_DOUBLE_EQ(optimizer.last_compression_ratio(), 1.0);
        }
    }

    TYST_EXPECT_TRUE(optimizer.in_compression_phase());
    TYST_EXPECT_LT(optimizer.last_compression_ratio(), 0.5);
    TYST_EXPECT_FALSE(optimizer.frozen_variance().empty());

    double final_loss = 0.0;
    for (size_t i = 0; i < parameters.size(); ++i) {
        final_loss += parameters.data()[i] * parameters.data()[i];
    }
    TYST_EXPECT_LT(final_loss, initial_loss);
    TYST_EXPECT_EQ(optimizer.name(), std::string("OneBitLAMB"));
}

TYST_TEST(OneBitLambTest, ResetClearsState) {
    OneBitLamb optimizer;
    Tensor parameters({4}, 1.0);
    Tensor gradients({4}, 0.5);
    optimizer.step(parameters, gradients);
    TYST_EXPECT_EQ(optimizer.step_count(), 1u);
    optimizer.reset();
    TYST_EXPECT_EQ(optimizer.step_count(), 0u);
}

// ============================================================================
// DeepSpeed-MoE (arXiv:2201.05596)
// ============================================================================

TYST_TEST(MoETest, ForwardProducesTokenShapedOutput) {
    MoEConfig config;
    config.d_model = 4;
    config.d_ff = 6;
    config.num_experts = 4;
    config.top_k = 1;
    config.capacity_factor = 4.0;

    MoELayer layer(config);
    Tensor input = make_tensor({8, 4}, 0.1);
    Tensor output = layer.forward(input);

    TYST_ASSERT_EQ(output.shape().size(), 2u);
    TYST_EXPECT_EQ(output.shape()[0], 8u);
    TYST_EXPECT_EQ(output.shape()[1], 4u);
    TYST_EXPECT_EQ(layer.routing().size(), 8u);
    TYST_EXPECT_EQ(layer.dropped_tokens(), 0u);
    TYST_EXPECT_TRUE(layer.has_parameters());
    TYST_EXPECT_EQ(layer.name(), std::string("MoE"));
}

TYST_TEST(MoETest, CapacityFactorDropsOverflowTokens) {
    MoEConfig config;
    config.d_model = 4;
    config.d_ff = 4;
    config.num_experts = 2;
    config.top_k = 1;
    config.capacity_factor = 0.25;

    MoELayer layer(config);
    TYST_EXPECT_EQ(layer.expert_capacity(16), 2u);

    layer.forward(make_tensor({16, 4}, 0.9));
    TYST_EXPECT_GT(layer.dropped_tokens(), 0u);
    TYST_EXPECT_LE(layer.routing().size(), 4u);
}

TYST_TEST(MoETest, LoadBalancingLossStaysInItsValidRange) {
    MoEConfig config;
    config.d_model = 4;
    config.d_ff = 4;
    config.num_experts = 4;
    config.top_k = 1;
    config.capacity_factor = 8.0;

    MoELayer layer(config);
    layer.forward(make_tensor({32, 4}, 0.3));
    // E * sum_e f_e * P_e is bounded above by the expert count and is only zero
    // when the gate routes tokens to experts it assigns no probability mass to.
    TYST_EXPECT_GT(layer.load_balancing_loss(), 0.0);
    TYST_EXPECT_LE(layer.load_balancing_loss(), 4.0);
}

TYST_TEST(MoETest, BackwardMatchesNumericalInputGradient) {
    MoEConfig config;
    config.d_model = 3;
    config.d_ff = 4;
    config.num_experts = 2;
    config.top_k = 1;
    config.capacity_factor = 8.0;
    config.residual_mlp = true;

    MoELayer layer(config);
    Tensor input = make_tensor({4, 3}, 0.7);

    Tensor output = layer.forward(input);
    Tensor seed(output.shape(), 1.0);
    Tensor analytic = layer.backward(seed);

    const double epsilon = 1e-6;
    for (size_t i = 0; i < input.size(); ++i) {
        Tensor plus = input.clone();
        Tensor minus = input.clone();
        plus.data()[i] += epsilon;
        minus.data()[i] -= epsilon;
        const double numerical =
            (layer.forward(plus).sum() - layer.forward(minus).sum()) / (2.0 * epsilon);
        TYST_EXPECT_NEAR(analytic.data()[i], numerical, 1e-4);
    }
}

TYST_TEST(MoETest, TrainingStepReducesLoss) {
    MoEConfig config;
    config.d_model = 4;
    config.d_ff = 8;
    config.num_experts = 2;
    config.top_k = 1;
    config.capacity_factor = 8.0;

    MoELayer layer(config);
    Tensor input = make_tensor({8, 4}, 0.2);
    Tensor target = make_tensor({8, 4}, 1.4);

    auto mse = [&](const Tensor& prediction) {
        double sum = 0.0;
        for (size_t i = 0; i < prediction.size(); ++i) {
            const double difference = prediction.data()[i] - target.data()[i];
            sum += difference * difference;
        }
        return sum / static_cast<double>(prediction.size());
    };

    const double before = mse(layer.forward(input));
    for (size_t step = 0; step < 40; ++step) {
        Tensor prediction = layer.forward(input);
        Tensor gradient(prediction.shape(), 0.0);
        for (size_t i = 0; i < prediction.size(); ++i) {
            gradient.data()[i] = 2.0 * (prediction.data()[i] - target.data()[i]) /
                                 static_cast<double>(prediction.size());
        }
        layer.backward(gradient);
        layer.update_parameters(0.5);
    }
    TYST_EXPECT_LT(mse(layer.forward(input)), before);
}

TYST_TEST(MoETest, PyramidResidualArchitectureHelpers) {
    const auto counts = pyramid_expert_counts(5, 32, 128);
    TYST_ASSERT_EQ(counts.size(), 5u);
    TYST_EXPECT_EQ(counts.front(), 32u);
    TYST_EXPECT_EQ(counts.back(), 128u);
    TYST_EXPECT_GE(counts[3], counts[1]);

    const auto placement = expert_placement(8, 3);
    TYST_ASSERT_EQ(placement.size(), 3u);
    TYST_EXPECT_EQ(placement[0].size(), 3u);
    TYST_EXPECT_EQ(placement[2].size(), 2u);
}

TYST_TEST(MoETest, SparseActivationKeepsPerTokenComputeLow) {
    MoEConfig config;
    config.d_model = 8;
    config.d_ff = 16;
    config.num_experts = 16;
    config.top_k = 1;

    MoELayer layer(config);
    TYST_EXPECT_LT(layer.activation_sparsity(), 0.2);
    TYST_EXPECT_LT(layer.active_parameters_per_token(), layer.total_parameters());
}

TYST_TEST(MoETest, StagedDistillationDecaysTheTeacherWeight) {
    const auto schedule = staged_distillation_schedule(100, 4, 1.0);
    TYST_ASSERT_EQ(schedule.size(), 4u);
    TYST_EXPECT_DOUBLE_EQ(schedule[0].kd_weight, 1.0);
    TYST_EXPECT_DOUBLE_EQ(schedule[3].kd_weight, 0.25);
    TYST_EXPECT_EQ(schedule[3].end_step, 100u);

    // Early on the teacher dominates; late in training the task loss does.
    TYST_EXPECT_DOUBLE_EQ(distillation_loss(4.0, 1.0, schedule, 0), 1.0);
    TYST_EXPECT_NEAR(distillation_loss(4.0, 1.0, schedule, 80), 3.25, 1e-12);
    TYST_EXPECT_DOUBLE_EQ(distillation_loss(4.0, 1.0, schedule, 500), 4.0);
}

// ============================================================================
// ZeroQuant (arXiv:2206.01861)
// ============================================================================

TYST_TEST(ZeroQuantTest, HigherPrecisionQuantizationHasLowerError) {
    Tensor weights = make_tensor({16, 16}, 0.5);

    const double int8_error = quantization_error(weights, fake_quantize(weights, 64, 8));
    const double int4_error = quantization_error(weights, fake_quantize(weights, 64, 4));

    TYST_EXPECT_LT(int8_error, int4_error);
    TYST_EXPECT_LT(int8_error, 1e-4);
}

TYST_TEST(ZeroQuantTest, FinerGroupsBeatPerTensorScaling) {
    Tensor weights = make_tensor({8, 32}, 2.0);
    for (size_t i = 0; i < 8; ++i) {
        weights.data()[i * 32] *= 50.0; // Outliers that a single scale must absorb.
    }

    const double coarse = quantization_error(weights, fake_quantize(weights, weights.size(), 4));
    const double fine = quantization_error(weights, fake_quantize(weights, 16, 4));
    TYST_EXPECT_LT(fine, coarse);
}

TYST_TEST(ZeroQuantTest, TokenWiseActivationQuantizationUsesOneScalePerRow) {
    Tensor activations = make_tensor({6, 10}, 1.1);
    const QuantizedTensor quantized = token_wise_quantize(activations, 8);

    TYST_EXPECT_EQ(quantized.num_groups(), 6u);
    TYST_EXPECT_EQ(quantized.group_size, 10u);
    TYST_EXPECT_LT(quantization_error(activations, dequantize(quantized)), 1e-4);
}

TYST_TEST(ZeroQuantTest, Int4WeightsShrinkTheFootprint) {
    Tensor weights = make_tensor({32, 32}, 0.25);
    const QuantizedTensor int8 = group_wise_quantize(weights, 64, 8);
    const QuantizedTensor int4 = group_wise_quantize(weights, 64, 4);

    TYST_EXPECT_GT(memory_reduction(int4), memory_reduction(int8));
    TYST_EXPECT_GT(memory_reduction(int4), 3.0);
    TYST_EXPECT_THROW(group_wise_quantize(weights, 0, 8), std::invalid_argument);
    TYST_EXPECT_THROW(group_wise_quantize(weights, 64, 1), std::invalid_argument);
}

TYST_TEST(ZeroQuantTest, LayerwiseDistillationRecoversQuantizationLoss) {
    Tensor teacher = make_tensor({8, 8}, 1.7);
    std::vector<Tensor> calibration = {make_tensor({6, 8}, 0.4), make_tensor({6, 8}, 2.2)};

    LayerwiseKnowledgeDistiller::Config config;
    config.iterations = 40;
    config.learning_rate = 0.5;
    config.group_size = 16;
    config.bits = 4;

    LayerwiseKnowledgeDistiller distiller(config);
    const Tensor student = distiller.distill(teacher, calibration);

    const auto& history = distiller.loss_history();
    TYST_ASSERT_FALSE(history.empty());
    TYST_EXPECT_LE(history.back(), history.front());

    // LKD optimizes the layer's output, so the student's quantized output is at
    // least as close to the teacher's as naive round-to-nearest quantization.
    auto output_error = [&](const Tensor& weight) {
        const Tensor quantized = fake_quantize(weight, config.group_size, config.bits);
        double sum = 0.0;
        size_t count = 0;
        for (const Tensor& input : calibration) {
            const Tensor expected = input.matmul(teacher);
            const Tensor actual = input.matmul(quantized);
            for (size_t i = 0; i < expected.size(); ++i) {
                const double difference = actual.data()[i] - expected.data()[i];
                sum += difference * difference;
                ++count;
            }
        }
        return sum / static_cast<double>(count);
    };

    TYST_EXPECT_LE(output_error(student), output_error(teacher) * 1.001);
}

TYST_TEST(ZeroQuantTest, RecipeDefaultsMatchThePaper) {
    QuantizationRecipe recipe;
    TYST_EXPECT_EQ(recipe.attention_weight_bits, 8);
    TYST_EXPECT_EQ(recipe.mlp_weight_bits, 4);
    TYST_EXPECT_EQ(recipe.activation_bits, 8);
}

// ============================================================================
// FastPersist (arXiv:2406.13768)
// ============================================================================

TYST_TEST(FastPersistTest, StripesShardsAcrossWriters) {
    const auto assignment = FastPersistWriter::assign_shards_to_writers(7, 3);
    TYST_ASSERT_EQ(assignment.size(), 3u);
    TYST_EXPECT_EQ(assignment[0].size(), 3u);
    TYST_EXPECT_EQ(assignment[1].size(), 2u);
    TYST_EXPECT_EQ(assignment[2].size(), 2u);
    TYST_EXPECT_THROW(FastPersistWriter::assign_shards_to_writers(4, 0), std::invalid_argument);
}

TYST_TEST(FastPersistTest, RoundTripsCheckpointShards) {
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "coolbox_fastpersist_roundtrip";
    std::filesystem::remove_all(directory);

    FastPersistWriter::Config config;
    config.directory = directory.string();
    config.num_writers = 3;
    FastPersistWriter writer(config);

    std::vector<CheckpointShard> shards;
    for (size_t i = 0; i < 6; ++i) {
        CheckpointShard shard;
        shard.name = "layer." + std::to_string(i) + ".weight";
        shard.data.assign(32, static_cast<double>(i) + 0.5);
        shards.push_back(std::move(shard));
    }

    const PersistResult result = writer.write("step100", shards);
    TYST_EXPECT_EQ(result.files.size(), 3u);
    TYST_EXPECT_EQ(result.bytes, 6u * 32u * sizeof(double));

    const auto restored = writer.read("step100");
    TYST_ASSERT_EQ(restored.size(), 6u);
    TYST_EXPECT_EQ(restored[0].name, std::string("layer.0.weight"));
    TYST_EXPECT_NEAR(restored[0].data[0], 0.5, 1e-12);
    TYST_EXPECT_NEAR(restored[5].data[31], 5.5, 1e-12);

    std::filesystem::remove_all(directory);
}

TYST_TEST(FastPersistTest, AsynchronousWriteOverlapsWithCompute) {
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "coolbox_fastpersist_async";
    std::filesystem::remove_all(directory);

    FastPersistWriter::Config config;
    config.directory = directory.string();
    config.num_writers = 2;
    config.overlap_with_compute = true;
    FastPersistWriter writer(config);

    std::vector<CheckpointShard> shards(4);
    for (size_t i = 0; i < shards.size(); ++i) {
        shards[i].name = "shard" + std::to_string(i);
        shards[i].data.assign(1024, static_cast<double>(i));
    }

    writer.write_async("iter1", shards);
    TYST_EXPECT_TRUE(writer.has_pending_write());

    double simulated_compute = 0.0;
    for (size_t i = 0; i < 100000; ++i) {
        simulated_compute += static_cast<double>(i) * 1e-9;
    }

    const PersistResult result = writer.wait();
    TYST_EXPECT_FALSE(writer.has_pending_write());
    TYST_EXPECT_EQ(result.bytes, 4u * 1024u * sizeof(double));
    TYST_EXPECT_GT(result.throughput(), 0.0);
    TYST_EXPECT_GT(simulated_compute, 0.0);

    TYST_EXPECT_EQ(writer.read("iter1").size(), 4u);
    std::filesystem::remove_all(directory);
}

TYST_TEST(FastPersistTest, ParallelismAndOverlapCutCheckpointCost) {
    const size_t bytes = 8ull * 1024 * 1024 * 1024;
    const double per_ssd = 2.0e9;

    const double serial = FastPersistWriter::estimated_seconds(bytes, per_ssd, 1, 1.0, false);
    const double parallel = FastPersistWriter::estimated_seconds(bytes, per_ssd, 8, 1.0, false);
    const double overlapped = FastPersistWriter::estimated_seconds(bytes, per_ssd, 8, 1.0, true);

    TYST_EXPECT_LT(parallel, serial);
    TYST_EXPECT_LT(overlapped, parallel);
}

// ============================================================================
// Universal Checkpointing (arXiv:2406.18820)
// ============================================================================

namespace {

std::vector<RankState> shard_for_test(const std::vector<ParameterSpec>& specs,
                                      const ParallelConfig& config,
                                      const std::map<std::string, std::vector<double>>& atoms) {
    UniversalCheckpoint seeded = UniversalCheckpoint::consolidate(
        specs,
        ParallelConfig{1, 1, config.pipeline_parallel},
        [&]() {
            std::vector<RankState> single(config.pipeline_parallel);
            for (size_t stage = 0; stage < config.pipeline_parallel; ++stage) {
                single[stage].rank = stage;
                for (const auto& spec : specs) {
                    if (spec.pipeline_stage == stage) {
                        single[stage].tensors[spec.name] = atoms.at(spec.name);
                    }
                }
            }
            return single;
        }());
    return seeded.reconfigure(config);
}

} // namespace

TYST_TEST(UniversalCheckpointTest, RankIndexingIsConsistent) {
    ParallelConfig config{2, 3, 2};
    TYST_EXPECT_EQ(config.world_size(), 12u);
    TYST_EXPECT_EQ(config.rank_of(0, 0, 0), 0u);
    TYST_EXPECT_EQ(config.rank_of(0, 1, 2), 5u);
    TYST_EXPECT_EQ(config.rank_of(1, 0, 0), 6u);
    TYST_EXPECT_THROW(config.rank_of(2, 0, 0), std::out_of_range);
    TYST_EXPECT_EQ(to_string(ParamPattern::kColumnParallel), std::string("column_parallel"));
}

TYST_TEST(UniversalCheckpointTest, ReconfiguresTensorParallelismFromTwoToFour) {
    std::vector<double> row_weight(24);
    std::iota(row_weight.begin(), row_weight.end(), 0.0);
    std::vector<double> column_weight(24);
    std::iota(column_weight.begin(), column_weight.end(), 100.0);
    std::vector<double> norm = {1.0, 2.0, 3.0, 4.0};

    const std::vector<ParameterSpec> specs = {
        {"mlp.h_to_4h", {4, 6}, ParamPattern::kRowParallel, 0},
        {"mlp.4h_to_h", {4, 6}, ParamPattern::kColumnParallel, 0},
        {"input_layernorm", {4}, ParamPattern::kReplicated, 0}
    };
    const std::map<std::string, std::vector<double>> atoms = {
        {"mlp.h_to_4h", row_weight},
        {"mlp.4h_to_h", column_weight},
        {"input_layernorm", norm}
    };

    const ParallelConfig source{1, 2, 1};
    const auto source_states = shard_for_test(specs, source, atoms);
    TYST_ASSERT_EQ(source_states.size(), 2u);
    TYST_EXPECT_EQ(source_states[0].tensors.at("mlp.h_to_4h").size(), 12u);
    TYST_EXPECT_EQ(source_states[0].tensors.at("mlp.4h_to_h").size(), 12u);

    const UniversalCheckpoint checkpoint =
        UniversalCheckpoint::consolidate(specs, source, source_states);
    TYST_EXPECT_EQ(checkpoint.atom_count(), 3u);
    TYST_EXPECT_TRUE(checkpoint.contains("input_layernorm"));
    for (size_t i = 0; i < row_weight.size(); ++i) {
        TYST_EXPECT_DOUBLE_EQ(checkpoint.atom("mlp.h_to_4h")[i], row_weight[i]);
        TYST_EXPECT_DOUBLE_EQ(checkpoint.atom("mlp.4h_to_h")[i], column_weight[i]);
    }

    const ParallelConfig target{1, 4, 1};
    const auto target_states = checkpoint.reconfigure(target);
    TYST_ASSERT_EQ(target_states.size(), 4u);
    TYST_EXPECT_EQ(target_states[0].tensors.at("mlp.h_to_4h").size(), 6u);
    TYST_EXPECT_EQ(target_states[0].tensors.at("input_layernorm").size(), 4u);

    // Round-tripping back through consolidation reproduces the atoms exactly.
    const UniversalCheckpoint reconsolidated =
        UniversalCheckpoint::consolidate(specs, target, target_states);
    for (size_t i = 0; i < row_weight.size(); ++i) {
        TYST_EXPECT_DOUBLE_EQ(reconsolidated.atom("mlp.h_to_4h")[i], row_weight[i]);
        TYST_EXPECT_DOUBLE_EQ(reconsolidated.atom("mlp.4h_to_h")[i], column_weight[i]);
    }
}

TYST_TEST(UniversalCheckpointTest, ReconfiguresZeroFlattenedShardsAcrossDataParallelism) {
    std::vector<double> flat(30);
    std::iota(flat.begin(), flat.end(), 1.0);

    const std::vector<ParameterSpec> specs = {
        {"optimizer.exp_avg", {30}, ParamPattern::kFlattenedShard, 0}
    };
    const std::map<std::string, std::vector<double>> atoms = {{"optimizer.exp_avg", flat}};

    const ParallelConfig source{4, 1, 1};
    const auto source_states = shard_for_test(specs, source, atoms);
    TYST_EXPECT_EQ(source_states[0].tensors.at("optimizer.exp_avg").size(), 8u);
    TYST_EXPECT_EQ(source_states[3].tensors.at("optimizer.exp_avg").size(), 7u);

    const UniversalCheckpoint checkpoint =
        UniversalCheckpoint::consolidate(specs, source, source_states);
    const auto target_states = checkpoint.reconfigure(ParallelConfig{7, 1, 1});
    TYST_ASSERT_EQ(target_states.size(), 7u);

    std::vector<double> rejoined;
    for (const auto& state : target_states) {
        const auto& shard = state.tensors.at("optimizer.exp_avg");
        rejoined.insert(rejoined.end(), shard.begin(), shard.end());
    }
    TYST_ASSERT_EQ(rejoined.size(), flat.size());
    for (size_t i = 0; i < flat.size(); ++i) {
        TYST_EXPECT_DOUBLE_EQ(rejoined[i], flat[i]);
    }
}

TYST_TEST(UniversalCheckpointTest, ReportsMissingAtoms) {
    UniversalCheckpoint empty;
    TYST_EXPECT_FALSE(empty.contains("missing"));
    TYST_EXPECT_THROW(empty.atom("missing"), std::out_of_range);
}

// ============================================================================
// Domino (arXiv:2409.15241)
// ============================================================================

TYST_TEST(DominoTest, RowAndColumnSlicingAreLossless) {
    Tensor tensor = make_tensor({6, 8}, 0.6);

    const Tensor rows = DominoScheduler::concat_rows(DominoScheduler::split_rows(tensor, 4));
    const Tensor columns =
        DominoScheduler::concat_columns(DominoScheduler::split_columns(tensor, 3));

    TYST_ASSERT_EQ(rows.shape(), tensor.shape());
    TYST_ASSERT_EQ(columns.shape(), tensor.shape());
    for (size_t i = 0; i < tensor.size(); ++i) {
        TYST_EXPECT_DOUBLE_EQ(rows.data()[i], tensor.data()[i]);
        TYST_EXPECT_DOUBLE_EQ(columns.data()[i], tensor.data()[i]);
    }
}

TYST_TEST(DominoTest, SlicedForwardMatchesTheUnslicedProduct) {
    Tensor input = make_tensor({6, 5}, 0.15);
    Tensor weight = make_tensor({5, 7}, 1.05);

    DominoConfig config;
    config.row_chunks = 3;
    config.column_chunks = 2;
    DominoScheduler scheduler(config);

    size_t collectives = 0;
    const Tensor sliced = scheduler.forward_sliced(
        input, weight, [&](Tensor&, size_t) { ++collectives; });
    const Tensor reference = input.matmul(weight);

    TYST_ASSERT_EQ(sliced.shape(), reference.shape());
    for (size_t i = 0; i < reference.size(); ++i) {
        TYST_EXPECT_NEAR(sliced.data()[i], reference.data()[i], 1e-12);
    }
    TYST_EXPECT_EQ(collectives, 3u);
}

TYST_TEST(DominoTest, ScheduleOverlapsEveryChunkButTheFirst) {
    DominoConfig config;
    config.row_chunks = 4;
    DominoScheduler scheduler(config);

    const auto schedule = scheduler.build_schedule(2);
    TYST_ASSERT_EQ(schedule.size(), 8u);
    TYST_EXPECT_FALSE(schedule[0].has_overlapped_comm);
    TYST_EXPECT_TRUE(schedule[1].has_overlapped_comm);
    TYST_EXPECT_EQ(schedule[3].overlapped_comm_chunk, 2u);
    TYST_EXPECT_EQ(schedule[4].layer, 1u);
    TYST_EXPECT_EQ(schedule[4].compute_tag, std::string("layer1.chunk0"));
}

TYST_TEST(DominoTest, OverlapHidesMostOfTheCollectiveCost) {
    DominoConfig overlapped;
    overlapped.row_chunks = 4;
    overlapped.overlap = true;

    DominoConfig serial;
    serial.row_chunks = 4;
    serial.overlap = false;

    DominoScheduler fast(overlapped);
    DominoScheduler slow(serial);

    TYST_EXPECT_NEAR(fast.hidden_communication_fraction(), 0.75, 1e-12);
    TYST_EXPECT_DOUBLE_EQ(slow.hidden_communication_fraction(), 0.0);

    const double compute = 1.0;
    const double communication = 0.5;
    TYST_EXPECT_LT(fast.estimated_iteration_seconds(compute, communication, 8),
                   slow.estimated_iteration_seconds(compute, communication, 8));
    TYST_EXPECT_GT(fast.speedup(compute, communication, 8), 1.0);
    TYST_EXPECT_DOUBLE_EQ(slow.speedup(compute, communication, 8), 1.0);
}

// ============================================================================
// Collective primitives shared by the ZeRO and Domino implementations
// ============================================================================

TYST_TEST(CollectiveTest, PartitionsAndReducesAcrossRanks) {
    const auto ranges = partition_evenly(10, 4);
    TYST_ASSERT_EQ(ranges.size(), 4u);
    TYST_EXPECT_EQ(ranges[0].count, 3u);
    TYST_EXPECT_EQ(ranges[3].end(), 10u);
    TYST_EXPECT_TRUE(ranges[1].contains(3));
    TYST_EXPECT_THROW(partition_evenly(10, 0), std::invalid_argument);

    CollectiveGroup group(3);
    std::vector<std::vector<double>> buffers = {{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}};
    group.all_reduce(buffers);
    for (const auto& buffer : buffers) {
        TYST_EXPECT_DOUBLE_EQ(buffer[0], 9.0);
        TYST_EXPECT_DOUBLE_EQ(buffer[1], 12.0);
    }
    TYST_EXPECT_GT(group.bytes_moved(), 0.0);
    group.reset_counters();
    TYST_EXPECT_DOUBLE_EQ(group.bytes_moved(), 0.0);
}

TYST_TEST(CollectiveTest, ReduceScatterThenAllGatherReproducesAllReduce) {
    CollectiveGroup group(2);
    std::vector<std::vector<double>> buffers = {{1.0, 2.0, 3.0, 4.0}, {5.0, 6.0, 7.0, 8.0}};

    std::vector<std::vector<double>> shards;
    group.reduce_scatter(buffers, shards);
    TYST_ASSERT_EQ(shards.size(), 2u);
    TYST_EXPECT_DOUBLE_EQ(shards[0][0], 6.0);
    TYST_EXPECT_DOUBLE_EQ(shards[1][1], 12.0);

    std::vector<std::vector<double>> gathered;
    group.all_gather(shards, gathered);
    TYST_ASSERT_EQ(gathered[0].size(), 4u);
    TYST_EXPECT_DOUBLE_EQ(gathered[0][3], 12.0);
}
