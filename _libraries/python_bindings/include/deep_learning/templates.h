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
    
    void build_simple(NeuralNetwork& net);
    void build_lenet(NeuralNetwork& net);
    void build_vgglike(NeuralNetwork& net);
    void build_resnet(NeuralNetwork& net);
};

// Autoencoder Template
class AutoencoderTemplate : public NetworkTemplate {
public:
    AutoencoderTemplate(int input_dim, const std::vector<int>& encoder_dims,
                       int latent_dim, bool variational = false);
    
    NeuralNetwork build() override;
    NeuralNetwork build_encoder();
    NeuralNetwork build_decoder();
    std::string name() const override { return variational_ ? "VAE" : "Autoencoder"; }
    
private:
    int input_dim_;
    std::vector<int> encoder_dims_;
    int latent_dim_;
    bool variational_;
};

// Recurrent Neural Network Template
class RNNTemplate : public NetworkTemplate {
public:
    enum class CellType {
        VANILLA,  // Simple RNN
        LSTM,     // Long Short-Term Memory
        GRU       // Gated Recurrent Unit
    };
    
    RNNTemplate(int input_dim, int hidden_dim, int num_layers, int output_dim,
                CellType cell_type = CellType::LSTM, bool bidirectional = false,
                double dropout = 0.0);
    
    NeuralNetwork build() override;
    std::string name() const override;
    
private:
    int input_dim_;
    int hidden_dim_;
    int num_layers_;
    int output_dim_;
    CellType cell_type_;
    bool bidirectional_;
    double dropout_;
};

// Siamese Network Template
class SiameseTemplate : public NetworkTemplate {
public:
    SiameseTemplate(int input_dim, const std::vector<int>& hidden_dims,
                   int embedding_dim, const std::string& distance_metric = "euclidean");
    
    NeuralNetwork build() override;
    NeuralNetwork build_embedding_network();
    std::string name() const override { return "Siamese"; }
    
private:
    int input_dim_;
    std::vector<int> hidden_dims_;
    int embedding_dim_;
    std::string distance_metric_;
};

// Generative Adversarial Network Template
class GANTemplate : public NetworkTemplate {
public:
    GANTemplate(int latent_dim, int output_dim,
               const std::vector<int>& generator_dims,
               const std::vector<int>& discriminator_dims);
    
    NeuralNetwork build() override;
    NeuralNetwork build_generator();
    NeuralNetwork build_discriminator();
    std::string name() const override { return "GAN"; }
    
private:
    int latent_dim_;
    int output_dim_;
    std::vector<int> generator_dims_;
    std::vector<int> discriminator_dims_;
};

// U-Net Template (for segmentation)
class UNetTemplate : public NetworkTemplate {
public:
    UNetTemplate(int input_channels, int num_classes,
                int base_filters = 64, int depth = 4);
    
    NeuralNetwork build() override;
    std::string name() const override { return "UNet"; }
    
private:
    int input_channels_;
    int num_classes_;
    int base_filters_;
    int depth_;
};

// Transformer Template
class TransformerTemplate : public NetworkTemplate {
public:
    TransformerTemplate(int input_dim, int model_dim, int num_heads,
                       int num_layers, int ff_dim, int output_dim,
                       double dropout = 0.1);
    
    NeuralNetwork build() override;
    std::string name() const override { return "Transformer"; }
    
private:
    int input_dim_;
    int model_dim_;
    int num_heads_;
    int num_layers_;
    int ff_dim_;
    int output_dim_;
    double dropout_;
};

// Large Language Model Template (GPT-style decoder-only transformer)
class LLMTemplate : public NetworkTemplate {
public:
    LLMTemplate(int vocab_size, int context_length, int embed_dim,
               int num_heads, int num_layers, int ff_dim,
               double dropout = 0.1, bool causal = true);
    
    NeuralNetwork build() override;
    std::string name() const override { return "LLM"; }
    
    // Get embedding dimension
    int embed_dim() const { return embed_dim_; }
    int context_length() const { return context_length_; }
    
private:
    int vocab_size_;
    int context_length_;
    int embed_dim_;
    int num_heads_;
    int num_layers_;
    int ff_dim_;
    double dropout_;
    bool causal_;  // Whether to use causal (autoregressive) masking
};

// =========================================================================
// Quick Builder Functions
// =========================================================================

namespace templates {

// Classification templates
NeuralNetwork binary_classifier(int input_dim, 
                                const std::vector<int>& hidden_dims = {64, 32});

NeuralNetwork multiclass_classifier(int input_dim, int num_classes,
                                    const std::vector<int>& hidden_dims = {128, 64});

NeuralNetwork image_classifier(int num_classes, int channels = 3,
                              int height = 32, int width = 32,
                              const std::string& arch = "simple");

// Regression template
NeuralNetwork regressor(int input_dim, int output_dim = 1,
                       const std::vector<int>& hidden_dims = {64, 32});

// Embedding template
NeuralNetwork embedding_network(int input_dim, int embedding_dim,
                               const std::vector<int>& hidden_dims = {128, 64});

// Sequence templates
NeuralNetwork sequence_classifier(int input_dim, int num_classes,
                                 int hidden_dim = 128, int num_layers = 2);

NeuralNetwork sequence_to_sequence(int input_dim, int output_dim,
                                   int hidden_dim = 128, int num_layers = 2);

// Generative templates
NeuralNetwork simple_autoencoder(int input_dim, int latent_dim,
                                const std::vector<int>& hidden_dims = {128, 64});

NeuralNetwork variational_autoencoder(int input_dim, int latent_dim,
                                     const std::vector<int>& encoder_dims = {256, 128});

NeuralNetwork simple_gan(int latent_dim, int output_dim,
                        const std::vector<int>& generator_dims = {128, 256},
                        const std::vector<int>& discriminator_dims = {256, 128});

// Language model template
NeuralNetwork language_model(int vocab_size, int context_length = 512,
                            int embed_dim = 512, int num_heads = 8,
                            int num_layers = 6, int ff_dim = 2048);

} // namespace templates

} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_TEMPLATES_H
