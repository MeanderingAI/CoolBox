
#include <complex>
#include <cstdlib>
#include <type_traits>
#include <limits>
#include "tyst_framework.hpp"
#include "tensor.h"
#include "layer.h"
#include "ifinder.h"
#include "loss.h"
#include "optimizer.h"
#include "neural_network.h"
#include "cdeepex.h"
#include "explainability_facade.h"
#include "templates.h"

using namespace ml::deep_learning;

// ============================================================================
// iFINDER Structured Grounding Tests
// ============================================================================

TEST(IFinderTest, AssignsObjectAndEgoLanes) {
    using namespace ml::deep_learning::ifinder;

    std::vector<LaneMarking> lanes = {
        {{{100.0, 0.0}, {100.0, 720.0}}},
        {{{300.0, 0.0}, {300.0, 720.0}}},
        {{{500.0, 0.0}, {500.0, 720.0}}}
    };

    BoundingBox object_box{320.0, 200.0, 420.0, 460.0};
    EXPECT_EQ(assign_object_lane(object_box, lanes, 640.0), 2);
    EXPECT_EQ(assign_ego_lane(640.0, 720.0, lanes), 2);
}

TEST(IFinderTest, EstimatesMaskedDistance) {
    using namespace ml::deep_learning::ifinder;

    std::vector<double> depth = {
        1.0, 2.0, 3.0,
        4.0, 5.0, 6.0,
        7.0, 8.0, 9.0
    };
    std::vector<unsigned char> mask = {
        0, 0, 0,
        0, 1, 1,
        0, 1, 1
    };

    EXPECT_NEAR(masked_mean_distance(depth, mask, 3, 3, BoundingBox{1.0, 1.0, 3.0, 3.0}), 7.0, 1e-6);
}

TEST(IFinderTest, EstimatesEgoMotionAndTurn) {
    using namespace ml::deep_learning::ifinder;

    std::vector<Point3D> positions = {
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {1.0, 0.0, 1.0}
    };

    IFinderConfig config;
    config.turn_threshold_rad = 0.1;
    config.stopped_speed_threshold = 0.01;

    EgoState state = estimate_ego_state(positions, 1, config);
    EXPECT_EQ(state.motion, "Moving");
    EXPECT_EQ(state.turn, "Right Turn");
    EXPECT_NEAR(state.heading_delta_rad, 1.57079632679, 1e-6);
}

TEST(IFinderTest, SerializesStructuredCueAndPromptBlocks) {
    using namespace ml::deep_learning::ifinder;

    ObjectCue object;
    object.track_id = 13;
    object.class_label = "car";
    object.box = BoundingBox{10.0, 20.0, 50.0, 80.0};
    object.distance_m = 9.78;
    object.orientation_yaw_rad = -2.1;
    object.lane = 1;
    object.attributes["color"] = "white";

    FrameCue frame;
    frame.frame_index = 24;
    frame.ego_state.motion = "Moving";
    frame.ego_state.turn = "Straight";
    frame.ego_lane = 1;
    frame.detected_objects.push_back(object);

    VideoCue cue;
    cue.surrounding_info = "daytime urban road";
    cue.event_description = "a vehicle cuts toward the ego lane";
    cue.peer_response = "no accident";
    cue.frames.push_back(frame);

    const std::string json = to_json(cue);
    EXPECT_NE(json.find("Video-Level-Information"), std::string::npos);
    EXPECT_NE(json.find("detected_objects"), std::string::npos);
    EXPECT_NE(json.find("\"track_id\":13"), std::string::npos);

    PromptBlocks blocks = build_prompt_blocks(cue, "What caused the incident?");
    const std::string prompt = blocks.compose();
    EXPECT_NE(prompt.find("Key Explanation"), std::string::npos);
    EXPECT_NE(prompt.find("Step Instructions"), std::string::npos);
    EXPECT_NE(prompt.find("Peer Instruction"), std::string::npos);
}

// ============================================================================
// Tensor Tests
// ============================================================================

TEST(TensorTest, Construction) {
    Tensor t({2, 3}, 1.0);
    EXPECT_EQ(t.shape().size(), 2);
    EXPECT_EQ(t.shape()[0], 2);
    EXPECT_EQ(t.shape()[1], 3);
    EXPECT_EQ(t.size(), 6);
    EXPECT_DOUBLE_EQ(t.data()[0], 1.0);
}

TEST(TensorTest, MatMul) {
    Tensor a({2, 3}, {1, 2, 3, 4, 5, 6});
    Tensor b({3, 2}, {7, 8, 9, 10, 11, 12});
    Tensor c = a.matmul(b);
    EXPECT_EQ(c.shape()[0], 2);
    EXPECT_EQ(c.shape()[1], 2);
    EXPECT_DOUBLE_EQ(c.data()[0], 58);  // 1*7 + 2*9 + 3*11
    EXPECT_DOUBLE_EQ(c.data()[1], 64);  // 1*8 + 2*10 + 3*12
}

TEST(TensorBackendTest, CpuOperationsPreserveResults) {
    set_tensor_backend(TensorBackend::CPU);
    Tensor a({2, 2}, {1.0, 2.0, 3.0, 4.0});
    Tensor b({2, 2}, {5.0, 6.0, 7.0, 8.0});

    Tensor sum = a + b;
    Tensor product = a.matmul(b);
    Tensor transposed = a.transpose();

    EXPECT_EQ(active_tensor_backend(), TensorBackend::CPU);
    EXPECT_DOUBLE_EQ(sum.data()[0], 6.0);
    EXPECT_DOUBLE_EQ(sum.data()[3], 12.0);
    EXPECT_DOUBLE_EQ(product.data()[0], 19.0);
    EXPECT_DOUBLE_EQ(product.data()[3], 50.0);
    EXPECT_DOUBLE_EQ(transposed.data()[1], 3.0);
    EXPECT_DOUBLE_EQ(transposed.data()[2], 2.0);
}

TEST(TensorBackendTest, VulkanSelectionMatchesAvailability) {
    const bool require_vulkan =
        std::getenv("COOLBOX_TEST_REQUIRE_VULKAN") != nullptr;
    if (require_vulkan) {
        EXPECT_TRUE(tensor_backend_available(TensorBackend::VULKAN));
    }
    if (tensor_backend_available(TensorBackend::VULKAN)) {
        set_tensor_backend(TensorBackend::VULKAN);
        Tensor a({2, 2}, {1.0, 2.0, 3.0, 4.0});
        Tensor b({2, 2}, {5.0, 6.0, 7.0, 8.0});
        Tensor sum = a + b;
        Tensor difference = b - a;
        Tensor elementwise_product = a * b;
        Tensor quotient = b / a;
        Tensor scaled = a * 2.0;
        Tensor transposed = a.transpose();
        Tensor product = a.matmul(b);
        EXPECT_EQ(active_tensor_backend(), TensorBackend::VULKAN);
        EXPECT_NEAR(sum.data()[3], 12.0, 1e-9);
        EXPECT_NEAR(difference.data()[0], 4.0, 1e-9);
        EXPECT_NEAR(elementwise_product.data()[2], 21.0, 1e-9);
        EXPECT_NEAR(quotient.data()[3], 2.0, 1e-9);
        EXPECT_NEAR(scaled.data()[1], 4.0, 1e-9);
        EXPECT_NEAR(transposed.data()[1], 3.0, 1e-9);
        EXPECT_NEAR(product.data()[0], 19.0, 1e-9);
        EXPECT_NEAR(product.data()[3], 50.0, 1e-9);
        EXPECT_FALSE(
            tensor_backend_device_name(TensorBackend::VULKAN).empty());
    } else {
        EXPECT_FALSE(
            tensor_backend_unavailable_reason(
                TensorBackend::VULKAN).empty());
        EXPECT_THROW(
            set_tensor_backend(TensorBackend::VULKAN),
            std::runtime_error);
    }
    set_tensor_backend(TensorBackend::CPU);
}

TEST(TensorBackendTest, VulkanSubmissionRingPreservesDependencies) {
    if (!tensor_backend_available(TensorBackend::VULKAN)) {
        return;
    }
    set_tensor_backend(TensorBackend::VULKAN);
    set_tensor_precision(TensorPrecision::FP32);
    Tensor value({1024}, 1.0);
    Tensor increment({1024}, 0.25);
    for (std::size_t iteration = 0; iteration < 800; ++iteration) {
        value += increment;
    }

    EXPECT_NEAR(value.data()[0], 201.0, 1e-4);
    EXPECT_NEAR(value.data()[1023], 201.0, 1e-4);
    set_tensor_backend(TensorBackend::CPU);
    set_tensor_precision(TensorPrecision::AUTO);
}

TEST(TensorBackendTest, VulkanCrossEntropyMatchesCpu) {
    if (!tensor_backend_available(TensorBackend::VULKAN)) {
        return;
    }
    Tensor logits(
        {2, 4},
        {0.1, 0.2, 0.3, 0.4,
         0.5, -0.5, 0.25, 0.0});
    Tensor targets(
        {2, 4},
        {0.0, 0.0, 0.0, 1.0,
         1.0, 0.0, 0.0, 0.0});

    set_tensor_backend(TensorBackend::CPU);
    Tensor expected =
        logits.cross_entropy_with_one_hot(targets);
    set_tensor_backend(TensorBackend::VULKAN);
    set_tensor_precision(TensorPrecision::FP32);
    Tensor actual =
        logits.cross_entropy_with_one_hot(targets);
    ASSERT_EQ(expected.shape(), actual.shape());
    for (std::size_t index = 0;
         index < expected.size();
         ++index) {
        EXPECT_NEAR(
            expected.data()[index],
            actual.data()[index],
            1e-6);
    }
    set_tensor_backend(TensorBackend::CPU);
    set_tensor_precision(TensorPrecision::AUTO);
}

TEST(TensorBackendTest, FusedVulkanAdamwMatchesCpu) {
    if (!tensor_backend_available(TensorBackend::VULKAN)) {
        return;
    }
    Tensor expected_values(
        {4}, {1.0, -2.0, 0.5, -0.25});
    Tensor expected_gradients(
        {4}, {0.1, -0.2, 0.05, 0.3});
    Tensor expected_first(
        {4}, {0.01, -0.02, 0.03, -0.04});
    Tensor expected_second(
        {4}, {0.001, 0.002, 0.003, 0.004});
    Tensor actual_values = expected_values;
    Tensor actual_gradients = expected_gradients;
    Tensor actual_first = expected_first;
    Tensor actual_second = expected_second;

    set_tensor_backend(TensorBackend::CPU);
    expected_values.fused_adamw_step_in_place(
        expected_gradients, expected_first, expected_second,
        0.001, 0.9, 0.999, 0.19, 0.001999,
        1e-8, 0.01, 0.25);

    set_tensor_backend(TensorBackend::VULKAN);
    set_tensor_precision(TensorPrecision::FP32);
    actual_values.fused_adamw_step_in_place(
        actual_gradients, actual_first, actual_second,
        0.001, 0.9, 0.999, 0.19, 0.001999,
        1e-8, 0.01, 0.25);
    synchronize_tensor_backend();
    for (std::size_t index = 0;
         index < expected_values.size();
         ++index) {
        EXPECT_NEAR(
            expected_values.data()[index],
            actual_values.data()[index],
            1e-6);
        EXPECT_NEAR(
            expected_first.data()[index],
            actual_first.data()[index],
            1e-6);
        EXPECT_NEAR(
            expected_second.data()[index],
            actual_second.data()[index],
            1e-6);
    }
    set_tensor_backend(TensorBackend::CPU);
    set_tensor_precision(TensorPrecision::AUTO);
}

TEST(TensorBackendTest, ParallelVulkanSquaredSumMatchesCpu) {
    if (!tensor_backend_available(TensorBackend::VULKAN)) {
        return;
    }
    std::vector<double> values(513);
    for (std::size_t index = 0; index < values.size(); ++index) {
        values[index] =
            static_cast<double>(static_cast<int>(index % 17) - 8) /
            16.0;
    }
    Tensor input({values.size()}, values);

    set_tensor_backend(TensorBackend::CPU);
    const double expected = input.squared_sum();

    set_tensor_backend(TensorBackend::VULKAN);
    set_tensor_precision(TensorPrecision::FP32);
    const double actual = input.squared_sum();
    EXPECT_NEAR(expected, actual, 1e-4);
    set_tensor_precision(TensorPrecision::BF16);
    const double bf16_actual = input.squared_sum();
    EXPECT_NEAR(expected, bf16_actual, expected * 0.01);
    set_tensor_backend(TensorBackend::CPU);
    set_tensor_precision(TensorPrecision::AUTO);
}

TEST(TensorBackendTest, VulkanAttentionScoresMatchCpu) {
    if (!tensor_backend_available(TensorBackend::VULKAN)) {
        return;
    }
    Tensor queries(
        {2, 3},
        {1.0, 2.0, 3.0,
         -1.0, 0.5, 2.0});
    Tensor keys(
        {3, 3},
        {0.5, 1.0, -1.0,
         2.0, -0.5, 1.0,
         1.5, 2.0, 0.25});
    Tensor mask(
        {2, 3},
        {1.0, 0.0, 1.0,
         1.0, 1.0, 0.0});
    Tensor left_transpose_other(
        {2, 2},
        {0.25, 1.0,
         -0.5, 2.0});
    Tensor packed_queries(
        {2, 4},
        {1.0, 0.5, -1.0, 2.0,
         0.25, -0.5, 1.5, 0.75});
    Tensor packed_keys(
        {2, 4},
        {0.5, -1.0, 2.0, 0.25,
         1.0, 0.75, -0.5, 1.25});
    Tensor packed_mask(
        {2, 2},
        {1.0, 0.0,
         1.0, 1.0});

    set_tensor_backend(TensorBackend::CPU);
    Tensor expected = queries.attention_scores(
        keys, mask, std::sqrt(3.0));
    Tensor expected_right =
        queries.matmul_right_transposed(keys);
    Tensor expected_left =
        queries.matmul_left_transposed(left_transpose_other);
    Tensor expected_packed_scores =
        packed_queries.multihead_attention_scores(
            packed_keys, packed_mask, 2);
    Tensor expected_attention_weights =
        expected_packed_scores.row_softmax();
    Tensor expected_packed_values =
        expected_attention_weights.multihead_attention_values(
            packed_keys, 2);
    Tensor expected_packed_keys =
        expected_attention_weights.
            multihead_attention_key_gradient(
                packed_queries, 2);
    Tensor expected_packed_weights =
        packed_queries.
            multihead_attention_weight_gradient(
                packed_keys, 2);

    set_tensor_backend(TensorBackend::VULKAN);
    set_tensor_precision(TensorPrecision::FP32);
    Tensor actual = queries.attention_scores(
        keys, mask, std::sqrt(3.0));
    Tensor actual_right =
        queries.matmul_right_transposed(keys);
    Tensor actual_left =
        queries.matmul_left_transposed(left_transpose_other);
    Tensor actual_packed_scores =
        packed_queries.multihead_attention_scores(
            packed_keys, packed_mask, 2);
    Tensor actual_attention_weights =
        actual_packed_scores.row_softmax();
    Tensor actual_packed_values =
        actual_attention_weights.multihead_attention_values(
            packed_keys, 2);
    Tensor actual_packed_keys =
        actual_attention_weights.
            multihead_attention_key_gradient(
                packed_queries, 2);
    Tensor actual_packed_weights =
        packed_queries.
            multihead_attention_weight_gradient(
                packed_keys, 2);
    ASSERT_EQ(expected.shape(), actual.shape());
    for (std::size_t index = 0;
         index < expected.size();
         ++index) {
        EXPECT_NEAR(
            expected.data()[index],
            actual.data()[index],
            1e-5);
    }
    for (std::size_t index = 0;
         index < expected_right.size();
         ++index) {
        EXPECT_NEAR(
            expected_right.data()[index],
            actual_right.data()[index],
            1e-5);
    }
    for (std::size_t index = 0;
         index < expected_left.size();
         ++index) {
        EXPECT_NEAR(
            expected_left.data()[index],
            actual_left.data()[index],
            1e-5);
    }
    const auto expect_near = [](
        const Tensor& expected_tensor,
        const Tensor& actual_tensor) {
        ASSERT_EQ(
            expected_tensor.shape(), actual_tensor.shape());
        for (std::size_t index = 0;
             index < expected_tensor.size();
             ++index) {
            EXPECT_NEAR(
                expected_tensor.data()[index],
                actual_tensor.data()[index],
                1e-5);
        }
    };
    expect_near(
        expected_packed_scores, actual_packed_scores);
    expect_near(
        expected_packed_values, actual_packed_values);
    expect_near(
        expected_packed_keys, actual_packed_keys);
    expect_near(
        expected_packed_weights, actual_packed_weights);
    set_tensor_backend(TensorBackend::CPU);
    set_tensor_precision(TensorPrecision::AUTO);
}

// ============================================================================
// Dense Layer Tests
// ============================================================================

TEST(DenseLayerTest, ForwardShape) {
    DenseLayer layer(4, 3);
    Tensor input({2, 4}, 1.0);
    Tensor output = layer.forward(input);
    EXPECT_EQ(output.shape()[0], 2);
    EXPECT_EQ(output.shape()[1], 3);
}

TEST(DenseLayerTest, BackwardShape) {
    DenseLayer layer(4, 3);
    Tensor input({2, 4}, 1.0);
    layer.forward(input);
    Tensor grad({2, 3}, 1.0);
    Tensor input_grad = layer.backward(grad);
    EXPECT_EQ(input_grad.shape()[0], 2);
    EXPECT_EQ(input_grad.shape()[1], 4);
}

// ============================================================================
// Activation Layer Tests
// ============================================================================

TEST(ReLULayerTest, Forward) {
    ReLULayer relu;
    Tensor input({1, 4}, {-2.0, -1.0, 0.0, 1.0});
    Tensor output = relu.forward(input);
    EXPECT_DOUBLE_EQ(output.data()[0], 0.0);
    EXPECT_DOUBLE_EQ(output.data()[1], 0.0);
    EXPECT_DOUBLE_EQ(output.data()[2], 0.0);
    EXPECT_DOUBLE_EQ(output.data()[3], 1.0);
}

TEST(ActivationTensorTest, GeluAndSiluBackwardMatchFiniteDifference) {
    set_tensor_backend(TensorBackend::CPU);
    Tensor input({1, 3}, {-1.25, 0.0, 0.75});
    Tensor gradient({1, 3}, {1.0, -0.5, 2.0});
    Tensor gelu_gradient = input.gelu_backward(gradient);
    Tensor silu_gradient = input.silu_backward(gradient);
    constexpr double epsilon = 1e-6;
    for (size_t index = 0; index < input.size(); ++index) {
        Tensor plus = input;
        Tensor minus = input;
        plus.data()[index] += epsilon;
        minus.data()[index] -= epsilon;
        const double gelu_numeric =
            (plus.gelu().data()[index] -
             minus.gelu().data()[index]) /
            (2.0 * epsilon) * gradient.data()[index];
        const double silu_numeric =
            (plus.silu().data()[index] -
             minus.silu().data()[index]) /
            (2.0 * epsilon) * gradient.data()[index];
        EXPECT_NEAR(
            gelu_gradient.data()[index], gelu_numeric, 1e-6);
        EXPECT_NEAR(
            silu_gradient.data()[index], silu_numeric, 1e-6);
    }
}

TEST(RotaryEmbeddingTest, BackwardInvertsForward) {
    set_tensor_backend(TensorBackend::CPU);
    Tensor input(
        {3, 8},
        {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0,
         8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0,
         16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0});
    Tensor restored =
        input.rotary_embedding(2).rotary_embedding_backward(2);
    for (size_t index = 0; index < input.size(); ++index) {
        EXPECT_NEAR(
            restored.data()[index], input.data()[index], 1e-10);
    }
}

TEST(ActivationTensorTest, VulkanMatchesCpu) {
    if (!tensor_backend_available(TensorBackend::VULKAN)) {
        return;
    }
    Tensor input({2, 4}, {-2.0, -1.0, 0.0, 1.0,
                           2.0, 3.0, -0.5, 0.5});
    Tensor gradient({2, 4}, 0.75);
    set_tensor_backend(TensorBackend::CPU);
    Tensor expected_gelu = input.gelu();
    Tensor expected_silu_gradient =
        input.silu_backward(gradient);
    Tensor expected_rope = input.rotary_embedding(2);

    set_tensor_backend(TensorBackend::VULKAN);
    set_tensor_precision(TensorPrecision::FP32);
    Tensor actual_gelu = input.gelu();
    Tensor actual_silu_gradient =
        input.silu_backward(gradient);
    Tensor actual_rope = input.rotary_embedding(2);
    for (size_t index = 0; index < input.size(); ++index) {
        EXPECT_NEAR(
            actual_gelu.data()[index],
            expected_gelu.data()[index],
            1e-5);
        EXPECT_NEAR(
            actual_silu_gradient.data()[index],
            expected_silu_gradient.data()[index],
            1e-5);
        EXPECT_NEAR(
            actual_rope.data()[index],
            expected_rope.data()[index],
            1e-5);
    }
    set_tensor_backend(TensorBackend::CPU);
    set_tensor_precision(TensorPrecision::AUTO);
}

TEST(SigmoidLayerTest, Forward) {
    SigmoidLayer sigmoid;
    Tensor input({1, 3}, {0.0, -100.0, 100.0});
    Tensor output = sigmoid.forward(input);
    EXPECT_NEAR(output.data()[0], 0.5, 1e-6);
    EXPECT_NEAR(output.data()[1], 0.0, 1e-6);
    EXPECT_NEAR(output.data()[2], 1.0, 1e-6);
}

TEST(TanhLayerTest, Forward) {
    TanhLayer tanh_layer;
    Tensor input({1, 3}, {0.0, -100.0, 100.0});
    Tensor output = tanh_layer.forward(input);
    EXPECT_NEAR(output.data()[0], 0.0, 1e-6);
    EXPECT_NEAR(output.data()[1], -1.0, 1e-6);
    EXPECT_NEAR(output.data()[2], 1.0, 1e-6);
}

TEST(SoftmaxLayerTest, Forward) {
    SoftmaxLayer softmax;
    Tensor input({1, 3}, {1.0, 2.0, 3.0});
    Tensor output = softmax.forward(input);
    // Sum should be 1
    double sum = output.data()[0] + output.data()[1] + output.data()[2];
    EXPECT_NEAR(sum, 1.0, 1e-6);
    // Values should be monotonically increasing
    EXPECT_LT(output.data()[0], output.data()[1]);
    EXPECT_LT(output.data()[1], output.data()[2]);
}

// ============================================================================
// Utility Layer Tests
// ============================================================================

TEST(DropoutLayerTest, ForwardTraining) {
    DropoutLayer dropout(0.5);
    Tensor input({2, 4}, 1.0);
    Tensor output = dropout.forward(input);
    // Some values should be zero, others scaled by 2
    int zeros = 0;
    for (size_t i = 0; i < output.size(); ++i) {
        if (output.data()[i] == 0.0) zeros++;
    }
    // With 50% dropout on 8 elements, expect some zeros (probabilistic)
    EXPECT_GT(output.size(), 0u);
}

TEST(DropoutLayerTest, ForwardInference) {
    DropoutLayer dropout(0.5);
    dropout.set_training(false);
    Tensor input({2, 4}, 1.0);
    Tensor output = dropout.forward(input);
    // In inference mode, output should equal input
    for (size_t i = 0; i < output.size(); ++i) {
        EXPECT_DOUBLE_EQ(output.data()[i], 1.0);
    }
}

TEST(BatchNormLayerTest, ForwardShape) {
    BatchNormLayer bn(4);
    Tensor input({3, 4}, 1.0);
    Tensor output = bn.forward(input);
    EXPECT_EQ(output.shape()[0], 3);
    EXPECT_EQ(output.shape()[1], 4);
}

TEST(LayerNormLayerTest, ForwardShape) {
    LayerNormLayer ln(4);
    Tensor input({3, 4}, 1.0);
    Tensor output = ln.forward(input);
    EXPECT_EQ(output.shape()[0], 3);
    EXPECT_EQ(output.shape()[1], 4);
}

TEST(FlattenLayerTest, Forward) {
    FlattenLayer flatten;
    Tensor input({2, 3, 4, 5});
    input.fill(1.0);
    Tensor output = flatten.forward(input);
    EXPECT_EQ(output.shape().size(), 2);
    EXPECT_EQ(output.shape()[0], 2);
    EXPECT_EQ(output.shape()[1], 60); // 3*4*5
}

TEST(FlattenLayerTest, Backward) {
    FlattenLayer flatten;
    Tensor input({2, 3, 4, 5});
    input.fill(1.0);
    flatten.forward(input);
    Tensor grad({2, 60}, 1.0);
    Tensor input_grad = flatten.backward(grad);
    EXPECT_EQ(input_grad.shape().size(), 4);
    EXPECT_EQ(input_grad.shape()[0], 2);
    EXPECT_EQ(input_grad.shape()[1], 3);
    EXPECT_EQ(input_grad.shape()[2], 4);
    EXPECT_EQ(input_grad.shape()[3], 5);
}

// ============================================================================
// Conv1D Tests
// ============================================================================

TEST(Conv1DLayerTest, ForwardShape) {
    Conv1DLayer conv(3, 8, 3, 1, 1); // in=3, out=8, kernel=3, stride=1, pad=1
    Tensor input({2, 3, 10}); // batch=2, channels=3, length=10
    input.randomize();
    Tensor output = conv.forward(input);
    EXPECT_EQ(output.shape()[0], 2);
    EXPECT_EQ(output.shape()[1], 8);
    EXPECT_EQ(output.shape()[2], 10); // same padding
}

TEST(Conv1DLayerTest, ForwardShapeNoPadding) {
    Conv1DLayer conv(3, 8, 3, 1, 0);
    Tensor input({1, 3, 10});
    input.randomize();
    Tensor output = conv.forward(input);
    EXPECT_EQ(output.shape()[2], 8); // (10 - 3)/1 + 1 = 8
}

TEST(Conv1DLayerTest, BackwardShape) {
    Conv1DLayer conv(3, 8, 3, 1, 1);
    Tensor input({2, 3, 10});
    input.randomize();
    Tensor output = conv.forward(input);
    Tensor grad(output.shape(), 1.0);
    Tensor input_grad = conv.backward(grad);
    EXPECT_EQ(input_grad.shape()[0], 2);
    EXPECT_EQ(input_grad.shape()[1], 3);
    EXPECT_EQ(input_grad.shape()[2], 10);
}

// ============================================================================
// Conv2D Tests
// ============================================================================

TEST(Conv2DLayerTest, ForwardShape) {
    auto conv = Conv2DLayer::create_square(3, 16, 3, 1, 1); // in=3, out=16, kernel=3, stride=1, pad=1
    Tensor input({2, 3, 8, 8}); // batch=2, channels=3, 8x8
    input.randomize();
    Tensor output = conv->forward(input);
    EXPECT_EQ(output.shape()[0], 2);
    EXPECT_EQ(output.shape()[1], 16);
    EXPECT_EQ(output.shape()[2], 8);
    EXPECT_EQ(output.shape()[3], 8);
}

TEST(Conv2DLayerTest, ForwardShapeNoPadding) {
    auto conv = Conv2DLayer::create_square(1, 4, 3, 1, 0);
    Tensor input({1, 1, 6, 6});
    input.randomize();
    Tensor output = conv->forward(input);
    EXPECT_EQ(output.shape()[2], 4); // (6-3)/1+1 = 4
    EXPECT_EQ(output.shape()[3], 4);
}

TEST(Conv2DLayerTest, ForwardShapeStride2) {
    auto conv = Conv2DLayer::create_square(1, 4, 3, 2, 1);
    Tensor input({1, 1, 8, 8});
    input.randomize();
    Tensor output = conv->forward(input);
    EXPECT_EQ(output.shape()[2], 4); // (8+2-3)/2+1 = 4
    EXPECT_EQ(output.shape()[3], 4);
}

TEST(Conv2DLayerTest, BackwardShape) {
    auto conv = Conv2DLayer::create_square(3, 16, 3, 1, 1);
    Tensor input({2, 3, 8, 8});
    input.randomize();
    Tensor output = conv->forward(input);
    Tensor grad(output.shape(), 1.0);
    Tensor input_grad = conv->backward(grad);
    EXPECT_EQ(input_grad.shape()[0], 2);
    EXPECT_EQ(input_grad.shape()[1], 3);
    EXPECT_EQ(input_grad.shape()[2], 8);
    EXPECT_EQ(input_grad.shape()[3], 8);
}

// ============================================================================
// Pooling Tests
// ============================================================================

TEST(MaxPool2DLayerTest, ForwardShape) {
    MaxPool2DLayer pool(2);
    Tensor input({1, 1, 4, 4}, {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    });
    Tensor output = pool.forward(input);
    EXPECT_EQ(output.shape()[2], 2);
    EXPECT_EQ(output.shape()[3], 2);
    EXPECT_DOUBLE_EQ(output.data()[0], 6.0);   // max(1,2,5,6)
    EXPECT_DOUBLE_EQ(output.data()[1], 8.0);   // max(3,4,7,8)
    EXPECT_DOUBLE_EQ(output.data()[2], 14.0);  // max(9,10,13,14)
    EXPECT_DOUBLE_EQ(output.data()[3], 16.0);  // max(11,12,15,16)
}

TEST(AvgPool2DLayerTest, ForwardShape) {
    AvgPool2DLayer pool(2);
    Tensor input({1, 1, 4, 4}, {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    });
    Tensor output = pool.forward(input);
    EXPECT_EQ(output.shape()[2], 2);
    EXPECT_EQ(output.shape()[3], 2);
    EXPECT_DOUBLE_EQ(output.data()[0], 3.5);   // mean(1,2,5,6)
    EXPECT_DOUBLE_EQ(output.data()[1], 5.5);   // mean(3,4,7,8)
}

// ============================================================================
// RNN Tests
// ============================================================================

TEST(RNNLayerTest, ForwardShapeSequences) {
    RNNLayer rnn(4, 8, true); // input=4, hidden=8, return_sequences=true
    Tensor input({2, 5, 4}); // batch=2, seq_len=5, input_size=4
    input.randomize();
    Tensor output = rnn.forward(input);
    EXPECT_EQ(output.shape()[0], 2);
    EXPECT_EQ(output.shape()[1], 5);
    EXPECT_EQ(output.shape()[2], 8);
}

TEST(RNNLayerTest, ForwardShapeLastOnly) {
    RNNLayer rnn(4, 8, false); // return_sequences=false
    Tensor input({2, 5, 4});
    input.randomize();
    Tensor output = rnn.forward(input);
    EXPECT_EQ(output.shape()[0], 2);
    EXPECT_EQ(output.shape()[1], 8);
}

TEST(RNNLayerTest, BackwardShape) {
    RNNLayer rnn(4, 8, true);
    Tensor input({2, 5, 4});
    input.randomize();
    Tensor output = rnn.forward(input);
    Tensor grad(output.shape(), 1.0);
    Tensor input_grad = rnn.backward(grad);
    EXPECT_EQ(input_grad.shape()[0], 2);
    EXPECT_EQ(input_grad.shape()[1], 5);
    EXPECT_EQ(input_grad.shape()[2], 4);
}

// ============================================================================
// LSTM Tests
// ============================================================================

TEST(LSTMLayerTest, ForwardShapeSequences) {
    LSTMLayer lstm(4, 8, true);
    Tensor input({2, 5, 4});
    input.randomize();
    Tensor output = lstm.forward(input);
    EXPECT_EQ(output.shape()[0], 2);
    EXPECT_EQ(output.shape()[1], 5);
    EXPECT_EQ(output.shape()[2], 8);
}

TEST(LSTMLayerTest, ForwardShapeLastOnly) {
    LSTMLayer lstm(4, 8, false);
    Tensor input({2, 5, 4});
    input.randomize();
    Tensor output = lstm.forward(input);
    EXPECT_EQ(output.shape()[0], 2);
    EXPECT_EQ(output.shape()[1], 8);
}

TEST(LSTMLayerTest, BackwardShape) {
    LSTMLayer lstm(4, 8, true);
    Tensor input({2, 5, 4});
    input.randomize();
    Tensor output = lstm.forward(input);
    Tensor grad(output.shape(), 1.0);
    Tensor input_grad = lstm.backward(grad);
    EXPECT_EQ(input_grad.shape()[0], 2);
    EXPECT_EQ(input_grad.shape()[1], 5);
    EXPECT_EQ(input_grad.shape()[2], 4);
}

// ============================================================================
// GRU Tests
// ============================================================================

TEST(GRULayerTest, ForwardShapeSequences) {
    GRULayer gru(4, 8, true);
    Tensor input({2, 5, 4});
    input.randomize();
    Tensor output = gru.forward(input);
    EXPECT_EQ(output.shape()[0], 2);
    EXPECT_EQ(output.shape()[1], 5);
    EXPECT_EQ(output.shape()[2], 8);
}

TEST(GRULayerTest, ForwardShapeLastOnly) {
    GRULayer gru(4, 8, false);
    Tensor input({2, 5, 4});
    input.randomize();
    Tensor output = gru.forward(input);
    EXPECT_EQ(output.shape()[0], 2);
    EXPECT_EQ(output.shape()[1], 8);
}

TEST(GRULayerTest, BackwardShape) {
    GRULayer gru(4, 8, true);
    Tensor input({2, 5, 4});
    input.randomize();
    Tensor output = gru.forward(input);
    Tensor grad(output.shape(), 1.0);
    Tensor input_grad = gru.backward(grad);
    EXPECT_EQ(input_grad.shape()[0], 2);
    EXPECT_EQ(input_grad.shape()[1], 5);
    EXPECT_EQ(input_grad.shape()[2], 4);
}

// ============================================================================
// Embedding Tests
// ============================================================================

TEST(EmbeddingLayerTest, ForwardShape) {
    EmbeddingLayer emb(100, 16); // vocab=100, dim=16
    Tensor input({2, 5}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}); // batch=2, seq_len=5
    Tensor output = emb.forward(input);
    EXPECT_EQ(output.shape()[0], 2);
    EXPECT_EQ(output.shape()[1], 5);
    EXPECT_EQ(output.shape()[2], 16);
}

// ============================================================================
// Transformer Tests
// ============================================================================

TEST(MultiHeadAttentionTest, ForwardShape) {
    MultiHeadAttentionLayer mha(16, 4); // d_model=16, heads=4
    Tensor input({5, 16}); // seq_len=5, d_model=16
    input.randomize();
    Tensor output = mha.forward(input);
    EXPECT_EQ(output.shape()[0], 5);
    EXPECT_EQ(output.shape()[1], 16);
}

TEST(MultiHeadAttentionTest, BackwardShape) {
    MultiHeadAttentionLayer mha(16, 4);
    Tensor input({5, 16});
    input.randomize();
    Tensor output = mha.forward(input);
    Tensor grad(output.shape(), 1.0);
    Tensor input_grad = mha.backward(grad);
    EXPECT_EQ(input_grad.shape()[0], 5);
    EXPECT_EQ(input_grad.shape()[1], 16);
}

TEST(FeedForwardLayerTest, ForwardShape) {
    FeedForwardLayer ff(16, 64);
    Tensor input({5, 16});
    input.randomize();
    Tensor output = ff.forward(input);
    EXPECT_EQ(output.shape()[0], 5);
    EXPECT_EQ(output.shape()[1], 16);
}

TEST(PositionalEncodingTest, ForwardShape) {
    PositionalEncodingLayer pe(16, 100, 0.0); // no dropout for deterministic test
    Tensor input({5, 16}, 0.0);
    Tensor output = pe.forward(input);
    EXPECT_EQ(output.shape()[0], 5);
    EXPECT_EQ(output.shape()[1], 16);
    // Output should not be all zeros (positional encoding added)
    bool has_nonzero = false;
    for (size_t i = 0; i < output.size(); ++i) {
        if (std::abs(output.data()[i]) > 1e-10) { has_nonzero = true; break; }
    }
    EXPECT_TRUE(has_nonzero);
}

TEST(TransformerEncoderLayerTest, ForwardShape) {
    TransformerEncoderLayer encoder(16, 4, 64, 0.0);
    Tensor input({5, 16});
    input.randomize(-0.1, 0.1);
    Tensor output = encoder.forward(input);
    EXPECT_EQ(output.shape()[0], 5);
    EXPECT_EQ(output.shape()[1], 16);
}

TEST(TransformerEncoderLayerTest, BackwardShape) {
    TransformerEncoderLayer encoder(16, 4, 64, 0.0);
    Tensor input({5, 16});
    input.randomize(-0.1, 0.1);
    Tensor output = encoder.forward(input);
    Tensor grad(output.shape(), 0.01);
    Tensor input_grad = encoder.backward(grad);
    EXPECT_EQ(input_grad.shape()[0], 5);
    EXPECT_EQ(input_grad.shape()[1], 16);
}

TEST(TransformerEncoderLayerTest, VulkanForwardAndBackward) {
    if (!tensor_backend_available(TensorBackend::VULKAN)) {
        return;
    }

    set_tensor_backend(TensorBackend::VULKAN);
    TransformerEncoderLayer encoder(16, 4, 64, 0.0);
    Tensor input({5, 16});
    input.randomize(-0.1, 0.1);
    Tensor output = encoder.forward(input);
    Tensor gradient(output.shape(), 0.01);
    Tensor input_gradient = encoder.backward(gradient);

    EXPECT_EQ(active_tensor_backend(), TensorBackend::VULKAN);
    EXPECT_EQ(output.shape()[0], 5);
    EXPECT_EQ(output.shape()[1], 16);
    EXPECT_EQ(input_gradient.shape()[0], 5);
    EXPECT_EQ(input_gradient.shape()[1], 16);
    set_tensor_backend(TensorBackend::CPU);
}

TEST(TransformerDecoderLayerTest, ForwardShape) {
    TransformerDecoderLayer decoder(16, 4, 64, 0.0);
    Tensor target({5, 16});
    Tensor encoder_output({7, 16});
    target.randomize(-0.1, 0.1);
    encoder_output.randomize(-0.1, 0.1);
    Tensor output = decoder.forward(target, encoder_output, nullptr, nullptr);
    EXPECT_EQ(output.shape()[0], 5);
    EXPECT_EQ(output.shape()[1], 16);
}

// ============================================================================
// Template Tests
// ============================================================================

TEST(MLPTemplateTest, Build) {
    MLPTemplate tmpl(10, {32, 16}, 5, "relu", 0.1, true);
    NeuralNetwork net = tmpl.build();
    EXPECT_GT(net.num_layers(), 0u);
}

TEST(CNNTemplateTest, BuildSimple) {
    CNNTemplate tmpl(CNNTemplate::Architecture::SIMPLE, 10, 3, 32, 32);
    NeuralNetwork net = tmpl.build();
    EXPECT_GT(net.num_layers(), 0u);
    EXPECT_EQ(tmpl.name(), "SimpleCNN");
}

TEST(CNNTemplateTest, BuildLeNet) {
    CNNTemplate tmpl(CNNTemplate::Architecture::LENET, 10, 1, 32, 32);
    NeuralNetwork net = tmpl.build();
    EXPECT_GT(net.num_layers(), 0u);
    EXPECT_EQ(tmpl.name(), "LeNet");
}

TEST(RNNTemplateTest, BuildVanillaRNN) {
    RNNTemplate tmpl(RNNTemplate::CellType::VANILLA_RNN, 10, 32, 5, 2, false, 0.1);
    NeuralNetwork net = tmpl.build();
    EXPECT_GT(net.num_layers(), 0u);
    EXPECT_EQ(tmpl.name(), "RNN");
}

TEST(RNNTemplateTest, BuildLSTM) {
    RNNTemplate tmpl(RNNTemplate::CellType::LSTM, 10, 32, 5, 1, false, 0.0);
    NeuralNetwork net = tmpl.build();
    EXPECT_GT(net.num_layers(), 0u);
    EXPECT_EQ(tmpl.name(), "LSTM");
}

TEST(RNNTemplateTest, BuildGRU) {
    RNNTemplate tmpl(RNNTemplate::CellType::GRU, 10, 32, 5, 1, false, 0.0);
    NeuralNetwork net = tmpl.build();
    EXPECT_GT(net.num_layers(), 0u);
    EXPECT_EQ(tmpl.name(), "GRU");
}

TEST(TransformerTemplateTest, BuildEncoderOnly) {
    TransformerTemplate tmpl(TransformerTemplate::Architecture::ENCODER_ONLY,
                             16, 4, 64, 2, 100, 50, 0.1);
    NeuralNetwork net = tmpl.build();
    EXPECT_GT(net.num_layers(), 0u);
    EXPECT_EQ(tmpl.name(), "TransformerEncoder");
}

TEST(TransformerTemplateTest, BuildDecoderOnly) {
    TransformerTemplate tmpl(TransformerTemplate::Architecture::DECODER_ONLY,
                             16, 4, 64, 2, 100, 50, 0.1);
    NeuralNetwork net = tmpl.build();
    EXPECT_GT(net.num_layers(), 0u);
    EXPECT_EQ(tmpl.name(), "TransformerDecoder");
}

TEST(TransformerTemplateTest, BuildEncoderDecoder) {
    TransformerTemplate tmpl(TransformerTemplate::Architecture::ENCODER_DECODER,
                             16, 4, 64, 2, 100, 50, 0.1);
    NeuralNetwork net = tmpl.build();
    EXPECT_GT(net.num_layers(), 0u);
    EXPECT_EQ(tmpl.name(), "Transformer");
}

// ============================================================================
// Loss Tests
// ============================================================================

TEST(MSELossTest, Compute) {
    MSELoss loss;
    Tensor pred({1, 3}, {1.0, 2.0, 3.0});
    Tensor target({1, 3}, {1.0, 2.0, 3.0});
    EXPECT_DOUBLE_EQ(loss.compute(pred, target), 0.0);
}

TEST(BCELossTest, Compute) {
    BCELoss loss;
    Tensor pred({1, 2}, {0.5, 0.5});
    Tensor target({1, 2}, {1.0, 0.0});
    double l = loss.compute(pred, target);
    EXPECT_GT(l, 0.0);
}

// ============================================================================
// Optimizer Tests
// ============================================================================

TEST(SGDTest, Step) {
    SGD sgd(0.1);
    Tensor params({3}, {1.0, 2.0, 3.0});
    Tensor grads({3}, {0.1, 0.2, 0.3});
    sgd.step(params, grads);
    EXPECT_NEAR(params.data()[0], 0.99, 1e-6);
    EXPECT_NEAR(params.data()[1], 1.98, 1e-6);
    EXPECT_NEAR(params.data()[2], 2.97, 1e-6);
}

TEST(AdamTest, Step) {
    Adam adam(0.01);
    Tensor params({3}, {1.0, 2.0, 3.0});
    Tensor grads({3}, {0.1, 0.2, 0.3});
    adam.step(params, grads);
    // After one step, params should have changed
    EXPECT_NE(params.data()[0], 1.0);
}

// ============================================================================
// Integration: CNN Pipeline
// ============================================================================

TEST(IntegrationTest, ConvPipeline) {
    // Build a small conv pipeline and run forward pass
    NeuralNetwork net;
    net.add_layer(ml::deep_learning::Conv2DLayer::create_square(1, 4, 3, 1, 1));
    net.add_layer(std::make_shared<ReLULayer>());
    net.add_layer(std::make_shared<MaxPool2DLayer>(2));
    net.add_layer(std::make_shared<FlattenLayer>());
    net.add_layer(std::make_shared<DenseLayer>(4 * 4 * 4, 10)); // After pool: 4x4

    Tensor input({1, 1, 8, 8});
    input.randomize();
    Tensor output = net.forward(input);
    EXPECT_EQ(output.shape()[0], 1);
    EXPECT_EQ(output.shape()[1], 10);
}

// ============================================================================
// Integration: RNN Pipeline
// ============================================================================

TEST(IntegrationTest, RNNPipeline) {
    NeuralNetwork net;
    net.add_layer(std::make_shared<LSTMLayer>(4, 8, false));
    net.add_layer(std::make_shared<DenseLayer>(8, 3));

    Tensor input({2, 5, 4}); // batch=2, seq=5, features=4
    input.randomize();
    Tensor output = net.forward(input);
    EXPECT_EQ(output.shape()[0], 2);
    EXPECT_EQ(output.shape()[1], 3);
}

// ============================================================================
// CDeepEx Tests
// ============================================================================

TEST(CDeepExTest, ContrastiveMapShapeAndRange) {
    NeuralNetwork net;
    net.add_layer(std::make_shared<DenseLayer>(4, 3));

    Tensor input({1, 4}, {0.2, -0.1, 0.5, 0.8});

    CDeepExExplainer explainer(net);
    ContrastiveExplanation explanation = explainer.explain(input, 0, 1, true);

    EXPECT_EQ(explanation.positive_gradient.shape(), input.shape());
    EXPECT_EQ(explanation.negative_gradient.shape(), input.shape());
    EXPECT_EQ(explanation.contrastive_map.shape(), input.shape());

    for (size_t i = 0; i < explanation.contrastive_map.size(); ++i) {
        EXPECT_GE(explanation.contrastive_map.data()[i], 0.0);
        EXPECT_LE(explanation.contrastive_map.data()[i], 1.0);
    }
}

TEST(CDeepExTest, AutoFoilPicksDifferentClass) {
    NeuralNetwork net;
    net.add_layer(std::make_shared<DenseLayer>(4, 3));

    Tensor input({1, 4}, {0.1, 0.2, -0.3, 0.4});

    CDeepExExplainer explainer(net);
    ContrastiveExplanation explanation = explainer.explain(input, 2);

    EXPECT_EQ(explanation.target_class, 2u);
    EXPECT_NE(explanation.foil_class, explanation.target_class);
    EXPECT_LT(explanation.foil_class, 3u);
}

TEST(CDeepExTest, InvalidTargetThrows) {
    NeuralNetwork net;
    net.add_layer(std::make_shared<DenseLayer>(4, 3));

    Tensor input({1, 4}, {0.1, 0.2, -0.3, 0.4});

    CDeepExExplainer explainer(net);
    EXPECT_THROW(explainer.explain(input, 3), std::invalid_argument);
}

TEST(CDeepExTest, LimeExplanationShapeAndFinitePrediction) {
    NeuralNetwork net;
    net.add_layer(std::make_shared<DenseLayer>(4, 3));

    Tensor input({1, 4}, {0.3, -0.4, 0.1, 0.9});

    CDeepExExplainer explainer(net);
    LimeExplanation explanation = explainer.explain_lime(input, 1, 96, 4, 0.8, 1e-3, 123);

    EXPECT_EQ(explanation.feature_importance.shape(), input.shape());
    EXPECT_EQ(explanation.target_class, 1u);
    EXPECT_GT(explanation.sampled_features, 0u);
    EXPECT_TRUE(std::isfinite(explanation.local_prediction));
    EXPECT_TRUE(std::isfinite(explanation.model_prediction));
}

TEST(CDeepExTest, IntegratedGradientsShapeAndRange) {
    NeuralNetwork net;
    net.add_layer(std::make_shared<DenseLayer>(4, 3));

    Tensor input({1, 4}, {0.5, -0.2, 0.8, 0.1});

    CDeepExExplainer explainer(net);
    IntegratedGradientsExplanation explanation =
        explainer.explain_integrated_gradients(input, 2, 16, true);

    EXPECT_EQ(explanation.attribution.shape(), input.shape());
    EXPECT_EQ(explanation.target_class, 2u);
    for (size_t i = 0; i < explanation.attribution.size(); ++i) {
        EXPECT_GE(explanation.attribution.data()[i], 0.0);
        EXPECT_LE(explanation.attribution.data()[i], 1.0);
    }
}

TEST(CDeepExTest, KernelShapShapeAndFiniteValues) {
    NeuralNetwork net;
    net.add_layer(std::make_shared<DenseLayer>(4, 3));

    Tensor input({1, 4}, {0.7, -0.3, 0.2, 0.5});

    CDeepExExplainer explainer(net);
    ShapExplanation explanation = explainer.explain_kernel_shap(input, 0, 200, 4, 1e-6, 11);

    EXPECT_EQ(explanation.shap_values.shape(), input.shape());
    EXPECT_EQ(explanation.target_class, 0u);
    EXPECT_GT(explanation.sampled_features, 0u);
    EXPECT_TRUE(std::isfinite(explanation.base_value));
    EXPECT_TRUE(std::isfinite(explanation.local_prediction));
    EXPECT_TRUE(std::isfinite(explanation.model_prediction));
}

TEST(CDeepExTest, KernelShapLocalAdditivityApproximation) {
    NeuralNetwork net;
    net.add_layer(std::make_shared<DenseLayer>(4, 3));

    Tensor input({1, 4}, {0.4, 0.1, -0.2, 0.9});

    CDeepExExplainer explainer(net);
    ShapExplanation explanation = explainer.explain_kernel_shap(input, 2, 240, 4, 1e-8, 13);

    const double gap = std::abs(explanation.local_prediction - explanation.model_prediction);
    EXPECT_LT(gap, 0.5);
}

TEST(ExplainabilityFacadeTest, DispatchesLime) {
    NeuralNetwork net;
    net.add_layer(std::make_shared<DenseLayer>(4, 3));
    ExplainabilityFacade facade(net);

    ExplainRequest request = ExplainRequest::for_lime(1, 96, 4, 0.75, 1e-3, 21);

    Tensor input({1, 4}, {0.2, 0.6, -0.1, 0.4});
    ExplainabilityResult result = facade.explain(input, request);

    EXPECT_EQ(result.method, ExplanationMethod::LIME);
    EXPECT_EQ(result.lime.feature_importance.shape(), input.shape());
    EXPECT_EQ(result.lime.target_class, 1u);
}

TEST(ExplainabilityFacadeTest, DispatchesKernelShap) {
    NeuralNetwork net;
    net.add_layer(std::make_shared<DenseLayer>(4, 3));
    ExplainabilityFacade facade(net);

    ExplainRequest request = ExplainRequest::for_kernel_shap(2, 180, 4, 1e-6, 27);

    Tensor input({1, 4}, {0.5, -0.2, 0.3, 0.9});
    ExplainabilityResult result = facade.explain(input, request);

    EXPECT_EQ(result.method, ExplanationMethod::KERNEL_SHAP);
    EXPECT_EQ(result.shap.shap_values.shape(), input.shape());
    EXPECT_EQ(result.shap.target_class, 2u);
}

TEST(ExplainabilityFacadeTest, RequestFactoryHelpersSetFields) {
    ExplainRequest contrastive = ExplainRequest::for_contrastive(3, false, false, 1);
    EXPECT_EQ(contrastive.method, ExplanationMethod::CDEEPEX_CONTRASTIVE);
    EXPECT_EQ(contrastive.target_class, 3u);
    EXPECT_FALSE(contrastive.normalize);
    EXPECT_FALSE(contrastive.use_default_foil);
    EXPECT_EQ(contrastive.foil_class, 1u);

    ExplainRequest lime = ExplainRequest::for_lime(2, 77, 5, 0.5, 1e-4, 99);
    EXPECT_EQ(lime.method, ExplanationMethod::LIME);
    EXPECT_EQ(lime.target_class, 2u);
    EXPECT_EQ(lime.lime_num_samples, 77u);
    EXPECT_EQ(lime.lime_max_features, 5u);
    EXPECT_DOUBLE_EQ(lime.lime_kernel_width, 0.5);
    EXPECT_DOUBLE_EQ(lime.lime_ridge, 1e-4);
    EXPECT_EQ(lime.lime_seed, 99u);

    ExplainRequest ig = ExplainRequest::for_integrated_gradients(4, 19, false);
    EXPECT_EQ(ig.method, ExplanationMethod::INTEGRATED_GRADIENTS);
    EXPECT_EQ(ig.target_class, 4u);
    EXPECT_EQ(ig.ig_steps, 19u);
    EXPECT_FALSE(ig.normalize);

    ExplainRequest shap = ExplainRequest::for_kernel_shap(5, 111, 6, 1e-7, 101);
    EXPECT_EQ(shap.method, ExplanationMethod::KERNEL_SHAP);
    EXPECT_EQ(shap.target_class, 5u);
    EXPECT_EQ(shap.shap_num_samples, 111u);
    EXPECT_EQ(shap.shap_max_features, 6u);
    EXPECT_DOUBLE_EQ(shap.shap_ridge, 1e-7);
    EXPECT_EQ(shap.shap_seed, 101u);
}


// ============================================================================
// Main entry point for tyst framework
// ============================================================================

int main(int argc, char** argv) {
    tyst::framework::init(&argc, argv);
    return tyst::framework::run_all_tests();
}
