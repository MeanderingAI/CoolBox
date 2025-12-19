#pragma once

#include "neural_network.h"
#include "layer.h"
#include "loss.h"
#include "optimizer.h"
#include <memory>
#include <vector>
#include <string>

namespace deep_learning {

// Base template class for pre-built architectures
class NetworkTemplate {
public:
    virtual ~NetworkTemplate() = default;
    virtual NeuralNetwork build() const = 0;
    virtual std::string name() const = 0;
};

// Multi-Layer Perceptron (Fully Connected Network)
class MLPTemplate : public NetworkTemplate {
public:
    MLPTemplate(int input_dim, 
                std::vector<int> hidden_dims, 
                int output_dim,
                std::string activation = "relu",
                double dropout_rate = 0.0,
                bool batch_norm = false);
    
    NeuralNetwork build() const override;
    std::string name() const override { return "MLP"; }
    
private:
    int input_dim_;
    std::vector<int> hidden_dims_;
    int output_dim_;
    std::string activation_;
    double dropout_rate_;
    bool batch_norm_;
};

// Convolutional Neural Network for image classification
class CNNTemplate : public NetworkTemplate {
public:
    enum class Architecture {
        SIMPLE,     // Simple 2-conv network
        LENET,      // LeNet-5 style
        VGGLIKE,    // VGG-style blocks
        RESNET      // ResNet-style with skip connections
    };
    
    CNNTemplate(Architecture arch,
                int num_classes,
                int input_channels = 3,
                int input_height = 32,
                int input_width = 32);
    
    NeuralNetwork build() const override;
    std::string name() const override;
    
private:
    Architecture arch_;
    int num_classes_;
    int input_channels_;
    int input_height_;
    int input_width_;
};

// Autoencoder for dimensionality reduction
class AutoencoderTemplate : public NetworkTemplate {
public:
    AutoencoderTemplate(int input_dim,
                       std::vector<int> encoder_dims,
                       int latent_dim,
                       bool variational = false);
    
    NeuralNetwork build() const override;
    std::string name() const override { 
        return variational_ ? "VariationalAutoencoder" : "Autoencoder"; 
    }
    
    // Get encoder/decoder separately
    NeuralNetwork build_encoder() const;
    NeuralNetwork build_decoder() const;
    
private:
    int input_dim_;
    std::vector<int> encoder_dims_;
    int latent_dim_;
    bool variational_;
};

// Recurrent Neural Network
class RNNTemplate : public NetworkTemplate {
public:
    enum class CellType {
        VANILLA,    // Simple RNN
        LSTM,       // Long Short-Term Memory
        GRU         // Gated Recurrent Unit
    };
    
    RNNTemplate(int input_dim,
                int hidden_dim,
                int num_layers,
                int output_dim,
                CellType cell_type = CellType::LSTM,
                bool bidirectional = false,
                double dropout = 0.0);
    
    NeuralNetwork build() const override;
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

// Transformer block
class TransformerTemplate : public NetworkTemplate {
public:
    TransformerTemplate(int d_model,
                       int num_heads,
                       int d_ff,
                       int num_layers,
                       int vocab_size,
                       int max_seq_length,
                       double dropout = 0.1);
    
    NeuralNetwork build() const override;
    std::string name() const override { return "Transformer"; }
    
private:
    int d_model_;
    int num_heads_;
    int d_ff_;
    int num_layers_;
    int vocab_size_;
    int max_seq_length_;
    double dropout_;
};

// Siamese Network for similarity learning
class SiameseTemplate : public NetworkTemplate {
public:
    SiameseTemplate(int input_dim,
                   std::vector<int> hidden_dims,
                   int embedding_dim,
                   std::string distance_metric = "euclidean");
    
    NeuralNetwork build() const override;
    std::string name() const override { return "Siamese"; }
    
    // Build the shared embedding network
    NeuralNetwork build_embedding_network() const;
    
private:
    int input_dim_;
    std::vector<int> hidden_dims_;
    int embedding_dim_;
    std::string distance_metric_;
};

// Generative Adversarial Network
class GANTemplate : public NetworkTemplate {
public:
    GANTemplate(int latent_dim,
               int output_dim,
               std::vector<int> generator_dims,
               std::vector<int> discriminator_dims);
    
    NeuralNetwork build() const override;  // Returns full GAN
    std::string name() const override { return "GAN"; }
    
    // Get generator/discriminator separately
    NeuralNetwork build_generator() const;
    NeuralNetwork build_discriminator() const;
    
private:
    int latent_dim_;
    int output_dim_;
    std::vector<int> generator_dims_;
    std::vector<int> discriminator_dims_;
};

// U-Net for segmentation
class UNetTemplate : public NetworkTemplate {
public:
    UNetTemplate(int input_channels,
                int output_channels,
                std::vector<int> encoder_channels = {64, 128, 256, 512},
                bool use_batch_norm = true);
    
    NeuralNetwork build() const override;
    std::string name() const override { return "UNet"; }
    
private:
    int input_channels_;
    int output_channels_;
    std::vector<int> encoder_channels_;
    bool use_batch_norm_;
};

// Quick builder functions for common configurations
namespace templates {
    // Classification networks
    NeuralNetwork binary_classifier(int input_dim, 
                                    std::vector<int> hidden_dims = {64, 32});
    
    NeuralNetwork multiclass_classifier(int input_dim, 
                                       int num_classes,
                                       std::vector<int> hidden_dims = {128, 64});
    
    NeuralNetwork image_classifier(int num_classes,
                                  int channels = 3,
                                  int height = 32,
                                  int width = 32,
                                  std::string arch = "simple");
    
    // Regression networks
    NeuralNetwork regressor(int input_dim,
                           int output_dim = 1,
                           std::vector<int> hidden_dims = {64, 32});
    
    // Embedding networks
    NeuralNetwork embedding_network(int input_dim,
                                   int embedding_dim,
                                   std::vector<int> hidden_dims = {128, 64});
    
    // Sequence models
    NeuralNetwork sequence_classifier(int input_dim,
                                     int num_classes,
                                     int hidden_dim = 128,
                                     int num_layers = 2);
    
    NeuralNetwork sequence_to_sequence(int input_dim,
                                      int output_dim,
                                      int hidden_dim = 256,
                                      int num_layers = 2);
    
    // Generative models
    NeuralNetwork simple_autoencoder(int input_dim,
                                    int latent_dim,
                                    std::vector<int> hidden_dims = {128, 64});
    
    NeuralNetwork variational_autoencoder(int input_dim,
                                         int latent_dim,
                                         std::vector<int> encoder_dims = {256, 128});
    
    NeuralNetwork simple_gan(int latent_dim,
                            int output_dim,
                            std::vector<int> gen_dims = {128, 256},
                            std::vector<int> disc_dims = {256, 128});
}

} // namespace deep_learning
