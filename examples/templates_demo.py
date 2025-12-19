#!/usr/bin/env python3
"""
Neural Network Templates Examples

Demonstrates the easy-to-use neural network templates for common architectures.
"""

import sys
sys.path.insert(0, './build')
import ml_core
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

print("=" * 80)
print("NEURAL NETWORK TEMPLATES DEMO")
print("=" * 80)

# ============================================================================
# Example 1: Binary Classification
# ============================================================================
print("\n" + "=" * 80)
print("Example 1: Binary Classification with Template")
print("=" * 80)

# Generate binary classification data
np.random.seed(42)
n_samples = 200
X = np.random.randn(n_samples, 5)
y = ((X[:, 0] + X[:, 1] - X[:, 2]) > 0).astype(float)

print(f"Dataset: {n_samples} samples, 5 features")
print(f"Class distribution: {np.sum(y)}/{len(y)} positive")

# Create binary classifier using template
model = ml_core.deep_learning.binary_classifier(
    input_dim=5,
    hidden_dims=[32, 16]
)

print("\n✓ Binary classifier created with template")
print("  Architecture: 5 → 32 → 16 → 1 (sigmoid)")
print("  Loss: Binary Cross-Entropy")
print("  Optimizer: Adam")

# ============================================================================
# Example 2: Multi-class Classification
# ============================================================================
print("\n" + "=" * 80)
print("Example 2: Multi-class Classification with Template")
print("=" * 80)

# Generate multi-class data
n_classes = 4
X_multi = np.random.randn(300, 10)
y_multi = np.random.randint(0, n_classes, 300)

# One-hot encode
y_multi_onehot = np.eye(n_classes)[y_multi]

print(f"Dataset: 300 samples, 10 features, {n_classes} classes")

# Create multi-class classifier
model_multi = ml_core.deep_learning.multiclass_classifier(
    input_dim=10,
    num_classes=n_classes,
    hidden_dims=[64, 32]
)

print("\n✓ Multi-class classifier created")
print(f"  Architecture: 10 → 64 → 32 → {n_classes} (softmax)")
print("  Loss: Categorical Cross-Entropy")

# ============================================================================
# Example 3: Regression Network
# ============================================================================
print("\n" + "=" * 80)
print("Example 3: Regression with Template")
print("=" * 80)

# Generate regression data
X_reg = np.random.randn(150, 8)
y_reg = 2 * X_reg[:, 0] - X_reg[:, 1] + 0.5 * X_reg[:, 2] + np.random.randn(150) * 0.1

print("Dataset: 150 samples, 8 features → 1 output")

# Create regressor
model_reg = ml_core.deep_learning.regressor(
    input_dim=8,
    output_dim=1,
    hidden_dims=[32, 16]
)

print("\n✓ Regressor created")
print("  Architecture: 8 → 32 → 16 → 1")
print("  Loss: Mean Squared Error")

# ============================================================================
# Example 4: Image Classifier (CNN-style)
# ============================================================================
print("\n" + "=" * 80)
print("Example 4: Image Classification Templates")
print("=" * 80)

architectures = ['simple', 'lenet', 'vgg', 'resnet']

for arch in architectures:
    model_img = ml_core.deep_learning.image_classifier(
        num_classes=10,
        channels=3,
        height=32,
        width=32,
        arch=arch
    )
    print(f"  ✓ {arch.upper():10s} - Image classifier (3x32x32 → 10 classes)")

# ============================================================================
# Example 5: Autoencoder
# ============================================================================
print("\n" + "=" * 80)
print("Example 5: Autoencoder Templates")
print("=" * 80)

# Simple autoencoder
ae = ml_core.deep_learning.simple_autoencoder(
    input_dim=784,  # 28x28 image
    latent_dim=32,
    hidden_dims=[256, 128, 64]
)

print("✓ Simple Autoencoder created")
print("  Encoder: 784 → 256 → 128 → 64 → 32 (latent)")
print("  Decoder: 32 → 64 → 128 → 256 → 784 (reconstruction)")

# Variational autoencoder
vae = ml_core.deep_learning.variational_autoencoder(
    input_dim=784,
    latent_dim=16,
    encoder_dims=[256, 128]
)

print("\n✓ Variational Autoencoder created")
print("  Encoder: 784 → 256 → 128 → 16 (latent)")
print("  Decoder: 16 → 128 → 256 → 784 (reconstruction)")

# ============================================================================
# Example 6: Using Template Classes Directly
# ============================================================================
print("\n" + "=" * 80)
print("Example 6: Using Template Classes for Custom Configuration")
print("=" * 80)

# MLP with custom settings
mlp_template = ml_core.deep_learning.MLPTemplate(
    input_dim=20,
    hidden_dims=[128, 64, 32],
    output_dim=5,
    activation="relu",
    dropout_rate=0.5,
    batch_norm=False
)

mlp_custom = mlp_template.build()
print(f"✓ Custom MLP: {mlp_template.name()}")
print("  - Input: 20 features")
print("  - Hidden: [128, 64, 32] with ReLU + 50% dropout")
print("  - Output: 5 units")

# Autoencoder with custom architecture
ae_template = ml_core.deep_learning.AutoencoderTemplate(
    input_dim=100,
    encoder_dims=[80, 60, 40],
    latent_dim=10,
    variational=False
)

# Build full autoencoder
full_ae = ae_template.build()
print(f"\n✓ Custom {ae_template.name()}")
print("  - Encoder: 100 → 80 → 60 → 40 → 10")
print("  - Decoder: 10 → 40 → 60 → 80 → 100")

# Build encoder only
encoder_only = ae_template.build_encoder()
print("\n✓ Encoder extracted separately")

# Build decoder only
decoder_only = ae_template.build_decoder()
print("✓ Decoder extracted separately")

# RNN template
rnn_template = ml_core.deep_learning.RNNTemplate(
    input_dim=50,
    hidden_dim=128,
    num_layers=2,
    output_dim=10,
    cell_type=ml_core.deep_learning.RNNCellType.LSTM,
    bidirectional=True,
    dropout=0.3
)

rnn_model = rnn_template.build()
print(f"\n✓ Custom RNN: {rnn_template.name()}")
print("  - Type: Bidirectional LSTM")
print("  - Input: 50 features")
print("  - Hidden: 128 units x 2 layers")
print("  - Output: 10 units")
print("  - Dropout: 30% between layers")

# CNN template
cnn_template = ml_core.deep_learning.CNNTemplate(
    architecture=ml_core.deep_learning.CNNArchitecture.LENET,
    num_classes=10,
    input_channels=1,  # Grayscale
    input_height=28,
    input_width=28
)

cnn_model = cnn_template.build()
print(f"\n✓ Custom CNN: {cnn_template.name()}")
print("  - Architecture: LeNet-5 style")
print("  - Input: 1x28x28 (grayscale)")
print("  - Output: 10 classes")

# GAN template
gan_template = ml_core.deep_learning.GANTemplate(
    latent_dim=100,
    output_dim=784,  # 28x28 image
    generator_dims=[256, 512],
    discriminator_dims=[512, 256]
)

generator = gan_template.build_generator()
discriminator = gan_template.build_discriminator()

print(f"\n✓ {gan_template.name()} Components:")
print("  Generator: 100 (noise) → 256 → 512 → 784 (image)")
print("  Discriminator: 784 (image) → 512 → 256 → 1 (real/fake)")

# Siamese network for similarity learning
siamese_template = ml_core.deep_learning.SiameseTemplate(
    input_dim=128,
    hidden_dims=[256, 128],
    embedding_dim=64,
    distance_metric="euclidean"
)

siamese_model = siamese_template.build_embedding_network()
print(f"\n✓ {siamese_template.name()} Embedding Network:")
print("  - Input: 128 features")
print("  - Hidden: [256, 128]")
print("  - Embedding: 64 dimensions")
print("  - Distance: Euclidean")

# ============================================================================
# Example 7: Sequence Models
# ============================================================================
print("\n" + "=" * 80)
print("Example 7: Sequence Models")
print("=" * 80)

# Sequence classifier
seq_clf = ml_core.deep_learning.sequence_classifier(
    input_dim=25,
    num_classes=5,
    hidden_dim=64,
    num_layers=2
)

print("✓ Sequence Classifier (LSTM-based)")
print("  - Input: 25 features per timestep")
print("  - Hidden: 64 units, 2 layers")
print("  - Output: 5 classes")

# ============================================================================
# Example 8: Embedding Network
# ============================================================================
print("\n" + "=" * 80)
print("Example 8: Embedding Networks")
print("=" * 80)

embedding_net = ml_core.deep_learning.embedding_network(
    input_dim=1000,  # e.g., one-hot vocabulary
    embedding_dim=128,
    hidden_dims=[512, 256]
)

print("✓ Embedding Network")
print("  - Input: 1000 dimensions (e.g., vocabulary)")
print("  - Hidden: [512, 256]")
print("  - Embedding: 128 dimensions")

# ============================================================================
# Summary Visualization
# ============================================================================
print("\n" + "=" * 80)
print("Creating Summary Visualization...")
print("=" * 80)

fig, axes = plt.subplots(3, 3, figsize=(16, 12))
fig.suptitle('Neural Network Templates Overview', fontsize=16, fontweight='bold')

templates_info = [
    ("Binary Classifier", "5 → [32, 16] → 1\nSigmoid + BCE Loss", "Classification"),
    ("Multiclass Classifier", "10 → [64, 32] → 4\nSoftmax + CE Loss", "Classification"),
    ("Regressor", "8 → [32, 16] → 1\nMSE Loss", "Regression"),
    ("Simple CNN", "3x32x32 → Classes\nConv Layers", "Vision"),
    ("LeNet", "1x28x28 → 10\nClassic CNN", "Vision"),
    ("Autoencoder", "784 → 32 → 784\nReconstruction", "Generative"),
    ("VAE", "784 → 16 → 784\nVariational", "Generative"),
    ("LSTM Classifier", "50 → [128×2] → 10\nBidirectional", "Sequence"),
    ("GAN", "100 → 784\nGenerator + Discriminator", "Generative"),
]

for idx, (ax, (name, arch, category)) in enumerate(zip(axes.flat, templates_info)):
    ax.axis('off')
    
    # Color by category
    colors = {
        'Classification': '#3498db',
        'Regression': '#2ecc71',
        'Vision': '#e74c3c',
        'Generative': '#9b59b6',
        'Sequence': '#f39c12'
    }
    
    color = colors.get(category, '#95a5a6')
    
    # Draw box
    ax.add_patch(plt.Rectangle((0.1, 0.2), 0.8, 0.6, 
                               facecolor=color, alpha=0.2, 
                               edgecolor=color, linewidth=2))
    
    ax.text(0.5, 0.75, name, ha='center', va='center',
           fontsize=12, fontweight='bold', transform=ax.transAxes)
    
    ax.text(0.5, 0.5, arch, ha='center', va='center',
           fontsize=9, family='monospace', transform=ax.transAxes)
    
    ax.text(0.5, 0.15, category, ha='center', va='center',
           fontsize=8, style='italic', color=color,
           transform=ax.transAxes)
    
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)

plt.tight_layout()
plt.savefig('neural_network_templates.png', dpi=300, bbox_inches='tight')
print("✓ Saved: neural_network_templates.png")

# Create a detailed architecture comparison
fig, ax = plt.subplots(figsize=(14, 8))
ax.axis('off')

summary_text = """
NEURAL NETWORK TEMPLATES - QUICK REFERENCE
═══════════════════════════════════════════════════════════════════════

CLASSIFICATION
──────────────
binary_classifier(input_dim, hidden_dims=[64, 32])
    → Binary classification with sigmoid output
    
multiclass_classifier(input_dim, num_classes, hidden_dims=[128, 64])
    → Multi-class classification with softmax output

image_classifier(num_classes, channels=3, height=32, width=32, arch='simple')
    → Image classification (architectures: simple, lenet, vgg, resnet)

sequence_classifier(input_dim, num_classes, hidden_dim=128, num_layers=2)
    → Sequence classification with LSTM

REGRESSION
──────────
regressor(input_dim, output_dim=1, hidden_dims=[64, 32])
    → General regression with MSE loss

GENERATIVE MODELS
─────────────────
simple_autoencoder(input_dim, latent_dim, hidden_dims=[128, 64])
    → Dimensionality reduction and reconstruction

variational_autoencoder(input_dim, latent_dim, encoder_dims=[256, 128])
    → Variational autoencoder for generative modeling

simple_gan(latent_dim, output_dim, gen_dims=[128, 256], disc_dims=[256, 128])
    → Generative adversarial network

EMBEDDINGS
──────────
embedding_network(input_dim, embedding_dim, hidden_dims=[128, 64])
    → Learn low-dimensional representations

TEMPLATE CLASSES
────────────────
MLPTemplate(input_dim, hidden_dims, output_dim, activation, dropout, batch_norm)
CNNTemplate(architecture, num_classes, channels, height, width)
AutoencoderTemplate(input_dim, encoder_dims, latent_dim, variational)
RNNTemplate(input_dim, hidden_dim, num_layers, output_dim, cell_type, bidirectional)
SiameseTemplate(input_dim, hidden_dims, embedding_dim, distance_metric)
GANTemplate(latent_dim, output_dim, generator_dims, discriminator_dims)

USAGE
─────
# Quick creation
model = ml_core.deep_learning.binary_classifier(10)

# Custom configuration
template = ml_core.deep_learning.MLPTemplate(10, [64, 32], 1)
model = template.build()
model.set_optimizer(ml_core.deep_learning.Adam(0.001))

All templates come pre-configured with appropriate loss functions and optimizers!
"""

ax.text(0.05, 0.95, summary_text, 
        family='monospace', fontsize=9,
        verticalalignment='top',
        transform=ax.transAxes)

plt.tight_layout()
plt.savefig('templates_reference.png', dpi=300, bbox_inches='tight')
print("✓ Saved: templates_reference.png")

print("\n" + "=" * 80)
print("SUMMARY")
print("=" * 80)
print("""
Created and demonstrated neural network templates:

✓ Binary Classification     - Quick binary classifiers
✓ Multi-class Classification - Softmax-based classifiers  
✓ Regression                - MSE-based regressors
✓ Image Classification      - CNN architectures (Simple, LeNet, VGG, ResNet)
✓ Autoencoders             - Standard and variational
✓ Sequence Models          - LSTM/GRU-based networks
✓ Embedding Networks       - Dimensionality reduction
✓ GANs                     - Generator + Discriminator
✓ Siamese Networks         - Similarity learning

All templates include:
• Pre-configured architectures
• Appropriate loss functions
• Optimizers ready to train
• Easy customization

Use quick functions like binary_classifier() or template classes like
MLPTemplate() for full control!
""")
print("=" * 80)
