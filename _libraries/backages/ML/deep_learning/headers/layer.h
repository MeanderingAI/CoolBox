#ifndef LAYER_H
#define LAYER_H

#include "tensor.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace ml {
namespace deep_learning {

class Layer {
public:
    virtual ~Layer() = default;
    
    // Forward pass
    virtual Tensor forward(const Tensor& input) = 0;
    
    // Backward pass (returns gradient w.r.t. input)
    virtual Tensor backward(const Tensor& gradient) = 0;
    
    // Update parameters with optimizer
    virtual void update_parameters(double learning_rate) {}
    
    // Getters
    virtual std::string name() const = 0;
    virtual bool has_parameters() const { return false; }
    
protected:
    Tensor last_input_;
    Tensor last_output_;
};

// ============================================================================
// Dense (Fully Connected) Layer
// ============================================================================
class DenseLayer : public Layer {
public:
    DenseLayer(size_t input_size, size_t output_size);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override;
    
    std::string name() const override { return "Dense"; }
    bool has_parameters() const override { return true; }
    
    const Tensor& weights() const { return weights_; }
    const Tensor& bias() const { return bias_; }
    
private:
    Tensor weights_;      // Shape: [input_size, output_size]
    Tensor bias_;         // Shape: [output_size]
    Tensor weight_gradient_;
    Tensor bias_gradient_;
    size_t input_size_;
    size_t output_size_;
};

// ============================================================================
// Activation Layers
// ============================================================================
class ReLULayer : public Layer {
public:
    ReLULayer() = default;
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    std::string name() const override { return "ReLU"; }
};

class SigmoidLayer : public Layer {
public:
    SigmoidLayer() = default;
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    std::string name() const override { return "Sigmoid"; }
};

class TanhLayer : public Layer {
public:
    TanhLayer() = default;
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    std::string name() const override { return "Tanh"; }
};

class SoftmaxLayer : public Layer {
public:
    SoftmaxLayer() = default;
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    std::string name() const override { return "Softmax"; }
};

// ============================================================================
// Utility Layers
// ============================================================================

// Dropout layer: randomly zeros elements during training
class DropoutLayer : public Layer {
public:
    explicit DropoutLayer(double rate = 0.5);
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    std::string name() const override { return "Dropout"; }
    
    void set_training(bool training) { training_ = training; }
    
private:
    double rate_;
    bool training_ = true;
    Tensor mask_;
};

// BatchNorm layer: normalizes activations across batch
class BatchNormLayer : public Layer {
public:
    explicit BatchNormLayer(size_t num_features, double epsilon = 1e-5, double momentum = 0.1);
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override;
    std::string name() const override { return "BatchNorm"; }
    bool has_parameters() const override { return true; }
    
    void set_training(bool training) { training_ = training; }
    
private:
    size_t num_features_;
    double epsilon_;
    double momentum_;
    bool training_ = true;
    Tensor gamma_;           // Scale parameter
    Tensor beta_;            // Shift parameter
    Tensor running_mean_;
    Tensor running_var_;
    Tensor gamma_gradient_;
    Tensor beta_gradient_;
    Tensor normalized_;      // Cached for backward
    Tensor batch_mean_;
    Tensor batch_var_;
};

// LayerNorm: normalizes across features (used in transformers)
class LayerNormLayer : public Layer {
public:
    explicit LayerNormLayer(size_t normalized_shape, double epsilon = 1e-5);
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override;
    std::string name() const override { return "LayerNorm"; }
    bool has_parameters() const override { return true; }
    
private:
    size_t normalized_shape_;
    double epsilon_;
    Tensor gamma_;
    Tensor beta_;
    Tensor gamma_gradient_;
    Tensor beta_gradient_;
    Tensor normalized_;
    Tensor std_inv_;
};

// Flatten layer: reshapes multi-dimensional input to 2D [batch, features]
class FlattenLayer : public Layer {
public:
    FlattenLayer() = default;
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    std::string name() const override { return "Flatten"; }
    
private:
    std::vector<size_t> original_shape_;
};

// ============================================================================
// Convolutional Layers
// ============================================================================

// 1D Convolution: input shape [batch, channels, length]
class Conv1DLayer : public Layer {
public:
    Conv1DLayer(size_t in_channels, size_t out_channels, size_t kernel_size,
                size_t stride = 1, size_t padding = 0);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override;
    
    std::string name() const override { return "Conv1D"; }
    bool has_parameters() const override { return true; }
    
    const Tensor& weights() const { return weights_; }
    const Tensor& bias() const { return bias_; }
    
private:
    size_t in_channels_;
    size_t out_channels_;
    size_t kernel_size_;
    size_t stride_;
    size_t padding_;
    Tensor weights_;          // Shape: [out_channels, in_channels, kernel_size]
    Tensor bias_;             // Shape: [out_channels]
    Tensor weight_gradient_;
    Tensor bias_gradient_;
};

// 2D Convolution: input shape [batch, channels, height, width]
class Conv2DLayer : public Layer {
public:
    Conv2DLayer(size_t in_channels, size_t out_channels,
                size_t kernel_h, size_t kernel_w,
                size_t stride_h = 1, size_t stride_w = 1,
                size_t pad_h = 0, size_t pad_w = 0);
    // Convenience constructor for square kernels (keeps parity with source)
    Conv2DLayer(size_t in_channels, size_t out_channels,
                size_t kernel_size, size_t stride = 1, size_t padding = 0);
    
    // Factory for square kernels to avoid overload ambiguity
    static std::shared_ptr<Conv2DLayer> create_square(size_t in_channels, size_t out_channels,
                                                     size_t kernel_size, size_t stride = 1,
                                                     size_t padding = 0);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override;
    
    std::string name() const override { return "Conv2D"; }
    bool has_parameters() const override { return true; }
    
    const Tensor& weights() const { return weights_; }
    const Tensor& bias() const { return bias_; }
    
private:
    size_t in_channels_;
    size_t out_channels_;
    size_t kernel_h_, kernel_w_;
    size_t stride_h_, stride_w_;
    size_t pad_h_, pad_w_;
    Tensor weights_;          // Shape: [out_channels, in_channels, kernel_h, kernel_w]
    Tensor bias_;             // Shape: [out_channels]
    Tensor weight_gradient_;
    Tensor bias_gradient_;
};

// 2D Max Pooling: input shape [batch, channels, height, width]
class MaxPool2DLayer : public Layer {
public:
    MaxPool2DLayer(size_t pool_h, size_t pool_w, size_t stride_h = 0, size_t stride_w = 0);
    
    // Convenience: square pooling
    explicit MaxPool2DLayer(size_t pool_size, size_t stride = 0);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    
    std::string name() const override { return "MaxPool2D"; }
    
private:
    size_t pool_h_, pool_w_;
    size_t stride_h_, stride_w_;
    Tensor max_indices_;  // Stores indices of max values for backward pass
    size_t input_h_, input_w_;
};

// Average Pooling 2D
class AvgPool2DLayer : public Layer {
public:
    AvgPool2DLayer(size_t pool_h, size_t pool_w, size_t stride_h = 0, size_t stride_w = 0);
    explicit AvgPool2DLayer(size_t pool_size, size_t stride = 0);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    
    std::string name() const override { return "AvgPool2D"; }
    
private:
    size_t pool_h_, pool_w_;
    size_t stride_h_, stride_w_;
    size_t input_h_, input_w_;
    size_t batch_size_, channels_;
};

// ============================================================================
// Recurrent Layers
// ============================================================================

// Vanilla RNN: input shape [batch, seq_len, input_size]
// Output shape [batch, seq_len, hidden_size] or [batch, hidden_size] if return_sequences=false
class RNNLayer : public Layer {
public:
    RNNLayer(size_t input_size, size_t hidden_size, bool return_sequences = true);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override;
    
    std::string name() const override { return "RNN"; }
    bool has_parameters() const override { return true; }
    
    void reset_hidden_state();
    
private:
    size_t input_size_;
    size_t hidden_size_;
    bool return_sequences_;
    
    Tensor W_ih_;    // Input-to-hidden weights [input_size, hidden_size]
    Tensor W_hh_;    // Hidden-to-hidden weights [hidden_size, hidden_size]
    Tensor b_h_;     // Hidden bias [hidden_size]
    
    Tensor W_ih_grad_;
    Tensor W_hh_grad_;
    Tensor b_h_grad_;
    
    Tensor hidden_state_;                 // Current hidden state
    std::vector<Tensor> hidden_states_;   // All hidden states (for BPTT)
    std::vector<Tensor> inputs_;          // Cached inputs per timestep
};

// LSTM: input shape [batch, seq_len, input_size]
class LSTMLayer : public Layer {
public:
    LSTMLayer(size_t input_size, size_t hidden_size, bool return_sequences = true);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override;
    
    std::string name() const override { return "LSTM"; }
    bool has_parameters() const override { return true; }
    
    void reset_hidden_state();
    
private:
    size_t input_size_;
    size_t hidden_size_;
    bool return_sequences_;
    
    // Gates: forget, input, cell candidate, output
    Tensor W_f_;  // Forget gate weights [input_size + hidden_size, hidden_size]
    Tensor b_f_;
    Tensor W_i_;  // Input gate weights
    Tensor b_i_;
    Tensor W_c_;  // Cell candidate weights
    Tensor b_c_;
    Tensor W_o_;  // Output gate weights
    Tensor b_o_;
    
    // Gradients
    Tensor W_f_grad_, b_f_grad_;
    Tensor W_i_grad_, b_i_grad_;
    Tensor W_c_grad_, b_c_grad_;
    Tensor W_o_grad_, b_o_grad_;
    
    Tensor hidden_state_;
    Tensor cell_state_;
    
    // Cached values for backward pass
    std::vector<Tensor> forget_gates_;
    std::vector<Tensor> input_gates_;
    std::vector<Tensor> cell_candidates_;
    std::vector<Tensor> output_gates_;
    std::vector<Tensor> cell_states_;
    std::vector<Tensor> hidden_states_;
    std::vector<Tensor> inputs_;
};

// GRU: input shape [batch, seq_len, input_size]
class GRULayer : public Layer {
public:
    GRULayer(size_t input_size, size_t hidden_size, bool return_sequences = true);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override;
    
    std::string name() const override { return "GRU"; }
    bool has_parameters() const override { return true; }
    
    void reset_hidden_state();
    
private:
    size_t input_size_;
    size_t hidden_size_;
    bool return_sequences_;
    
    // Gates: reset, update, candidate
    Tensor W_r_;  // Reset gate weights [input_size + hidden_size, hidden_size]
    Tensor b_r_;
    Tensor W_z_;  // Update gate weights
    Tensor b_z_;
    Tensor W_n_;  // New/candidate weights
    Tensor b_n_;
    
    Tensor W_r_grad_, b_r_grad_;
    Tensor W_z_grad_, b_z_grad_;
    Tensor W_n_grad_, b_n_grad_;
    
    Tensor hidden_state_;
    
    std::vector<Tensor> reset_gates_;
    std::vector<Tensor> update_gates_;
    std::vector<Tensor> candidates_;
    std::vector<Tensor> hidden_states_;
    std::vector<Tensor> inputs_;
};

// Embedding layer: maps integer indices to dense vectors
class EmbeddingLayer : public Layer {
public:
    EmbeddingLayer(size_t vocab_size, size_t embedding_dim);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override;
    
    std::string name() const override { return "Embedding"; }
    bool has_parameters() const override { return true; }
    
private:
    size_t vocab_size_;
    size_t embedding_dim_;
    Tensor embeddings_;       // Shape: [vocab_size, embedding_dim]
    Tensor embedding_grad_;
};

// ============================================================================
// Transformer Layers
// ============================================================================

// Scaled Dot-Product Attention
// Q, K, V shapes: [batch, seq_len, d_model] (or [batch, heads, seq_len, d_k])
class ScaledDotProductAttention {
public:
    Tensor forward(const Tensor& Q, const Tensor& K, const Tensor& V,
                   const Tensor* mask = nullptr);
    
    // Returns gradients for Q, K, V as a vector of 3 tensors
    std::vector<Tensor> backward(const Tensor& gradient);
    
private:
    Tensor attention_weights_;
    Tensor Q_, K_, V_;
    double scale_;
};

// Multi-Head Attention
class MultiHeadAttentionLayer : public Layer {
public:
    MultiHeadAttentionLayer(size_t d_model, size_t num_heads);
    
    Tensor forward(const Tensor& input) override;
    
    // Cross-attention forward: query from one source, key/value from another
    Tensor forward(const Tensor& query, const Tensor& key, const Tensor& value,
                   const Tensor* mask = nullptr);
    
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override;
    
    std::string name() const override { return "MultiHeadAttention"; }
    bool has_parameters() const override { return true; }
    
private:
    size_t d_model_;
    size_t num_heads_;
    size_t d_k_;  // d_model / num_heads
    
    // Linear projections
    Tensor W_q_, W_k_, W_v_, W_o_;  // Shape: [d_model, d_model]
    Tensor b_q_, b_k_, b_v_, b_o_;
    
    Tensor W_q_grad_, W_k_grad_, W_v_grad_, W_o_grad_;
    Tensor b_q_grad_, b_k_grad_, b_v_grad_, b_o_grad_;
    
    ScaledDotProductAttention attention_;
    
    // Cached for backward
    Tensor query_, key_, value_;
    Tensor Q_proj_, K_proj_, V_proj_;
    Tensor attention_output_;
};

// Position-wise Feed-Forward Network (used inside transformer blocks)
class FeedForwardLayer : public Layer {
public:
    FeedForwardLayer(size_t d_model, size_t d_ff);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override;
    
    std::string name() const override { return "FeedForward"; }
    bool has_parameters() const override { return true; }
    
private:
    DenseLayer linear1_;
    DenseLayer linear2_;
    ReLULayer relu_;
};

// Positional Encoding: adds sinusoidal position information
class PositionalEncodingLayer : public Layer {
public:
    PositionalEncodingLayer(size_t d_model, size_t max_seq_len = 5000, double dropout_rate = 0.1);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    
    std::string name() const override { return "PositionalEncoding"; }
    
private:
    size_t d_model_;
    size_t max_seq_len_;
    Tensor encoding_;  // Precomputed positional encoding table
    DropoutLayer dropout_;
};

// Transformer Encoder Layer: self-attention + feed-forward + residual + layer norm
class TransformerEncoderLayer : public Layer {
public:
    TransformerEncoderLayer(size_t d_model, size_t num_heads, size_t d_ff,
                            double dropout_rate = 0.1);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override;
    
    std::string name() const override { return "TransformerEncoder"; }
    bool has_parameters() const override { return true; }
    
private:
    MultiHeadAttentionLayer self_attention_;
    FeedForwardLayer feed_forward_;
    LayerNormLayer norm1_;
    LayerNormLayer norm2_;
    DropoutLayer dropout1_;
    DropoutLayer dropout2_;
    
    Tensor attn_output_;
    Tensor ff_input_;
};

// Transformer Decoder Layer: self-attention + cross-attention + feed-forward
class TransformerDecoderLayer : public Layer {
public:
    TransformerDecoderLayer(size_t d_model, size_t num_heads, size_t d_ff,
                            double dropout_rate = 0.1);
    
    Tensor forward(const Tensor& input) override;
    
    // Forward with encoder output for cross-attention
    Tensor forward(const Tensor& target, const Tensor& encoder_output,
                   const Tensor* target_mask = nullptr,
                   const Tensor* memory_mask = nullptr);
    
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override;
    
    std::string name() const override { return "TransformerDecoder"; }
    bool has_parameters() const override { return true; }
    
    void set_encoder_output(const Tensor& encoder_output) { encoder_output_ = encoder_output; }
    
private:
    MultiHeadAttentionLayer self_attention_;
    MultiHeadAttentionLayer cross_attention_;
    FeedForwardLayer feed_forward_;
    LayerNormLayer norm1_;
    LayerNormLayer norm2_;
    LayerNormLayer norm3_;
    DropoutLayer dropout1_;
    DropoutLayer dropout2_;
    DropoutLayer dropout3_;
    
    Tensor encoder_output_;
    Tensor self_attn_output_;
    Tensor cross_attn_output_;
    Tensor ff_input_;
};

} // namespace deep_learning
} // namespace ml
#include <memory>

// Inline factory definition
inline std::shared_ptr<ml::deep_learning::Conv2DLayer> ml::deep_learning::Conv2DLayer::create_square(size_t in_channels, size_t out_channels,
                                                                                                  size_t kernel_size, size_t stride, size_t padding) {
    return std::make_shared<ml::deep_learning::Conv2DLayer>(in_channels, out_channels,
                                                           kernel_size, kernel_size,
                                                           stride, stride,
                                                           padding, padding);
}
#endif // LAYER_H
