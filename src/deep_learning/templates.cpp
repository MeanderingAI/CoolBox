#include "deep_learning/templates.h"
#include "deep_learning/layer.h"
#include "deep_learning/loss.h"
#include "deep_learning/optimizer.h"
#include <stdexcept>
#include <algorithm>

namespace ml {
namespace deep_learning {

// =========================================================================
// MLPTemplate Implementation
// =========================================================================

MLPTemplate::MLPTemplate(int input_dim, const std::vector<int>& hidden_dims,
                         int output_dim, const std::string& activation,
                         double dropout_rate, bool batch_norm)
    : input_dim_(input_dim), hidden_dims_(hidden_dims), output_dim_(output_dim),
      activation_(activation), dropout_rate_(dropout_rate), batch_norm_(batch_norm) {
}

NeuralNetwork MLPTemplate::build() {
    NeuralNetwork net;
    
    int prev_dim = input_dim_;
    
    // Add hidden layers
    for (size_t i = 0; i < hidden_dims_.size(); ++i) {
        // Dense layer
        net.add_layer(std::make_shared<DenseLayer>(prev_dim, hidden_dims_[i]));
        
        // Batch normalization (optional) - TODO: Implement BatchNormLayer
        // if (batch_norm_) {
        //     net.add_layer(std::make_shared<BatchNormLayer>(hidden_dims_[i]));
        // }
        
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

// =========================================================================
// CNNTemplate Implementation
// =========================================================================

CNNTemplate::CNNTemplate(Architecture architecture, int num_classes,
                         int input_channels, int input_height, int input_width)
    : architecture_(architecture), num_classes_(num_classes),
      input_channels_(input_channels), input_height_(input_height),
      input_width_(input_width) {
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

NeuralNetwork CNNTemplate::build() {
    NeuralNetwork net;
    
    switch (architecture_) {
        case Architecture::SIMPLE:
            build_simple(net);
            break;
        case Architecture::LENET:
            build_lenet(net);
            break;
        case Architecture::VGGLIKE:
            build_vgglike(net);
            break;
        case Architecture::RESNET:
            build_resnet(net);
            break;
    }
    
    return net;
}

void CNNTemplate::build_simple(NeuralNetwork& net) {
    // TODO: Implement Conv2D, MaxPool2D, and Flatten layers
    // Simple CNN: Conv -> ReLU -> Pool -> Conv -> ReLU -> Pool -> FC
    // net.add_layer(std::make_shared<Conv2DLayer>(input_channels_, 32, 3, 1, 1));
    // net.add_layer(std::make_shared<ReLULayer>());
    // net.add_layer(std::make_shared<MaxPool2DLayer>(2, 2));
    
    // For now, use a simple fully connected network
    int flattened = input_channels_ * input_height_ * input_width_;
    net.add_layer(std::make_shared<DenseLayer>(flattened, 128));
    net.add_layer(std::make_shared<ReLULayer>());
    net.add_layer(std::make_shared<DenseLayer>(128, num_classes_));
}

void CNNTemplate::build_lenet(NeuralNetwork& net) {
    // TODO: Implement LeNet-5 with Conv2D and AvgPool2D layers
    int flattened = input_channels_ * input_height_ * input_width_;
    net.add_layer(std::make_shared<DenseLayer>(flattened, 120));
    net.add_layer(std::make_shared<TanhLayer>());
    net.add_layer(std::make_shared<DenseLayer>(120, 84));
    net.add_layer(std::make_shared<TanhLayer>());
    net.add_layer(std::make_shared<DenseLayer>(84, num_classes_));
}

void CNNTemplate::build_vgglike(NeuralNetwork& net) {
    // TODO: Implement VGG-style with Conv2D blocks
    int flattened = input_channels_ * input_height_ * input_width_;
    net.add_layer(std::make_shared<DenseLayer>(flattened, 512));
    net.add_layer(std::make_shared<ReLULayer>());
    net.add_layer(std::make_shared<DropoutLayer>(0.5));
    net.add_layer(std::make_shared<DenseLayer>(512, num_classes_));
}

void CNNTemplate::build_resnet(NeuralNetwork& net) {
    // TODO: Implement ResNet with residual connections
    int flattened = input_channels_ * input_height_ * input_width_;
    net.add_layer(std::make_shared<DenseLayer>(flattened, 256));
    net.add_layer(std::make_shared<ReLULayer>());
    net.add_layer(std::make_shared<DenseLayer>(256, 64));
    net.add_layer(std::make_shared<ReLULayer>());
    net.add_layer(std::make_shared<DenseLayer>(64, num_classes_));
}

// =========================================================================
// AutoencoderTemplate Implementation
// =========================================================================

AutoencoderTemplate::AutoencoderTemplate(int input_dim, const std::vector<int>& encoder_dims,
                                         int latent_dim, bool variational)
    : input_dim_(input_dim), encoder_dims_(encoder_dims),
      latent_dim_(latent_dim), variational_(variational) {
}

NeuralNetwork AutoencoderTemplate::build() {
    NeuralNetwork net;
    
    // Encoder
    int prev_dim = input_dim_;
    for (int dim : encoder_dims_) {
        net.add_layer(std::make_shared<DenseLayer>(prev_dim, dim));
        net.add_layer(std::make_shared<ReLULayer>());
        prev_dim = dim;
    }
    
    // Latent layer
    if (variational_) {
        // For VAE, output mu and log_var
        net.add_layer(std::make_shared<DenseLayer>(prev_dim, latent_dim_ * 2));
    } else {
        net.add_layer(std::make_shared<DenseLayer>(prev_dim, latent_dim_));
    }
    
    // Decoder
    net.add_layer(std::make_shared<DenseLayer>(latent_dim_, encoder_dims_.back()));
    net.add_layer(std::make_shared<ReLULayer>());
    
    for (int i = encoder_dims_.size() - 2; i >= 0; --i) {
        net.add_layer(std::make_shared<DenseLayer>(encoder_dims_[i + 1], encoder_dims_[i]));
        net.add_layer(std::make_shared<ReLULayer>());
    }
    
    // Reconstruction layer
    net.add_layer(std::make_shared<DenseLayer>(encoder_dims_[0], input_dim_));
    net.add_layer(std::make_shared<SigmoidLayer>());
    
    return net;
}

NeuralNetwork AutoencoderTemplate::build_encoder() {
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

NeuralNetwork AutoencoderTemplate::build_decoder() {
    NeuralNetwork decoder;
    
    decoder.add_layer(std::make_shared<DenseLayer>(latent_dim_, encoder_dims_.back()));
    decoder.add_layer(std::make_shared<ReLULayer>());
    
    for (int i = encoder_dims_.size() - 2; i >= 0; --i) {
        decoder.add_layer(std::make_shared<DenseLayer>(encoder_dims_[i + 1], encoder_dims_[i]));
        decoder.add_layer(std::make_shared<ReLULayer>());
    }
    
    decoder.add_layer(std::make_shared<DenseLayer>(encoder_dims_[0], input_dim_));
    decoder.add_layer(std::make_shared<SigmoidLayer>());
    
    return decoder;
}

// =========================================================================
// RNNTemplate Implementation
// =========================================================================

RNNTemplate::RNNTemplate(int input_dim, int hidden_dim, int num_layers,
                         int output_dim, CellType cell_type, bool bidirectional,
                         double dropout)
    : input_dim_(input_dim), hidden_dim_(hidden_dim), num_layers_(num_layers),
      output_dim_(output_dim), cell_type_(cell_type),
      bidirectional_(bidirectional), dropout_(dropout) {
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

NeuralNetwork RNNTemplate::build() {
    NeuralNetwork net;
    
    // TODO: Implement LSTM, GRU, and RNN layers
    // For now, use DenseLayer as a placeholder
    for (int i = 0; i < num_layers_; ++i) {
        int input_size = (i == 0) ? input_dim_ : hidden_dim_;
        
        // Placeholder: use DenseLayer instead of RNN variants
        net.add_layer(std::make_shared<DenseLayer>(input_size, hidden_dim_));
        net.add_layer(std::make_shared<TanhLayer>());
        
        // Add dropout between layers
        if (dropout_ > 0.0 && i < num_layers_ - 1) {
            net.add_layer(std::make_shared<DropoutLayer>(dropout_));
        }
    }
    
    // Output layer
    int rnn_output_dim = bidirectional_ ? hidden_dim_ * 2 : hidden_dim_;
    net.add_layer(std::make_shared<DenseLayer>(rnn_output_dim, output_dim_));
    
    return net;
}

// =========================================================================
// SiameseTemplate Implementation
// =========================================================================

SiameseTemplate::SiameseTemplate(int input_dim, const std::vector<int>& hidden_dims,
                                 int embedding_dim, const std::string& distance_metric)
    : input_dim_(input_dim), hidden_dims_(hidden_dims),
      embedding_dim_(embedding_dim), distance_metric_(distance_metric) {
}

NeuralNetwork SiameseTemplate::build() {
    // Return the embedding network (to be used twice for siamese architecture)
    return build_embedding_network();
}

NeuralNetwork SiameseTemplate::build_embedding_network() {
    NeuralNetwork net;
    
    int prev_dim = input_dim_;
    for (int dim : hidden_dims_) {
        net.add_layer(std::make_shared<DenseLayer>(prev_dim, dim));
        net.add_layer(std::make_shared<ReLULayer>());
        prev_dim = dim;
    }
    
    // Embedding layer
    net.add_layer(std::make_shared<DenseLayer>(prev_dim, embedding_dim_));
    
    // TODO: Implement L2NormLayer for proper cosine similarity
    // For now, skip normalization
    // if (distance_metric_ == "cosine") {
    //     net.add_layer(std::make_shared<L2NormLayer>());
    // }
    
    return net;
}

// =========================================================================
// GANTemplate Implementation
// =========================================================================

GANTemplate::GANTemplate(int latent_dim, int output_dim,
                         const std::vector<int>& generator_dims,
                         const std::vector<int>& discriminator_dims)
    : latent_dim_(latent_dim), output_dim_(output_dim),
      generator_dims_(generator_dims), discriminator_dims_(discriminator_dims) {
}

NeuralNetwork GANTemplate::build() {
    // By default, return the generator
    return build_generator();
}

NeuralNetwork GANTemplate::build_generator() {
    NeuralNetwork generator;
    
    int prev_dim = latent_dim_;
    for (size_t i = 0; i < generator_dims_.size(); ++i) {
        generator.add_layer(std::make_shared<DenseLayer>(prev_dim, generator_dims_[i]));
        generator.add_layer(std::make_shared<ReLULayer>());
        // TODO: Implement BatchNormLayer
        // generator.add_layer(std::make_shared<BatchNormLayer>(generator_dims_[i]));
        prev_dim = generator_dims_[i];
    }
    
    generator.add_layer(std::make_shared<DenseLayer>(prev_dim, output_dim_));
    generator.add_layer(std::make_shared<TanhLayer>());
    
    return generator;
}

NeuralNetwork GANTemplate::build_discriminator() {
    NeuralNetwork discriminator;
    
    int prev_dim = output_dim_;
    for (size_t i = 0; i < discriminator_dims_.size(); ++i) {
        discriminator.add_layer(std::make_shared<DenseLayer>(prev_dim, discriminator_dims_[i]));
        // TODO: Implement LeakyReLULayer  
        // discriminator.add_layer(std::make_shared<LeakyReLULayer>(0.2));
        discriminator.add_layer(std::make_shared<ReLULayer>());
        discriminator.add_layer(std::make_shared<DropoutLayer>(0.3));
        prev_dim = discriminator_dims_[i];
    }
    
    discriminator.add_layer(std::make_shared<DenseLayer>(prev_dim, 1));
    discriminator.add_layer(std::make_shared<SigmoidLayer>());
    
    return discriminator;
}

// =========================================================================
// UNetTemplate Implementation
// =========================================================================

UNetTemplate::UNetTemplate(int input_channels, int num_classes,
                           int base_filters, int depth)
    : input_channels_(input_channels), num_classes_(num_classes),
      base_filters_(base_filters), depth_(depth) {
}

NeuralNetwork UNetTemplate::build() {
    NeuralNetwork net;
    
    // TODO: Implement U-Net with Conv2D, pooling, and upsampling layers
    // Placeholder implementation with simple dense layers
    int flattened = input_channels_ * 32 * 32;  // Assuming 32x32 default
    net.add_layer(std::make_shared<DenseLayer>(flattened, 256));
    net.add_layer(std::make_shared<ReLULayer>());
    net.add_layer(std::make_shared<DenseLayer>(256, num_classes_));
    
    return net;
}

// =========================================================================
// TransformerTemplate Implementation
// =========================================================================

TransformerTemplate::TransformerTemplate(int input_dim, int model_dim, int num_heads,
                                         int num_layers, int ff_dim, int output_dim,
                                         double dropout)
    : input_dim_(input_dim), model_dim_(model_dim), num_heads_(num_heads),
      num_layers_(num_layers), ff_dim_(ff_dim), output_dim_(output_dim),
      dropout_(dropout) {
}

NeuralNetwork TransformerTemplate::build() {
    NeuralNetwork net;
    
    // TODO: Implement Transformer with MultiHeadAttention, LayerNorm
    // Simplified version using dense layers
    net.add_layer(std::make_shared<DenseLayer>(input_dim_, model_dim_));
    
    for (int i = 0; i < num_layers_; ++i) {
        net.add_layer(std::make_shared<DenseLayer>(model_dim_, ff_dim_));
        net.add_layer(std::make_shared<ReLULayer>());
        net.add_layer(std::make_shared<DropoutLayer>(dropout_));
        net.add_layer(std::make_shared<DenseLayer>(ff_dim_, model_dim_));
    }
    
    net.add_layer(std::make_shared<DenseLayer>(model_dim_, output_dim_));
    
    return net;
}

// =========================================================================
// LLMTemplate Implementation
// =========================================================================

LLMTemplate::LLMTemplate(int vocab_size, int context_length, int embed_dim,
                         int num_heads, int num_layers, int ff_dim,
                         double dropout, bool causal)
    : vocab_size_(vocab_size), context_length_(context_length),
      embed_dim_(embed_dim), num_heads_(num_heads), num_layers_(num_layers),
      ff_dim_(ff_dim), dropout_(dropout), causal_(causal) {
}

NeuralNetwork LLMTemplate::build() {
    NeuralNetwork net;
    
    // TODO: Implement LLM with Embedding, Positional Encoding, Causal Attention, LayerNorm
    // Simplified version using dense layers
    net.add_layer(std::make_shared<DenseLayer>(vocab_size_, embed_dim_));
    net.add_layer(std::make_shared<DropoutLayer>(dropout_));
    
    // Simplified transformer blocks
    for (int i = 0; i < num_layers_; ++i) {
        net.add_layer(std::make_shared<DenseLayer>(embed_dim_, ff_dim_));
        net.add_layer(std::make_shared<ReLULayer>());  // Should be GELU but not implemented
        net.add_layer(std::make_shared<DenseLayer>(ff_dim_, embed_dim_));
        net.add_layer(std::make_shared<DropoutLayer>(dropout_));
    }
    
    // Language modeling head
    net.add_layer(std::make_shared<DenseLayer>(embed_dim_, vocab_size_));
    
    return net;
}

// =========================================================================
// Quick Builder Functions
// =========================================================================

namespace templates {

NeuralNetwork binary_classifier(int input_dim, const std::vector<int>& hidden_dims) {
    MLPTemplate template_builder(input_dim, hidden_dims, 1, "relu", 0.0, false);
    NeuralNetwork net = template_builder.build();
    net.add_layer(std::make_shared<SigmoidLayer>());
    net.set_loss(std::make_shared<BCELoss>());
    net.set_optimizer(std::make_shared<Adam>(0.001));
    return net;
}

NeuralNetwork multiclass_classifier(int input_dim, int num_classes,
                                    const std::vector<int>& hidden_dims) {
    MLPTemplate template_builder(input_dim, hidden_dims, num_classes, "relu", 0.0, false);
    NeuralNetwork net = template_builder.build();
    net.add_layer(std::make_shared<SoftmaxLayer>());
    net.set_loss(std::make_shared<CategoricalCrossEntropyLoss>());
    net.set_optimizer(std::make_shared<Adam>(0.001));
    return net;
}

NeuralNetwork image_classifier(int num_classes, int channels, int height, int width,
                              const std::string& arch) {
    CNNTemplate::Architecture architecture;
    if (arch == "lenet") {
        architecture = CNNTemplate::Architecture::LENET;
    } else if (arch == "vgg") {
        architecture = CNNTemplate::Architecture::VGGLIKE;
    } else if (arch == "resnet") {
        architecture = CNNTemplate::Architecture::RESNET;
    } else {
        architecture = CNNTemplate::Architecture::SIMPLE;
    }
    
    CNNTemplate template_builder(architecture, num_classes, channels, height, width);
    NeuralNetwork net = template_builder.build();
    net.add_layer(std::make_shared<SoftmaxLayer>());
    net.set_loss(std::make_shared<CategoricalCrossEntropyLoss>());
    net.set_optimizer(std::make_shared<Adam>(0.001));
    return net;
}

NeuralNetwork regressor(int input_dim, int output_dim,
                       const std::vector<int>& hidden_dims) {
    MLPTemplate template_builder(input_dim, hidden_dims, output_dim, "relu", 0.0, false);
    NeuralNetwork net = template_builder.build();
    net.set_loss(std::make_shared<MSELoss>());
    net.set_optimizer(std::make_shared<Adam>(0.001));
    return net;
}

NeuralNetwork embedding_network(int input_dim, int embedding_dim,
                               const std::vector<int>& hidden_dims) {
    SiameseTemplate template_builder(input_dim, hidden_dims, embedding_dim, "euclidean");
    return template_builder.build_embedding_network();
}

NeuralNetwork sequence_classifier(int input_dim, int num_classes,
                                 int hidden_dim, int num_layers) {
    RNNTemplate template_builder(input_dim, hidden_dim, num_layers, num_classes,
                                RNNTemplate::CellType::LSTM, false, 0.0);
    NeuralNetwork net = template_builder.build();
    net.add_layer(std::make_shared<SoftmaxLayer>());
    net.set_loss(std::make_shared<CategoricalCrossEntropyLoss>());
    net.set_optimizer(std::make_shared<Adam>(0.001));
    return net;
}

NeuralNetwork sequence_to_sequence(int input_dim, int output_dim,
                                   int hidden_dim, int num_layers) {
    RNNTemplate encoder(input_dim, hidden_dim, num_layers, hidden_dim,
                       RNNTemplate::CellType::LSTM, false, 0.0);
    RNNTemplate decoder(hidden_dim, hidden_dim, num_layers, output_dim,
                       RNNTemplate::CellType::LSTM, false, 0.0);
    
    // For simplicity, return the encoder-decoder as a single network
    // In practice, this would need custom handling
    NeuralNetwork net = encoder.build();
    return net;
}

NeuralNetwork simple_autoencoder(int input_dim, int latent_dim,
                                const std::vector<int>& hidden_dims) {
    AutoencoderTemplate template_builder(input_dim, hidden_dims, latent_dim, false);
    NeuralNetwork net = template_builder.build();
    net.set_loss(std::make_shared<MSELoss>());
    net.set_optimizer(std::make_shared<Adam>(0.001));
    return net;
}

NeuralNetwork variational_autoencoder(int input_dim, int latent_dim,
                                     const std::vector<int>& encoder_dims) {
    AutoencoderTemplate template_builder(input_dim, encoder_dims, latent_dim, true);
    NeuralNetwork net = template_builder.build();
    // VAE needs special loss combining reconstruction and KL divergence
    net.set_loss(std::make_shared<MSELoss>());
    net.set_optimizer(std::make_shared<Adam>(0.001));
    return net;
}

NeuralNetwork simple_gan(int latent_dim, int output_dim,
                        const std::vector<int>& generator_dims,
                        const std::vector<int>& discriminator_dims) {
    GANTemplate template_builder(latent_dim, output_dim, generator_dims, discriminator_dims);
    return template_builder.build_generator();
}

NeuralNetwork language_model(int vocab_size, int context_length,
                            int embed_dim, int num_heads,
                            int num_layers, int ff_dim) {
    LLMTemplate template_builder(vocab_size, context_length, embed_dim,
                                num_heads, num_layers, ff_dim, 0.1, true);
    NeuralNetwork net = template_builder.build();
    net.set_loss(std::make_shared<CategoricalCrossEntropyLoss>());
    net.set_optimizer(std::make_shared<Adam>(0.0001));  // Lower LR for LLMs
    return net;
}

} // namespace templates

} // namespace deep_learning
} // namespace ml
