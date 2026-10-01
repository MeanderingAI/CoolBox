#ifndef ML_DEEP_LEARNING_TEMPLATES_H
#define ML_DEEP_LEARNING_TEMPLATES_H

#include "neural_network.h"
#include <vector>
#include <string>
#include <memory>

namespace ml {
namespace deep_learning {

// Base class for all network templates
class NetworkTemplate {
public:
    virtual ~NetworkTemplate() = default;
    virtual NeuralNetwork build() = 0;
    virtual std::string name() const = 0;
};

// Multi-Layer Perceptron Template
class MLPTemplate : public NetworkTemplate {
public:
    MLPTemplate(int input_dim, const std::vector<int>& hidden_dims, int output_dim,
                const std::string& activation = "relu", double dropout_rate = 0.0,
                bool batch_norm = false);
    
    NeuralNetwork build() override;
    std::string name() const override { return "MLP"; }
    
private:
    int input_dim_;
    std::vector<int> hidden_dims_;
    int output_dim_;
    std::string activation_;
    double dropout_rate_;
    bool batch_norm_;
};

// Convolutional Neural Network Template
class CNNTemplate : public NetworkTemplate {
public:
    enum class Architecture {
        SIMPLE,    // Simple 2-3 conv layer network
        LENET,     // LeNet-5 style
        VGGLIKE,   // VGG-style with multiple conv blocks
        RESNET     // ResNet-style with residual connections
    };
    
    CNNTemplate(Architecture architecture, int num_classes,
                int input_channels = 3, int input_height = 32, int input_width = 32);
    
    NeuralNetwork build() override;
    std::string name() const override;
    
private:
    Architecture architecture_;
    int num_classes_;
    int input_channels_;
    int input_height_;
    int input_width_;
};

// Recurrent Neural Network Template
class RNNTemplate : public NetworkTemplate {
public:
    enum class CellType {
        VANILLA_RNN,
        LSTM,
        GRU
    };
    
    RNNTemplate(CellType cell_type, int input_size, int hidden_size, int output_size,
                int num_layers = 1, bool bidirectional = false, double dropout_rate = 0.0);
    
    NeuralNetwork build() override;
    std::string name() const override;
    
private:
    CellType cell_type_;
    int input_size_;
    int hidden_size_;
    int output_size_;
    int num_layers_;
    [[maybe_unused]] bool bidirectional_;
    double dropout_rate_;
};

// Transformer Template
class TransformerTemplate : public NetworkTemplate {
public:
    enum class Architecture {
        ENCODER_ONLY,   // BERT-style (classification, embeddings)
        DECODER_ONLY,   // GPT-style (autoregressive generation)
        ENCODER_DECODER  // T5/original Transformer (seq2seq)
    };
    
    TransformerTemplate(Architecture architecture, int d_model, int num_heads,
                        int d_ff, int num_layers, int vocab_size, int max_seq_len = 512,
                        double dropout_rate = 0.1);
    
    NeuralNetwork build() override;
    std::string name() const override;
    
private:
    Architecture architecture_;
    int d_model_;
    int num_heads_;
    int d_ff_;
    int num_layers_;
    int vocab_size_;
    int max_seq_len_;
    double dropout_rate_;
};

} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_TEMPLATES_H
