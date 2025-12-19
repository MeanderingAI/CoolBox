#include "../../include/deep_learning/templates.h"
#include <stdexcept>

namespace deep_learning {

// ============================================================================
// MLPTemplate Implementation
// ============================================================================

MLPTemplate::MLPTemplate(int input_dim, 
                         std::vector<int> hidden_dims, 
                         int output_dim,
                         std::string activation,
                         double dropout_rate,
                         bool batch_norm)
    : input_dim_(input_dim),
      hidden_dims_(hidden_dims),
      output_dim_(output_dim),
      activation_(activation),
      dropout_rate_(dropout_rate),
      batch_norm_(batch_norm) {}

NeuralNetwork MLPTemplate::build() const {
    NeuralNetwork net;
    
    int prev_dim = input_dim_;
    
    // Hidden layers
    for (size_t i = 0; i < hidden_dims_.size(); ++i) {
        // Dense layer
        net.add_layer(std::make_shared<DenseLayer>(prev_dim, hidden_dims_[i]));
        
        // Batch normalization (optional)
        if (batch_norm_) {
            // net.add_layer(std::make_shared<BatchNormLayer>(hidden_dims_[i]));
        }
        
        // Activation
        if (activation_ == "relu") {
            net.add_layer(std::make_shared<ReLULayer>());
        } else if (activation_ == "tanh") {
            net.add_layer(std::make_shared<TanhLayer>());
        } else if (activation_ == "sigmoid") {
            net.add_layer(std::make_shared<SigmoidLayer>());
        }
        
        // Dropout (optional)
        if (dropout_rate_ > 0.0) {
            net.add_layer(std::make_shared<DropoutLayer>(dropout_rate_));
        }
        
        prev_dim = hidden_dims_[i];
    }
    
    // Output layer
    net.add_layer(std::make_shared<DenseLayer>(prev_dim, output_dim_));
    
    return net;
}

// ============================================================================
// CNNTemplate Implementation
// ============================================================================

CNNTemplate::CNNTemplate(Architecture arch,
                         int num_classes,
                         int input_channels,
                         int input_height,
                         int input_width)
    : arch_(arch),
      num_classes_(num_classes),
      input_channels_(input_channels),
      input_height_(input_height),
      input_width_(input_width) {}

NeuralNetwork CNNTemplate::build() const {
    NeuralNetwork net;
    
    switch (arch_) {
        case Architecture::SIMPLE: {
            // Simple 2-layer CNN
            // Conv1: 32 filters, 3x3
            // Conv2: 64 filters, 3x3
            // Flatten + Dense
            
            // Note: Actual conv layers would be added here
            // For now, using dense approximation
            int flattened_size = 64 * (input_height_ / 4) * (input_width_ / 4);
            
            net.add_layer(std::make_shared<DenseLayer>(
                input_channels_ * input_height_ * input_width_, 512));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<DenseLayer>(512, 256));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<DenseLayer>(256, num_classes_));
            break;
        }
        
        case Architecture::LENET: {
            // LeNet-5 style architecture
            int input_size = input_channels_ * input_height_ * input_width_;
            
            net.add_layer(std::make_shared<DenseLayer>(input_size, 256));
            net.add_layer(std::make_shared<TanhLayer>());
            net.add_layer(std::make_shared<DenseLayer>(256, 128));
            net.add_layer(std::make_shared<TanhLayer>());
            net.add_layer(std::make_shared<DenseLayer>(128, 84));
            net.add_layer(std::make_shared<TanhLayer>());
            net.add_layer(std::make_shared<DenseLayer>(84, num_classes_));
            break;
        }
        
        case Architecture::VGGLIKE: {
            // VGG-style deep network
            int input_size = input_channels_ * input_height_ * input_width_;
            
            net.add_layer(std::make_shared<DenseLayer>(input_size, 512));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<DenseLayer>(512, 512));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<DenseLayer>(512, 256));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<DenseLayer>(256, num_classes_));
            break;
        }
        
        case Architecture::RESNET: {
            // ResNet-style (without skip connections in this simplified version)
            int input_size = input_channels_ * input_height_ * input_width_;
            
            net.add_layer(std::make_shared<DenseLayer>(input_size, 256));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<DenseLayer>(256, 256));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<DenseLayer>(256, 128));
            net.add_layer(std::make_shared<ReLULayer>());
            net.add_layer(std::make_shared<DenseLayer>(128, num_classes_));
            break;
        }
    }
    
    return net;
}

std::string CNNTemplate::name() const {
    switch (arch_) {
        case Architecture::SIMPLE: return "SimpleCNN";
        case Architecture::LENET: return "LeNet";
        case Architecture::VGGLIKE: return "VGGLike";
        case Architecture::RESNET: return "ResNet";
        default: return "CNN";
    }
}

// ============================================================================
// AutoencoderTemplate Implementation
// ============================================================================

AutoencoderTemplate::AutoencoderTemplate(int input_dim,
                                         std::vector<int> encoder_dims,
                                         int latent_dim,
                                         bool variational)
    : input_dim_(input_dim),
      encoder_dims_(encoder_dims),
      latent_dim_(latent_dim),
      variational_(variational) {}

NeuralNetwork AutoencoderTemplate::build() const {
    NeuralNetwork net;
    
    // Encoder
    int prev_dim = input_dim_;
    for (int dim : encoder_dims_) {
        net.add_layer(std::make_shared<DenseLayer>(prev_dim, dim));
        net.add_layer(std::make_shared<ReLULayer>());
        prev_dim = dim;
    }
    
    // Latent layer
    net.add_layer(std::make_shared<DenseLayer>(prev_dim, latent_dim_));
    if (!variational_) {
        net.add_layer(std::make_shared<ReLULayer>());
    }
    
    // Decoder (mirror of encoder)
    prev_dim = latent_dim_;
    for (int i = encoder_dims_.size() - 1; i >= 0; --i) {
        net.add_layer(std::make_shared<DenseLayer>(prev_dim, encoder_dims_[i]));
        net.add_layer(std::make_shared<ReLULayer>());
        prev_dim = encoder_dims_[i];
    }
    
    // Output layer (reconstruction)
    net.add_layer(std::make_shared<DenseLayer>(prev_dim, input_dim_));
    net.add_layer(std::make_shared<SigmoidLayer>());  // For [0,1] range
    
    return net;
}

NeuralNetwork AutoencoderTemplate::build_encoder() const {
    NeuralNetwork encoder;
    
    int prev_dim = input_dim_;
    for (int dim : encoder_dims_) {
        encoder.add_layer(std::make_shared<DenseLayer>(prev_dim, dim));
        encoder.add_layer(std::make_shared<ReLULayer>());
        prev_dim = dim;
    }
    
    encoder.add_layer(std::make_shared<DenseLayer>(prev_dim, latent_dim_));
    
    return encoder;
}

NeuralNetwork AutoencoderTemplate::build_decoder() const {
    NeuralNetwork decoder;
    
    int prev_dim = latent_dim_;
    for (int i = encoder_dims_.size() - 1; i >= 0; --i) {
        decoder.add_layer(std::make_shared<DenseLayer>(prev_dim, encoder_dims_[i]));
        decoder.add_layer(std::make_shared<ReLULayer>());
        prev_dim = encoder_dims_[i];
    }
    
    decoder.add_layer(std::make_shared<DenseLayer>(prev_dim, input_dim_));
    decoder.add_layer(std::make_shared<SigmoidLayer>());
    
    return decoder;
}

// ============================================================================
// RNNTemplate Implementation
// ============================================================================

RNNTemplate::RNNTemplate(int input_dim,
                         int hidden_dim,
                         int num_layers,
                         int output_dim,
                         CellType cell_type,
                         bool bidirectional,
                         double dropout)
    : input_dim_(input_dim),
      hidden_dim_(hidden_dim),
      num_layers_(num_layers),
      output_dim_(output_dim),
      cell_type_(cell_type),
      bidirectional_(bidirectional),
      dropout_(dropout) {}

NeuralNetwork RNNTemplate::build() const {
    NeuralNetwork net;
    
    int multiplier = bidirectional_ ? 2 : 1;
    
    // Input projection
    net.add_layer(std::make_shared<DenseLayer>(input_dim_, hidden_dim_));
    net.add_layer(std::make_shared<TanhLayer>());
    
    // Recurrent layers (simplified as dense layers)
    for (int i = 0; i < num_layers_; ++i) {
        net.add_layer(std::make_shared<DenseLayer>(
            hidden_dim_ * multiplier, hidden_dim_ * multiplier));
        net.add_layer(std::make_shared<TanhLayer>());
        
        if (dropout_ > 0.0 && i < num_layers_ - 1) {
            net.add_layer(std::make_shared<DropoutLayer>(dropout_));
        }
    }
    
    // Output projection
    net.add_layer(std::make_shared<DenseLayer>(hidden_dim_ * multiplier, output_dim_));
    
    return net;
}

std::string RNNTemplate::name() const {
    std::string base;
    switch (cell_type_) {
        case CellType::VANILLA: base = "RNN"; break;
        case CellType::LSTM: base = "LSTM"; break;
        case CellType::GRU: base = "GRU"; break;
        default: base = "RNN";
    }
    if (bidirectional_) base = "Bi" + base;
    return base;
}

// ============================================================================
// SiameseTemplate Implementation
// ============================================================================

SiameseTemplate::SiameseTemplate(int input_dim,
                                 std::vector<int> hidden_dims,
                                 int embedding_dim,
                                 std::string distance_metric)
    : input_dim_(input_dim),
      hidden_dims_(hidden_dims),
      embedding_dim_(embedding_dim),
      distance_metric_(distance_metric) {}

NeuralNetwork SiameseTemplate::build() const {
    // Returns the shared embedding network
    return build_embedding_network();
}

NeuralNetwork SiameseTemplate::build_embedding_network() const {
    NeuralNetwork net;
    
    int prev_dim = input_dim_;
    for (int dim : hidden_dims_) {
        net.add_layer(std::make_shared<DenseLayer>(prev_dim, dim));
        net.add_layer(std::make_shared<ReLULayer>());
        prev_dim = dim;
    }
    
    // Embedding layer (no activation for metric learning)
    net.add_layer(std::make_shared<DenseLayer>(prev_dim, embedding_dim_));
    
    return net;
}

// ============================================================================
// GANTemplate Implementation
// ============================================================================

GANTemplate::GANTemplate(int latent_dim,
                         int output_dim,
                         std::vector<int> generator_dims,
                         std::vector<int> discriminator_dims)
    : latent_dim_(latent_dim),
      output_dim_(output_dim),
      generator_dims_(generator_dims),
      discriminator_dims_(discriminator_dims) {}

NeuralNetwork GANTemplate::build() const {
    // For GAN, typically train generator and discriminator separately
    return build_generator();
}

NeuralNetwork GANTemplate::build_generator() const {
    NeuralNetwork generator;
    
    int prev_dim = latent_dim_;
    for (int dim : generator_dims_) {
        generator.add_layer(std::make_shared<DenseLayer>(prev_dim, dim));
        generator.add_layer(std::make_shared<ReLULayer>());
        prev_dim = dim;
    }
    
    generator.add_layer(std::make_shared<DenseLayer>(prev_dim, output_dim_));
    generator.add_layer(std::make_shared<TanhLayer>());  // [-1, 1] output
    
    return generator;
}

NeuralNetwork GANTemplate::build_discriminator() const {
    NeuralNetwork discriminator;
    
    int prev_dim = output_dim_;
    for (int dim : discriminator_dims_) {
        discriminator.add_layer(std::make_shared<DenseLayer>(prev_dim, dim));
        discriminator.add_layer(std::make_shared<ReLULayer>());
        discriminator.add_layer(std::make_shared<DropoutLayer>(0.3));
        prev_dim = dim;
    }
    
    discriminator.add_layer(std::make_shared<DenseLayer>(prev_dim, 1));
    discriminator.add_layer(std::make_shared<SigmoidLayer>());  // [0, 1] probability
    
    return discriminator;
}

// ============================================================================
// Quick Builder Functions
// ============================================================================

namespace templates {

NeuralNetwork binary_classifier(int input_dim, std::vector<int> hidden_dims) {
    MLPTemplate template_net(input_dim, hidden_dims, 1, "relu");
    auto net = template_net.build();
    net.add_layer(std::make_shared<SigmoidLayer>());
    net.set_loss(std::make_shared<BCELoss>());
    net.set_optimizer(std::make_shared<Adam>(0.001));
    return net;
}

NeuralNetwork multiclass_classifier(int input_dim, int num_classes, 
                                    std::vector<int> hidden_dims) {
    MLPTemplate template_net(input_dim, hidden_dims, num_classes, "relu");
    auto net = template_net.build();
    net.add_layer(std::make_shared<SoftmaxLayer>());
    net.set_loss(std::make_shared<CategoricalCrossEntropyLoss>());
    net.set_optimizer(std::make_shared<Adam>(0.001));
    return net;
}

NeuralNetwork image_classifier(int num_classes, int channels, 
                               int height, int width, std::string arch) {
    CNNTemplate::Architecture cnn_arch;
    if (arch == "lenet") {
        cnn_arch = CNNTemplate::Architecture::LENET;
    } else if (arch == "vgg") {
        cnn_arch = CNNTemplate::Architecture::VGGLIKE;
    } else if (arch == "resnet") {
        cnn_arch = CNNTemplate::Architecture::RESNET;
    } else {
        cnn_arch = CNNTemplate::Architecture::SIMPLE;
    }
    
    CNNTemplate template_net(cnn_arch, num_classes, channels, height, width);
    auto net = template_net.build();
    net.add_layer(std::make_shared<SoftmaxLayer>());
    net.set_loss(std::make_shared<CategoricalCrossEntropyLoss>());
    net.set_optimizer(std::make_shared<Adam>(0.001));
    return net;
}

NeuralNetwork regressor(int input_dim, int output_dim, 
                       std::vector<int> hidden_dims) {
    MLPTemplate template_net(input_dim, hidden_dims, output_dim, "relu");
    auto net = template_net.build();
    net.set_loss(std::make_shared<MSELoss>());
    net.set_optimizer(std::make_shared<Adam>(0.001));
    return net;
}

NeuralNetwork embedding_network(int input_dim, int embedding_dim,
                               std::vector<int> hidden_dims) {
    MLPTemplate template_net(input_dim, hidden_dims, embedding_dim, "relu");
    return template_net.build();
}

NeuralNetwork sequence_classifier(int input_dim, int num_classes,
                                 int hidden_dim, int num_layers) {
    RNNTemplate template_net(input_dim, hidden_dim, num_layers, num_classes,
                            RNNTemplate::CellType::LSTM);
    auto net = template_net.build();
    net.add_layer(std::make_shared<SoftmaxLayer>());
    net.set_loss(std::make_shared<CategoricalCrossEntropyLoss>());
    net.set_optimizer(std::make_shared<Adam>(0.001));
    return net;
}

NeuralNetwork sequence_to_sequence(int input_dim, int output_dim,
                                  int hidden_dim, int num_layers) {
    RNNTemplate template_net(input_dim, hidden_dim, num_layers, output_dim,
                            RNNTemplate::CellType::LSTM);
    auto net = template_net.build();
    net.set_loss(std::make_shared<MSELoss>());
    net.set_optimizer(std::make_shared<Adam>(0.001));
    return net;
}

NeuralNetwork simple_autoencoder(int input_dim, int latent_dim,
                                std::vector<int> hidden_dims) {
    AutoencoderTemplate template_net(input_dim, hidden_dims, latent_dim, false);
    auto net = template_net.build();
    net.set_loss(std::make_shared<MSELoss>());
    net.set_optimizer(std::make_shared<Adam>(0.001));
    return net;
}

NeuralNetwork variational_autoencoder(int input_dim, int latent_dim,
                                     std::vector<int> encoder_dims) {
    AutoencoderTemplate template_net(input_dim, encoder_dims, latent_dim, true);
    auto net = template_net.build();
    net.set_loss(std::make_shared<MSELoss>());
    net.set_optimizer(std::make_shared<Adam>(0.001));
    return net;
}

NeuralNetwork simple_gan(int latent_dim, int output_dim,
                        std::vector<int> gen_dims,
                        std::vector<int> disc_dims) {
    GANTemplate template_net(latent_dim, output_dim, gen_dims, disc_dims);
    return template_net.build_generator();
}

} // namespace templates

} // namespace deep_learning
