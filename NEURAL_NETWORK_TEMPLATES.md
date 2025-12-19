# Neural Network Templates Guide

## Overview

The neural network templates provide pre-configured architectures for common machine learning tasks. All templates are available through both C++ and Python bindings.

## Quick Start

### Python Usage

```python
import ml_core.deep_learning as dl

# Binary classification - simplest way
model = dl.binary_classifier(input_dim=10, hidden_dims=[64, 32])

# Multi-class classification
model = dl.multiclass_classifier(input_dim=784, num_classes=10, hidden_dims=[128, 64])

# Regression
model = dl.regressor(input_dim=20, output_dim=1, hidden_dims=[64, 32])

# Image classification
model = dl.image_classifier(num_classes=10, channels=3, height=32, width=32, arch='lenet')

# Autoencoder
model = dl.simple_autoencoder(input_dim=784, latent_dim=32, hidden_dims=[256, 128])
```

### C++ Usage

```cpp
#include "deep_learning/templates.h"

using namespace deep_learning;

// Binary classification
auto model = templates::binary_classifier(10, {64, 32});

// Multi-class classification  
auto model = templates::multiclass_classifier(784, 10, {128, 64});

// Regression
auto model = templates::regressor(20, 1, {64, 32});

// Image classification
auto model = templates::image_classifier(10, 3, 32, 32, "lenet");
```

## Template Classes

For more control, use template classes directly:

### MLPTemplate - Multi-Layer Perceptron

```python
# Python
template = dl.MLPTemplate(
    input_dim=50,
    hidden_dims=[128, 64, 32],
    output_dim=10,
    activation="relu",      # 'relu', 'tanh', 'sigmoid'
    dropout_rate=0.3,       # 0.0 to disable
    batch_norm=False        # Enable batch normalization
)
model = template.build()
```

```cpp
// C++
MLPTemplate mlp(50, {128, 64, 32}, 10, "relu", 0.3, false);
auto model = mlp.build();
```

**Use Cases:**
- General classification and regression
- Feature learning
- Fully connected networks

**Architectures:**
```
Input → Dense(128) → ReLU → Dropout(0.3) →
        Dense(64)  → ReLU → Dropout(0.3) →
        Dense(32)  → ReLU → Dropout(0.3) →
        Dense(Output)
```

### CNNTemplate - Convolutional Neural Networks

```python
# Python
template = dl.CNNTemplate(
    architecture=dl.CNNArchitecture.LENET,  # SIMPLE, LENET, VGGLIKE, RESNET
    num_classes=10,
    input_channels=1,    # 1 for grayscale, 3 for RGB
    input_height=28,
    input_width=28
)
model = template.build()
```

**Available Architectures:**

1. **SIMPLE** - Basic 2-layer CNN
   - Conv1: 32 filters, 3x3
   - Conv2: 64 filters, 3x3
   - Dense layers

2. **LENET** - LeNet-5 style
   - Classic CNN architecture
   - Tanh activations
   - Small parameter count

3. **VGGLIKE** - VGG-style deep network
   - Deep stacked layers
   - 3x3 convolutions
   - Max pooling

4. **RESNET** - ResNet-style with skip connections
   - Residual blocks
   - Batch normalization
   - Identity shortcuts

**Use Cases:**
- Image classification
- Object recognition
- Feature extraction from images

### AutoencoderTemplate - Dimensionality Reduction

```python
# Python
template = dl.AutoencoderTemplate(
    input_dim=784,              # e.g., 28x28 image
    encoder_dims=[256, 128, 64],
    latent_dim=32,
    variational=False           # True for VAE
)

# Build complete autoencoder
full_model = template.build()

# Or build components separately
encoder = template.build_encoder()
decoder = template.build_decoder()
```

**Architecture:**
```
Encoder:
  784 → Dense(256) → ReLU →
        Dense(128) → ReLU →
        Dense(64)  → ReLU →
        Dense(32)  [Latent]

Decoder:
  32  → Dense(64)  → ReLU →
        Dense(128) → ReLU →
        Dense(256) → ReLU →
        Dense(784) → Sigmoid
```

**Use Cases:**
- Dimensionality reduction
- Anomaly detection
- Data denoising
- Feature learning
- Generative modeling (VAE)

**Variational Autoencoder:**
```python
vae = dl.AutoencoderTemplate(
    input_dim=784,
    encoder_dims=[256, 128],
    latent_dim=16,
    variational=True  # Enables reparameterization trick
)
```

### RNNTemplate - Recurrent Neural Networks

```python
# Python
template = dl.RNNTemplate(
    input_dim=50,
    hidden_dim=128,
    num_layers=2,
    output_dim=10,
    cell_type=dl.RNNCellType.LSTM,    # VANILLA, LSTM, GRU
    bidirectional=True,                # Bidirectional RNN
    dropout=0.3                        # Dropout between layers
)
model = template.build()
```

**Cell Types:**
- **VANILLA**: Simple RNN (prone to vanishing gradients)
- **LSTM**: Long Short-Term Memory (default, best for most tasks)
- **GRU**: Gated Recurrent Unit (faster than LSTM, similar performance)

**Architecture (Bidirectional LSTM):**
```
Input(50) → LSTM(128)  ←→ LSTM(128) [Layer 1, Bidirectional] →
           → Dropout(0.3) →
           → LSTM(128)  ←→ LSTM(128) [Layer 2, Bidirectional] →
           → Dense(10)
```

**Use Cases:**
- Sequence classification
- Time series prediction
- Natural language processing
- Speech recognition
- Video analysis

### SiameseTemplate - Similarity Learning

```python
# Python
template = dl.SiameseTemplate(
    input_dim=128,
    hidden_dims=[256, 128],
    embedding_dim=64,
    distance_metric="euclidean"  # or "cosine"
)
embedding_net = template.build_embedding_network()
```

**Architecture:**
```
Input(128) → Dense(256) → ReLU →
             Dense(128) → ReLU →
             Dense(64)  [Embedding]
```

**Usage Pattern:**
```python
# Train with pairs
embedding1 = model.predict(input1)
embedding2 = model.predict(input2)

# Compute similarity
if distance_metric == "euclidean":
    similarity = np.linalg.norm(embedding1 - embedding2)
elif distance_metric == "cosine":
    similarity = np.dot(embedding1, embedding2) / (norm1 * norm2)
```

**Use Cases:**
- Face verification
- Signature verification
- One-shot learning
- Similarity search
- Image retrieval

### GANTemplate - Generative Adversarial Network

```python
# Python
template = dl.GANTemplate(
    latent_dim=100,              # Noise dimension
    output_dim=784,              # Generated output size
    generator_dims=[256, 512],   # Generator hidden layers
    discriminator_dims=[512, 256] # Discriminator hidden layers
)

generator = template.build_generator()
discriminator = template.build_discriminator()
```

**Generator Architecture:**
```
Noise(100) → Dense(256) → ReLU →
             Dense(512) → ReLU →
             Dense(784) → Tanh  [-1, 1 output]
```

**Discriminator Architecture:**
```
Input(784) → Dense(512)  → ReLU → Dropout(0.3) →
             Dense(256)  → ReLU → Dropout(0.3) →
             Dense(1)    → Sigmoid  [Real/Fake probability]
```

**Training Loop:**
```python
# Train discriminator
real_output = discriminator.predict(real_data)
fake_data = generator.predict(noise)
fake_output = discriminator.predict(fake_data)

d_loss = -log(real_output) - log(1 - fake_output)
# Update discriminator

# Train generator  
fake_data = generator.predict(noise)
fake_output = discriminator.predict(fake_data)

g_loss = -log(fake_output)
# Update generator
```

**Use Cases:**
- Image generation
- Data augmentation
- Style transfer
- Super-resolution
- Text-to-image synthesis

## Quick Builder Functions

All quick builders return fully configured networks with:
✓ Appropriate loss function
✓ Optimizer (Adam by default)
✓ Ready to train

### binary_classifier()

```python
model = dl.binary_classifier(
    input_dim=10,
    hidden_dims=[64, 32]  # Optional, default=[64, 32]
)

# Pre-configured with:
# - Sigmoid output layer
# - Binary Cross-Entropy loss
# - Adam optimizer (lr=0.001)
```

### multiclass_classifier()

```python
model = dl.multiclass_classifier(
    input_dim=784,
    num_classes=10,
    hidden_dims=[128, 64]  # Optional, default=[128, 64]
)

# Pre-configured with:
# - Softmax output layer
# - Categorical Cross-Entropy loss
# - Adam optimizer (lr=0.001)
```

### regressor()

```python
model = dl.regressor(
    input_dim=20,
    output_dim=1,  # Optional, default=1
    hidden_dims=[64, 32]  # Optional, default=[64, 32]
)

# Pre-configured with:
# - Linear output layer
# - MSE loss
# - Adam optimizer (lr=0.001)
```

### image_classifier()

```python
model = dl.image_classifier(
    num_classes=10,
    channels=3,      # Optional, default=3
    height=32,       # Optional, default=32
    width=32,        # Optional, default=32
    arch='simple'    # Options: 'simple', 'lenet', 'vgg', 'resnet'
)

# Pre-configured with:
# - Softmax output layer
# - Categorical Cross-Entropy loss
# - Adam optimizer (lr=0.001)
```

### sequence_classifier()

```python
model = dl.sequence_classifier(
    input_dim=50,
    num_classes=5,
    hidden_dim=128,    # Optional, default=128
    num_layers=2       # Optional, default=2
)

# Pre-configured with:
# - LSTM cells
# - Softmax output layer
# - Categorical Cross-Entropy loss
# - Adam optimizer (lr=0.001)
```

### embedding_network()

```python
model = dl.embedding_network(
    input_dim=1000,
    embedding_dim=128,
    hidden_dims=[512, 256]  # Optional, default=[128, 64]
)

# No loss/optimizer (embedding network)
# Use for feature extraction or as component
```

### simple_autoencoder()

```python
model = dl.simple_autoencoder(
    input_dim=784,
    latent_dim=32,
    hidden_dims=[128, 64]  # Optional, default=[128, 64]
)

# Pre-configured with:
# - Sigmoid output (reconstruction in [0,1])
# - MSE loss
# - Adam optimizer (lr=0.001)
```

### variational_autoencoder()

```python
model = dl.variational_autoencoder(
    input_dim=784,
    latent_dim=16,
    encoder_dims=[256, 128]  # Optional, default=[256, 128]
)

# Pre-configured with:
# - Variational layers
# - MSE loss (should add KL divergence)
# - Adam optimizer (lr=0.001)
```

### simple_gan()

```python
generator = dl.simple_gan(
    latent_dim=100,
    output_dim=784,
    generator_dims=[128, 256],       # Optional, default=[128, 256]
    discriminator_dims=[256, 128]    # Optional, default=[256, 128]
)

# Returns generator network
# Build discriminator separately with GANTemplate
```

## Complete Examples

### Example 1: Binary Classification

```python
import ml_core.deep_learning as dl
import numpy as np

# Data
X_train = np.random.randn(1000, 10)
y_train = (X_train[:, 0] > 0).astype(float).reshape(-1, 1)

# Create model
model = dl.binary_classifier(input_dim=10)

# Prepare data as tensors
X_tensor = dl.Tensor([1000, 10])
y_tensor = dl.Tensor([1000, 1])
# ... fill tensors ...

# Train
for epoch in range(100):
    loss = model.train_step(X_tensor, y_tensor)
    if epoch % 10 == 0:
        print(f"Epoch {epoch}: Loss = {loss}")

# Predict
X_test_tensor = dl.Tensor([100, 10])
predictions = model.predict(X_test_tensor)
```

### Example 2: Image Classification with Different Architectures

```python
# Try different architectures
architectures = ['simple', 'lenet', 'vgg', 'resnet']

for arch in architectures:
    model = dl.image_classifier(
        num_classes=10,
        channels=3,
        height=32,
        width=32,
        arch=arch
    )
    print(f"Created {arch} model")
```

### Example 3: Custom MLP with Template

```python
# Fine-grained control with template
template = dl.MLPTemplate(
    input_dim=100,
    hidden_dims=[256, 128, 64],
    output_dim=10,
    activation="relu",
    dropout_rate=0.5,
    batch_norm=True
)

model = template.build()

# Set custom optimizer and loss
model.set_optimizer(dl.Adam(learning_rate=0.0001))
model.set_loss(dl.CategoricalCrossEntropyLoss())
```

### Example 4: Autoencoder for Dimensionality Reduction

```python
# Create autoencoder
ae_template = dl.AutoencoderTemplate(
    input_dim=784,
    encoder_dims=[512, 256, 128],
    latent_dim=32,
    variational=False
)

# Full autoencoder for training
autoencoder = ae_template.build()

# Train for reconstruction
# ... training loop ...

# Extract encoder for inference
encoder = ae_template.build_encoder()

# Use encoder to compress data
latent_representation = encoder.predict(input_tensor)
```

### Example 5: GAN Training Loop

```python
# Create GAN
gan_template = dl.GANTemplate(
    latent_dim=100,
    output_dim=784,
    generator_dims=[256, 512],
    discriminator_dims=[512, 256]
)

generator = gan_template.build_generator()
discriminator = gan_template.build_discriminator()

# Set optimizers
generator.set_optimizer(dl.Adam(0.0002))
discriminator.set_optimizer(dl.Adam(0.0002))

# Training loop
for epoch in range(epochs):
    # Train discriminator
    noise_tensor = dl.Tensor([batch_size, 100])
    # ... fill with random noise ...
    fake_images = generator.predict(noise_tensor)
    
    # ... discriminator training ...
    
    # Train generator
    # ... generator training ...
```

## Best Practices

### 1. Choosing Hidden Layer Sizes

```python
# Rule of thumb: 
# - Start with layers 2-3x larger than input
# - Gradually decrease size
# - End with bottleneck or expansion to output

# Good for classification
model = dl.multiclass_classifier(
    input_dim=100,
    num_classes=10,
    hidden_dims=[256, 128, 64]  # Gradual decrease
)

# Good for regression
model = dl.regressor(
    input_dim=50,
    output_dim=1,
    hidden_dims=[100, 50]  # 2x, 1x input size
)
```

### 2. Activation Functions

```python
# ReLU (default) - Best for most cases
template = dl.MLPTemplate(..., activation="relu")

# Tanh - For centered data [-1, 1]
template = dl.MLPTemplate(..., activation="tanh")

# Sigmoid - For binary features
template = dl.MLPTemplate(..., activation="sigmoid")
```

### 3. Regularization

```python
# Dropout prevents overfitting
template = dl.MLPTemplate(
    input_dim=100,
    hidden_dims=[256, 128],
    output_dim=10,
    dropout_rate=0.5  # Drop 50% during training
)

# Higher dropout for smaller datasets
# Lower dropout for larger datasets
```

### 4. Architecture Selection

```python
# Simple classification → binary_classifier() or multiclass_classifier()
# Tabular regression → regressor()
# Images → image_classifier() with appropriate arch
# Sequences → sequence_classifier()
# Dimensionality reduction → simple_autoencoder()
# Generative modeling → variational_autoencoder() or simple_gan()
```

### 5. Learning Rate

```python
# Start with defaults
model = dl.binary_classifier(input_dim=10)  # Uses Adam(0.001)

# Adjust if needed
model.set_optimizer(dl.Adam(learning_rate=0.0001))  # Lower for stability
model.set_optimizer(dl.Adam(learning_rate=0.01))    # Higher for faster convergence
```

## Comparison Table

| Template | Input | Output | Loss | Best For |
|----------|-------|--------|------|----------|
| `binary_classifier` | Features | 1 (sigmoid) | BCE | Binary classification |
| `multiclass_classifier` | Features | Classes (softmax) | CCE | Multi-class classification |
| `regressor` | Features | Continuous | MSE | Regression tasks |
| `image_classifier` | Images | Classes (softmax) | CCE | Computer vision |
| `sequence_classifier` | Sequences | Classes (softmax) | CCE | Time series, NLP |
| `embedding_network` | High-dim | Low-dim | None | Feature learning |
| `simple_autoencoder` | Features | Reconstruction | MSE | Compression, denoising |
| `variational_autoencoder` | Features | Reconstruction | MSE+KL | Generative modeling |
| `simple_gan` | Noise | Generated data | Adversarial | Image generation |

## Performance Tips

1. **Batch Size**: Larger batches = faster but more memory
2. **Hidden Layers**: More layers = more capacity but slower
3. **Dropout**: Prevents overfitting but increases training time
4. **Architecture**: Start simple, increase complexity as needed
5. **Pre-trained**: Use templates as starting point, fine-tune as needed

## See Also

- [Deep Learning Module](deep_learning/README.md)
- [Training Examples](examples/training_visualization.py)
- [Distributed Training](DISTRIBUTED_COMPUTING.md)
