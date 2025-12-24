#include "ml/deep_learning/neural_network.h"
#include "ml/deep_learning/layer.h"
#include "ml/deep_learning/loss.h"
#include "ml/deep_learning/tensor.h"
#include <gtest/gtest.h>
#include <memory>

using namespace ml::deep_learning;

TEST(TensorTest, BasicOperations) {
    Tensor t1({2, 2}, {1.0, 2.0, 3.0, 4.0});
    Tensor t2({2, 2}, {5.0, 6.0, 7.0, 8.0});
    
    // Addition
    Tensor sum = t1 + t2;
    EXPECT_DOUBLE_EQ(sum.data()[0], 6.0);
    EXPECT_DOUBLE_EQ(sum.data()[3], 12.0);
    
    // Subtraction
    Tensor diff = t2 - t1;
    EXPECT_DOUBLE_EQ(diff.data()[0], 4.0);
    
    // Scalar multiplication
    Tensor scaled = t1 * 2.0;
    EXPECT_DOUBLE_EQ(scaled.data()[0], 2.0);
    EXPECT_DOUBLE_EQ(scaled.data()[1], 4.0);
}

TEST(TensorTest, MatrixMultiplication) {
    Tensor a({2, 3}, {1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
    Tensor b({3, 2}, {7.0, 8.0, 9.0, 10.0, 11.0, 12.0});
    
    Tensor c = a.matmul(b);
    
    EXPECT_EQ(c.shape()[0], 2);
    EXPECT_EQ(c.shape()[1], 2);
    
    // First element: 1*7 + 2*9 + 3*11 = 7 + 18 + 33 = 58
    EXPECT_DOUBLE_EQ(c.data()[0], 58.0);
}

TEST(DenseLayerTest, ForwardPass) {
    DenseLayer layer(2, 3);
    
    Tensor input({1, 2}, {1.0, 2.0});
    Tensor output = layer.forward(input);
    
    EXPECT_EQ(output.shape()[0], 1);
    EXPECT_EQ(output.shape()[1], 3);
}

TEST(ActivationLayerTest, ReLU) {
    ReLULayer relu;
    
    Tensor input({1, 4}, {-1.0, 0.0, 1.0, 2.0});
    Tensor output = relu.forward(input);
    
    EXPECT_DOUBLE_EQ(output.data()[0], 0.0);  // -1 -> 0
    EXPECT_DOUBLE_EQ(output.data()[1], 0.0);  // 0 -> 0
    EXPECT_DOUBLE_EQ(output.data()[2], 1.0);  // 1 -> 1
    EXPECT_DOUBLE_EQ(output.data()[3], 2.0);  // 2 -> 2
}

TEST(ActivationLayerTest, Sigmoid) {
    SigmoidLayer sigmoid;
    
    Tensor input({1, 1}, {0.0});
    Tensor output = sigmoid.forward(input);
    
    EXPECT_NEAR(output.data()[0], 0.5, 1e-6);
}

TEST(LossTest, MSELoss) {
    MSELoss loss;
    
    Tensor predictions({2, 1}, {1.0, 2.0});
    Tensor targets({2, 1}, {1.5, 2.5});
    
    double loss_value = loss.compute(predictions, targets);
    
    // MSE = ((0.5)^2 + (0.5)^2) / 2 = 0.25
    EXPECT_NEAR(loss_value, 0.25, 1e-6);
}

TEST(NeuralNetworkTest, XORProblem) {
    // Create XOR dataset
    std::vector<Tensor> inputs;
    std::vector<Tensor> targets;
    
    inputs.push_back(Tensor({1, 2}, {0.0, 0.0}));
    targets.push_back(Tensor({1, 1}, {0.0}));
    
    inputs.push_back(Tensor({1, 2}, {0.0, 1.0}));
    targets.push_back(Tensor({1, 1}, {1.0}));
    
    inputs.push_back(Tensor({1, 2}, {1.0, 0.0}));
    targets.push_back(Tensor({1, 1}, {1.0}));
    
    inputs.push_back(Tensor({1, 2}, {1.0, 1.0}));
    targets.push_back(Tensor({1, 1}, {0.0}));
    
    // Create neural network
    NeuralNetwork nn;
    nn.add_layer(std::make_shared<DenseLayer>(2, 4));
    nn.add_layer(std::make_shared<ReLULayer>());
    nn.add_layer(std::make_shared<DenseLayer>(4, 1));
    nn.add_layer(std::make_shared<SigmoidLayer>());
    nn.set_loss(std::make_shared<MSELoss>());
    
    // Train
    nn.train(inputs, targets, 500, 4, false);
    
    // Test - the network should learn XOR reasonably well
    double total_error = 0.0;
    for (size_t i = 0; i < inputs.size(); ++i) {
        Tensor output = nn.predict(inputs[i]);
        double error = std::abs(output.data()[0] - targets[i].data()[0]);
        total_error += error;
    }
    
    // Average error should be less than 0.3 after training
    EXPECT_LT(total_error / inputs.size(), 0.3);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
