# Neural Network Templates - Quick Reference

## One-Line Network Creation

```python
import ml_core.deep_learning as dl

# Binary Classification
model = dl.binary_classifier(input_dim=10, hidden_dims=[64, 32])

# Multi-class Classification
model = dl.multiclass_classifier(input_dim=784, num_classes=10)

# Regression
model = dl.regressor(input_dim=20, output_dim=1)

# Image Classification
model = dl.image_classifier(num_classes=10, arch='lenet')

# Sequence Classification
model = dl.sequence_classifier(input_dim=50, num_classes=5)

# Autoencoder
model = dl.simple_autoencoder(input_dim=784, latent_dim=32)

# Variational Autoencoder
model = dl.variational_autoencoder(input_dim=784, latent_dim=16)

# Embedding Network
model = dl.embedding_network(input_dim=1000, embedding_dim=128)

# GAN Generator
generator = dl.simple_gan(latent_dim=100, output_dim=784)
```

## Template Classes (Advanced)

```python
# MLP with custom configuration
template = dl.MLPTemplate(
    input_dim=100,
    hidden_dims=[256, 128, 64],
    output_dim=10,
    activation="relu",
    dropout_rate=0.5
)
model = template.build()

# CNN with specific architecture
template = dl.CNNTemplate(
    architecture=dl.CNNArchitecture.LENET,
    num_classes=10,
    input_channels=1,
    input_height=28,
    input_width=28
)
model = template.build()

# Autoencoder with separate encoder/decoder
template = dl.AutoencoderTemplate(
    input_dim=784,
    encoder_dims=[256, 128],
    latent_dim=32
)
encoder = template.build_encoder()
decoder = template.build_decoder()
full_ae = template.build()

# RNN/LSTM
template = dl.RNNTemplate(
    input_dim=50,
    hidden_dim=128,
    num_layers=2,
    output_dim=10,
    cell_type=dl.RNNCellType.LSTM,
    bidirectional=True
)
model = template.build()

# Siamese Network
template = dl.SiameseTemplate(
    input_dim=128,
    hidden_dims=[256, 128],
    embedding_dim=64
)
embedding_net = template.build_embedding_network()

# GAN with custom architecture
template = dl.GANTemplate(
    latent_dim=100,
    output_dim=784,
    generator_dims=[256, 512],
    discriminator_dims=[512, 256]
)
generator = template.build_generator()
discriminator = template.build_discriminator()
```

## Available Architectures

### Image Classifiers
- `'simple'` - Basic 2-layer CNN
- `'lenet'` - LeNet-5 style
- `'vgg'` - VGG-like deep network
- `'resnet'` - ResNet with skip connections

### RNN Cell Types
- `RNNCellType.VANILLA` - Simple RNN
- `RNNCellType.LSTM` - Long Short-Term Memory
- `RNNCellType.GRU` - Gated Recurrent Unit

## Common Patterns

### Classification Task
```python
# 1. Create model
model = dl.multiclass_classifier(input_dim=784, num_classes=10)

# 2. Prepare data as tensors
X_tensor = dl.Tensor([1000, 784])
y_tensor = dl.Tensor([1000, 10])

# 3. Train
for epoch in range(100):
    loss = model.train_step(X_tensor, y_tensor)
```

### Regression Task
```python
model = dl.regressor(input_dim=10, output_dim=1)
# ... training ...
```

### Dimensionality Reduction
```python
# Train autoencoder
ae = dl.simple_autoencoder(input_dim=784, latent_dim=32)
# ... train ...

# Extract encoder for compression
template = dl.AutoencoderTemplate(784, [256, 128], 32)
encoder = template.build_encoder()
compressed = encoder.predict(data)
```

### Similarity Learning
```python
template = dl.SiameseTemplate(
    input_dim=128,
    hidden_dims=[256],
    embedding_dim=64
)
embedding_net = template.build_embedding_network()

# Get embeddings
emb1 = embedding_net.predict(input1)
emb2 = embedding_net.predict(input2)

# Compute similarity
distance = np.linalg.norm(emb1 - emb2)
```

## All Pre-configured Features

✅ **Loss Functions**: BCE, CCE, MSE (automatically selected)
✅ **Optimizers**: Adam with lr=0.001 (can be changed)
✅ **Activations**: ReLU, Tanh, Sigmoid (configurable)
✅ **Regularization**: Dropout, Batch Norm (optional)
✅ **Ready to Train**: No additional setup needed

## C++ Usage

```cpp
#include "deep_learning/templates.h"

using namespace deep_learning::templates;

// Quick builders
auto model1 = binary_classifier(10, {64, 32});
auto model2 = multiclass_classifier(784, 10, {128, 64});
auto model3 = regressor(20, 1, {64, 32});
auto model4 = image_classifier(10, 3, 32, 32, "lenet");

// Template classes
MLPTemplate mlp_template(10, {64, 32}, 1, "relu", 0.5);
auto mlp = mlp_template.build();

CNNTemplate cnn_template(CNNTemplate::Architecture::LENET, 10, 1, 28, 28);
auto cnn = cnn_template.build();

AutoencoderTemplate ae_template(784, {256, 128}, 32, false);
auto autoencoder = ae_template.build();
auto encoder = ae_template.build_encoder();

RNNTemplate rnn_template(50, 128, 2, 10, RNNTemplate::CellType::LSTM, true);
auto rnn = rnn_template.build();
```

## Template Summary

| Function | Pre-configured | Customizable |
|----------|---------------|--------------|
| `binary_classifier` | ✓ | Hidden dims |
| `multiclass_classifier` | ✓ | Hidden dims |
| `regressor` | ✓ | Hidden dims, output dim |
| `image_classifier` | ✓ | Architecture, image size |
| `sequence_classifier` | ✓ | Hidden dim, layers |
| `embedding_network` | Partial | All dimensions |
| `simple_autoencoder` | ✓ | Hidden dims, latent dim |
| `variational_autoencoder` | ✓ | Encoder dims, latent dim |
| `simple_gan` | Partial | Generator/discriminator dims |

---

For detailed documentation, see [NEURAL_NETWORK_TEMPLATES.md](NEURAL_NETWORK_TEMPLATES.md)
