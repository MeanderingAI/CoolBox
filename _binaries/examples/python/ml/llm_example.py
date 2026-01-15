#!/usr/bin/env python3
"""
Large Language Model (LLM) Template Example

This example demonstrates how to create and configure GPT-style language models
using the LLM template system.
"""

import sys
import os

# Add the build directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'build'))

try:
    import ml_core
    from ml_core import deep_learning as dl
except ImportError as e:
    print(f"Error importing ml_core: {e}")
    print("\nPlease build the Python bindings first:")
    print("  cd python_bindings && ./build.sh")
    sys.exit(1)

import numpy as np


def print_separator(title=""):
    """Print a formatted separator."""
    if title:
        print(f"\n{'='*70}")
        print(f"  {title}")
        print(f"{'='*70}\n")
    else:
        print(f"\n{'-'*70}\n")


def create_tiny_llm():
    """Create a tiny language model for demonstration."""
    print_separator("1. Tiny Language Model (Demo Size)")
    
    # Very small model for quick demonstration
    vocab_size = 1000
    context_length = 128
    embed_dim = 128
    num_heads = 4
    num_layers = 2
    ff_dim = 512
    
    print(f"Configuration:")
    print(f"  Vocabulary Size:  {vocab_size:,}")
    print(f"  Context Length:   {context_length}")
    print(f"  Embedding Dim:    {embed_dim}")
    print(f"  Attention Heads:  {num_heads}")
    print(f"  Transformer Layers: {num_layers}")
    print(f"  Feed-Forward Dim: {ff_dim}")
    
    # Create using template class
    llm_template = dl.LLMTemplate(
        vocab_size=vocab_size,
        context_length=context_length,
        embed_dim=embed_dim,
        num_heads=num_heads,
        num_layers=num_layers,
        ff_dim=ff_dim,
        dropout=0.1,
        causal=True
    )
    
    model = llm_template.build()
    print(f"\n✓ Model created successfully!")
    print(f"  Template name: {llm_template.name()}")
    
    # Estimate parameters
    params = estimate_parameters(vocab_size, embed_dim, num_heads, num_layers, ff_dim)
    print(f"  Estimated parameters: ~{params:,}")
    
    return model


def create_gpt2_small():
    """Create a GPT-2 small-sized model."""
    print_separator("2. GPT-2 Small Configuration")
    
    # GPT-2 small configuration
    vocab_size = 50257
    context_length = 1024
    embed_dim = 768
    num_heads = 12
    num_layers = 12
    ff_dim = 3072  # 4 * embed_dim
    
    print(f"Configuration (matches GPT-2 small):")
    print(f"  Vocabulary Size:  {vocab_size:,}")
    print(f"  Context Length:   {context_length:,}")
    print(f"  Embedding Dim:    {embed_dim}")
    print(f"  Attention Heads:  {num_heads}")
    print(f"  Transformer Layers: {num_layers}")
    print(f"  Feed-Forward Dim: {ff_dim:,}")
    
    # Using quick builder function
    model = dl.language_model(
        vocab_size=vocab_size,
        context_length=context_length,
        embed_dim=embed_dim,
        num_heads=num_heads,
        num_layers=num_layers,
        ff_dim=ff_dim
    )
    
    params = estimate_parameters(vocab_size, embed_dim, num_heads, num_layers, ff_dim)
    print(f"\n✓ GPT-2 small model created!")
    print(f"  Estimated parameters: ~{params:,} ({params/1e6:.1f}M)")
    
    return model


def create_custom_llm():
    """Create a custom-sized language model."""
    print_separator("3. Custom Language Model")
    
    # Medium-sized custom configuration
    vocab_size = 32000  # Common for many tokenizers
    context_length = 2048
    embed_dim = 512
    num_heads = 8
    num_layers = 8
    ff_dim = 2048
    
    print(f"Custom Configuration:")
    print(f"  Vocabulary Size:  {vocab_size:,}")
    print(f"  Context Length:   {context_length:,}")
    print(f"  Embedding Dim:    {embed_dim}")
    print(f"  Attention Heads:  {num_heads}")
    print(f"  Transformer Layers: {num_layers}")
    print(f"  Feed-Forward Dim: {ff_dim:,}")
    
    model = dl.language_model(
        vocab_size=vocab_size,
        context_length=context_length,
        embed_dim=embed_dim,
        num_heads=num_heads,
        num_layers=num_layers,
        ff_dim=ff_dim
    )
    
    params = estimate_parameters(vocab_size, embed_dim, num_heads, num_layers, ff_dim)
    print(f"\n✓ Custom model created!")
    print(f"  Estimated parameters: ~{params:,} ({params/1e6:.1f}M)")
    
    return model


def estimate_parameters(vocab_size, embed_dim, num_heads, num_layers, ff_dim):
    """Estimate the number of parameters in the LLM."""
    # Token embeddings
    token_embed = vocab_size * embed_dim
    
    # Positional embeddings (if learned)
    pos_embed = 0  # Can add if using learned positional embeddings
    
    # Per transformer layer:
    # - Self-attention: 4 * (embed_dim * embed_dim) for Q, K, V, O projections
    # - FFN: 2 * (embed_dim * ff_dim)
    # - Layer norms: negligible
    per_layer = 4 * (embed_dim * embed_dim) + 2 * (embed_dim * ff_dim)
    transformer_params = num_layers * per_layer
    
    # Output projection (language modeling head)
    output_proj = embed_dim * vocab_size
    
    total = token_embed + pos_embed + transformer_params + output_proj
    return int(total)


def compare_llm_sizes():
    """Compare different LLM sizes."""
    print_separator("4. Comparing LLM Sizes")
    
    configs = [
        ("Tiny (Demo)", 1000, 128, 4, 2, 512),
        ("Small", 10000, 256, 4, 4, 1024),
        ("Medium", 32000, 512, 8, 8, 2048),
        ("GPT-2 Small", 50257, 768, 12, 12, 3072),
        ("GPT-2 Medium", 50257, 1024, 16, 24, 4096),
        ("GPT-2 Large", 50257, 1280, 20, 36, 5120),
    ]
    
    print(f"{'Model':<20} {'Params':<15} {'Embed':<8} {'Layers':<8} {'Heads':<8}")
    print("-" * 70)
    
    for name, vocab, embed, heads, layers, ff in configs:
        params = estimate_parameters(vocab, embed, heads, layers, ff)
        if params >= 1e9:
            param_str = f"{params/1e9:.2f}B"
        elif params >= 1e6:
            param_str = f"{params/1e6:.1f}M"
        elif params >= 1e3:
            param_str = f"{params/1e3:.1f}K"
        else:
            param_str = f"{params}"
        
        print(f"{name:<20} {param_str:<15} {embed:<8} {layers:<8} {heads:<8}")


def demonstrate_llm_usage():
    """Demonstrate how to use the LLM templates."""
    print_separator("5. LLM Usage Examples")
    
    print("Creating models with different approaches:\n")
    
    # Method 1: Using LLMTemplate class
    print("Method 1: LLMTemplate class")
    template = dl.LLMTemplate(
        vocab_size=10000,
        context_length=512,
        embed_dim=256,
        num_heads=4,
        num_layers=4,
        ff_dim=1024
    )
    model1 = template.build()
    print(f"  ✓ Created via template class")
    print(f"    Embed dim: {template.embed_dim()}")
    print(f"    Context length: {template.context_length()}")
    
    # Method 2: Using quick builder function
    print("\nMethod 2: language_model() quick builder")
    model2 = dl.language_model(
        vocab_size=10000,
        context_length=512,
        embed_dim=256,
        num_heads=4,
        num_layers=4,
        ff_dim=1024
    )
    print(f"  ✓ Created via quick builder (includes loss & optimizer)")


def main():
    """Main demonstration function."""
    print("\n" + "="*70)
    print("  LARGE LANGUAGE MODEL (LLM) TEMPLATE EXAMPLES")
    print("="*70)
    print("\nThis example shows how to create GPT-style language models")
    print("using the neural network template system.")
    
    try:
        # Create different sized models
        tiny_model = create_tiny_llm()
        gpt2_model = create_gpt2_small()
        custom_model = create_custom_llm()
        
        # Compare sizes
        compare_llm_sizes()
        
        # Demonstrate usage
        demonstrate_llm_usage()
        
        print_separator("Summary")
        print("✓ Successfully demonstrated LLM template creation")
        print("\nKey Features:")
        print("  • GPT-style decoder-only transformer architecture")
        print("  • Causal (autoregressive) self-attention masking")
        print("  • Token + positional embeddings")
        print("  • Configurable model size (vocab, layers, heads, etc.)")
        print("  • Pre-configured with cross-entropy loss and Adam optimizer")
        print("\nUse Cases:")
        print("  • Text generation")
        print("  • Language modeling")
        print("  • Next-token prediction")
        print("  • Fine-tuning for downstream tasks")
        
        print("\nNote: This creates the model architecture. For actual training,")
        print("you would need:")
        print("  1. Tokenized text data")
        print("  2. Training loop with batching")
        print("  3. GPU acceleration for larger models")
        print("  4. Proper regularization and learning rate scheduling")
        
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    print("\n" + "="*70 + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
