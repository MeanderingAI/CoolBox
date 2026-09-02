#include "templates.h"

namespace ml {
namespace deep_learning {

// ============================================================================
// MLPTemplate
// ============================================================================

MLPTemplate::MLPTemplate(int input_dim, const std::vector<int>& hidden_dims, int output_dim,
                         const std::string& activation, double dropout_rate, bool batch_norm)
    : input_dim_(input_dim), hidden_dims_(hidden_dims), output_dim_(output_dim),
      activation_(activation), dropout_rate_(dropout_rate), batch_norm_(batch_norm) {}

NeuralNetwork MLPTemplate::build() {
    NeuralNetwork net;
    int prev_dim = input_dim_;
    for (int hidden_dim : hidden_dims_) {
        net.add_layer(std::make_shared<DenseLayer>(prev_dim, hidden_dim));
        if (batch_norm_) {
            net.add_layer(std::make_shared<BatchNormLayer>(hidden_dim));
        }
        if (activation_ == "relu") {
            net.add_layer(std::make_shared<ReLULayer>());
        } else if (activation_ == "sigmoid") {
            net.add_layer(std::make_shared<SigmoidLayer>());
        } else if (activation_ == "tanh") {
            net.add_layer(std::make_shared<TanhLayer>());
        }
        if (dropout_rate_ > 0.0) {
            net.add_layer(std::make_shared<DropoutLayer>(dropout_rate_));
        }
        prev_dim = hidden_dim;
    }
    net.add_layer(std::make_shared<DenseLayer>(prev_dim, output_dim_));
    return net;
}

// ============================================================================
// CNNTemplate
// ============================================================================

CNNTemplate::CNNTemplate(Architecture architecture, int num_classes,
                         int input_channels, int input_height, int input_width)
    : architecture_(architecture), num_classes_(num_classes),
      input_channels_(input_channels), input_height_(input_height), input_width_(input_width) {}

NeuralNetwork CNNTemplate::build() {
    NeuralNetwork net;
    
    switch (architecture_) {
        case Architecture::SIMPLE: {
            // Simple: Conv -> ReLU -> Pool -> Conv -> ReLU -> Pool -> Flatten -> Dense
            net.add_layer(ml::deep_learning::Conv2DLayer::create_square(input_channels_, 16, 3, 1, 1));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<MaxPool2DLayer>(2));
            net.add_layer(ml::deep_learning::Conv2DLayer::create_square(16, 32, 3, 1, 1));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<MaxPool2DLayer>(2));
            net.add_layer(std::make_shared<FlattenLayer>());
            // After two 2x2 pools: h/4 x w/4
            int flat_size = 32 * (input_height_ / 4) * (input_width_ / 4);
            net.add_layer(std::make_shared<DenseLayer>(flat_size, 128));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<DenseLayer>(128, num_classes_));
            break;
        }
        case Architecture::LENET: {
            // LeNet-5 style
            net.add_layer(ml::deep_learning::Conv2DLayer::create_square(input_channels_, 6, 5, 1, 0));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<AvgPool2DLayer>(2));
            net.add_layer(ml::deep_learning::Conv2DLayer::create_square(6, 16, 5, 1, 0));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<AvgPool2DLayer>(2));
            net.add_layer(std::make_shared<FlattenLayer>());
            // Compute flattened size: after conv1(5x5): (h-4)x(w-4), pool: /2, conv2(5x5): -4, pool: /2
            int h1 = (input_height_ - 4) / 2;
            int w1 = (input_width_ - 4) / 2;
            int h2 = (h1 - 4) / 2;
            int w2 = (w1 - 4) / 2;
            int flat_size = 16 * h2 * w2;
            net.add_layer(std::make_shared<DenseLayer>(flat_size, 120));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<DenseLayer>(120, 84));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<DenseLayer>(84, num_classes_));
            break;
        }
        case Architecture::VGGLIKE: {
            // VGG-style: blocks of 2x conv + pool
            int channels = 64;
            int prev_channels = input_channels_;
            int h = input_height_, w = input_width_;
            
            // Block 1
            net.add_layer(ml::deep_learning::Conv2DLayer::create_square(prev_channels, channels, 3, 1, 1));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(ml::deep_learning::Conv2DLayer::create_square(channels, channels, 3, 1, 1));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<MaxPool2DLayer>(2));
            h /= 2; w /= 2;
            
            // Block 2
            prev_channels = channels;
            channels = 128;
            net.add_layer(ml::deep_learning::Conv2DLayer::create_square(prev_channels, channels, 3, 1, 1));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(ml::deep_learning::Conv2DLayer::create_square(channels, channels, 3, 1, 1));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<MaxPool2DLayer>(2));
            h /= 2; w /= 2;
            
            net.add_layer(std::make_shared<FlattenLayer>());
            int flat_size = channels * h * w;
            net.add_layer(std::make_shared<DenseLayer>(flat_size, 256));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<DropoutLayer>(0.5));
            net.add_layer(std::make_shared<DenseLayer>(256, num_classes_));
            break;
        }
        case Architecture::RESNET:
        default: {
            // ResNet-style fallback (simplified - no actual skip connections in sequential model)
            net.add_layer(ml::deep_learning::Conv2DLayer::create_square(input_channels_, 64, 3, 1, 1));
            net.add_layer(std::make_shared<BatchNormLayer>(64));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(ml::deep_learning::Conv2DLayer::create_square(64, 64, 3, 1, 1));
            net.add_layer(std::make_shared<BatchNormLayer>(64));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<MaxPool2DLayer>(2));
            net.add_layer(ml::deep_learning::Conv2DLayer::create_square(64, 128, 3, 1, 1));
            net.add_layer(std::make_shared<BatchNormLayer>(128));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<MaxPool2DLayer>(2));
            net.add_layer(std::make_shared<FlattenLayer>());
            int flat_size = 128 * (input_height_ / 4) * (input_width_ / 4);
            net.add_layer(std::make_shared<DenseLayer>(flat_size, 256));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<DenseLayer>(256, num_classes_));
            break;
        }
    }
    
    return net;
}

std::string CNNTemplate::name() const {
    switch (architecture_) {
        case Architecture::SIMPLE: return "SimpleCNN";
        case Architecture::LENET: return "LeNet";
        case Architecture::VGGLIKE: return "VGGLike";
        case Architecture::RESNET: return "ResNet";
        default: return "CNN";
    }
}

// ============================================================================
// RNNTemplate
// ============================================================================

RNNTemplate::RNNTemplate(CellType cell_type, int input_size, int hidden_size, int output_size,
                         int num_layers, bool bidirectional, double dropout_rate)
    : cell_type_(cell_type), input_size_(input_size), hidden_size_(hidden_size),
      output_size_(output_size), num_layers_(num_layers), bidirectional_(bidirectional),
      dropout_rate_(dropout_rate) {}

NeuralNetwork RNNTemplate::build() {
    NeuralNetwork net;
    
    int prev_size = input_size_;
    
    for (int i = 0; i < num_layers_; ++i) {
        bool return_sequences = (i < num_layers_ - 1); // Last layer returns final hidden state
        
        switch (cell_type_) {
            case CellType::VANILLA_RNN:
                net.add_layer(std::make_shared<RNNLayer>(prev_size, hidden_size_, return_sequences));
                break;
            case CellType::LSTM:
                net.add_layer(std::make_shared<LSTMLayer>(prev_size, hidden_size_, return_sequences));
                break;
            case CellType::GRU:
                net.add_layer(std::make_shared<GRULayer>(prev_size, hidden_size_, return_sequences));
                break;
        }
        
        if (dropout_rate_ > 0.0 && i < num_layers_ - 1) {
            net.add_layer(std::make_shared<DropoutLayer>(dropout_rate_));
        }
        
        prev_size = hidden_size_;
    }
    
    // Output projection
    net.add_layer(std::make_shared<DenseLayer>(hidden_size_, output_size_));
    
    return net;
}

std::string RNNTemplate::name() const {
    switch (cell_type_) {
        case CellType::VANILLA_RNN: return "RNN";
        case CellType::LSTM: return "LSTM";
        case CellType::GRU: return "GRU";
        default: return "RNN";
    }
}

// ============================================================================
// TransformerTemplate
// ============================================================================

TransformerTemplate::TransformerTemplate(Architecture architecture, int d_model, int num_heads,
                                         int d_ff, int num_layers, int vocab_size, int max_seq_len,
                                         double dropout_rate)
    : architecture_(architecture), d_model_(d_model), num_heads_(num_heads),
      d_ff_(d_ff), num_layers_(num_layers), vocab_size_(vocab_size),
      max_seq_len_(max_seq_len), dropout_rate_(dropout_rate) {}

NeuralNetwork TransformerTemplate::build() {
    NeuralNetwork net;
    
    // Embedding + positional encoding
    net.add_layer(std::make_shared<EmbeddingLayer>(vocab_size_, d_model_));
    net.add_layer(std::make_shared<PositionalEncodingLayer>(d_model_, max_seq_len_, dropout_rate_));
    
    switch (architecture_) {
        case Architecture::ENCODER_ONLY: {
            for (int i = 0; i < num_layers_; ++i) {
                net.add_layer(std::make_shared<TransformerEncoderLayer>(
                    d_model_, num_heads_, d_ff_, dropout_rate_));
            }
            // Classification head
            net.add_layer(std::make_shared<DenseLayer>(d_model_, vocab_size_));
            break;
        }
        case Architecture::DECODER_ONLY: {
            for (int i = 0; i < num_layers_; ++i) {
                // Use encoder layers for decoder-only (self-attention only)
                net.add_layer(std::make_shared<TransformerEncoderLayer>(
                    d_model_, num_heads_, d_ff_, dropout_rate_));
            }
            net.add_layer(std::make_shared<DenseLayer>(d_model_, vocab_size_));
            break;
        }
        case Architecture::ENCODER_DECODER: {
            // Encoder stack
            for (int i = 0; i < num_layers_; ++i) {
                net.add_layer(std::make_shared<TransformerEncoderLayer>(
                    d_model_, num_heads_, d_ff_, dropout_rate_));
            }
            // Decoder stack
            for (int i = 0; i < num_layers_; ++i) {
                net.add_layer(std::make_shared<TransformerDecoderLayer>(
                    d_model_, num_heads_, d_ff_, dropout_rate_));
            }
            net.add_layer(std::make_shared<DenseLayer>(d_model_, vocab_size_));
            break;
        }
    }
    
    return net;
}

std::string TransformerTemplate::name() const {
    switch (architecture_) {
        case Architecture::ENCODER_ONLY: return "TransformerEncoder";
        case Architecture::DECODER_ONLY: return "TransformerDecoder";
        case Architecture::ENCODER_DECODER: return "Transformer";
        default: return "Transformer";
    }
}

} // namespace deep_learning
} // namespace ml
