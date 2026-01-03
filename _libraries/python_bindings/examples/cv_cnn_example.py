#!/usr/bin/env python3
"""
Computer Vision Library - CNN Example

This example demonstrates building a simple CNN using the computer vision layers.
"""

import sys
sys.path.insert(0, '.')
from ml_core import deep_learning as dl
from ml_core import computer_vision as cv

def create_simple_image_batch(batch_size=4, height=28, width=28):
    """Create a batch of simple test images"""
    images = []
    for i in range(batch_size):
        img = cv.Image(height, width, cv.ImageFormat.GRAYSCALE)
        # Fill with some pattern
        for h in range(height):
            for w in range(width):
                val = (h * width + w) / (height * width) * 0.5 + i * 0.1
                img.at(h, w, 0, min(1.0, val))
        images.append(img)
    return images

def images_to_tensor_batch(images):
    """Convert list of images to a batched tensor"""
    if not images:
        return dl.Tensor([0])
    
    batch_size = len(images)
    channels = images[0].channels()
    height = images[0].height()
    width = images[0].width()
    
    # Create batch tensor [batch_size, channels, height, width]
    tensor = dl.Tensor([batch_size, channels, height, width])
    data = tensor.data()
    
    for b, img in enumerate(images):
        img_data = img.data()
        for c in range(channels):
            for h in range(height):
                for w in range(width):
                    idx = ((b * channels + c) * height + h) * width + w
                    img_idx = (h * width + w) * channels + c
                    data[idx] = img_data[img_idx]
    
    return tensor

def test_conv_layers():
    """Test convolutional layers"""
    print("=" * 70)
    print("Testing Convolutional Layers")
    print("=" * 70)
    
    # Create a simple input (batch of images)
    images = create_simple_image_batch(2, 28, 28)
    print(f"\nCreated {len(images)} images of size {images[0].height()}x{images[0].width()}")
    
    # Convert to tensor
    input_tensor = images_to_tensor_batch(images)
    print(f"Input tensor shape: {input_tensor.shape()}")
    
    # Create Conv2D layer
    conv = cv.Conv2DLayer(
        in_channels=1,
        out_channels=16,
        kernel_size=3,
        stride=1,
        padding=1
    )
    
    print(f"\nConv2D Layer:")
    print(f"  Input channels: {conv.in_channels()}")
    print(f"  Output channels: {conv.out_channels()}")
    print(f"  Kernel size: {conv.kernel_size()}")
    print(f"  Stride: {conv.stride()}")
    print(f"  Padding: {conv.padding()}")
    
    # Forward pass
    output = conv.forward(input_tensor)
    print(f"  Output shape: {output.shape()}")
    
    # Test MaxPool2D
    maxpool = cv.MaxPool2DLayer(kernel_size=2, stride=2)
    pooled = maxpool.forward(output)
    print(f"\nMaxPool2D output shape: {pooled.shape()}")
    
    # Test AvgPool2D
    avgpool = cv.AvgPool2DLayer(kernel_size=2, stride=2)
    avg_pooled = avgpool.forward(output)
    print(f"AvgPool2D output shape: {avg_pooled.shape()}")

def test_simple_cnn():
    """Build and test a simple CNN architecture"""
    print("\n" + "=" * 70)
    print("Testing Simple CNN Architecture")
    print("=" * 70)
    
    # Create input
    images = create_simple_image_batch(1, 28, 28)
    input_tensor = images_to_tensor_batch(images)
    
    print(f"\nBuilding CNN:")
    print(f"  Input: 1x28x28")
    
    # Build a simple CNN
    # Conv -> ReLU -> MaxPool -> Conv -> ReLU -> MaxPool -> Flatten -> Dense
    
    conv1 = cv.Conv2DLayer(1, 8, 3, stride=1, padding=1)
    relu1 = dl.ReLULayer()
    pool1 = cv.MaxPool2DLayer(2, 2)
    
    conv2 = cv.Conv2DLayer(8, 16, 3, stride=1, padding=1)
    relu2 = dl.ReLULayer()
    pool2 = cv.MaxPool2DLayer(2, 2)
    
    flatten = cv.FlattenLayer()
    dense = dl.DenseLayer(784, 10)  # 16 * 7 * 7 = 784
    
    print("  Layer 1: Conv2D(1 -> 8, kernel=3, padding=1)")
    x = conv1.forward(input_tensor)
    print(f"    Output: {x.shape()}")
    
    print("  Layer 2: ReLU")
    x = relu1.forward(x)
    print(f"    Output: {x.shape()}")
    
    print("  Layer 3: MaxPool2D(kernel=2, stride=2)")
    x = pool1.forward(x)
    print(f"    Output: {x.shape()}")
    
    print("  Layer 4: Conv2D(8 -> 16, kernel=3, padding=1)")
    x = conv2.forward(x)
    print(f"    Output: {x.shape()}")
    
    print("  Layer 5: ReLU")
    x = relu2.forward(x)
    print(f"    Output: {x.shape()}")
    
    print("  Layer 6: MaxPool2D(kernel=2, stride=2)")
    x = pool2.forward(x)
    print(f"    Output: {x.shape()}")
    
    print("  Layer 7: Flatten")
    x = flatten.forward(x)
    print(f"    Output: {x.shape()}")
    
    print("  Layer 8: Dense(784 -> 10)")
    x = dense.forward(x)
    print(f"    Output: {x.shape()}")
    
    print("\n✓ CNN forward pass successful!")

def test_global_pooling():
    """Test global average pooling"""
    print("\n" + "=" * 70)
    print("Testing Global Average Pooling")
    print("=" * 70)
    
    images = create_simple_image_batch(2, 32, 32)
    input_tensor = images_to_tensor_batch(images)
    
    # Apply convolution to get feature maps
    conv = cv.Conv2DLayer(1, 64, 3, stride=1, padding=1)
    features = conv.forward(input_tensor)
    print(f"\nFeature maps shape: {features.shape()}")
    
    # Global average pooling
    gap = cv.GlobalAvgPool2DLayer()
    pooled = gap.forward(features)
    print(f"After Global Avg Pool: {pooled.shape()}")
    print("  (Reduced spatial dimensions to 1x1)")

def test_image_tensor_conversion():
    """Test conversion between images and tensors"""
    print("\n" + "=" * 70)
    print("Testing Image-Tensor Conversion")
    print("=" * 70)
    
    # Create an image
    img = cv.Image(32, 32, cv.ImageFormat.RGB)
    for i in range(32):
        for j in range(32):
            img.at(i, j, 0, i / 32.0)
            img.at(i, j, 1, j / 32.0)
            img.at(i, j, 2, 0.5)
    
    print(f"\nOriginal image: {img.height()}x{img.width()}x{img.channels()}")
    mean_before = img.mean()
    print(f"  Mean: R={mean_before[0]:.3f}, G={mean_before[1]:.3f}, B={mean_before[2]:.3f}")
    
    # Convert to tensor
    tensor = cv.image_to_tensor(img)
    print(f"\nTensor shape: {tensor.shape()}")
    
    # Convert back to image
    img_restored = cv.tensor_to_image(tensor, cv.ImageFormat.RGB)
    print(f"\nRestored image: {img_restored.height()}x{img_restored.width()}x{img_restored.channels()}")
    mean_after = img_restored.mean()
    print(f"  Mean: R={mean_after[0]:.3f}, G={mean_after[1]:.3f}, B={mean_after[2]:.3f}")
    
    # Check if values are preserved
    diff = sum(abs(a - b) for a, b in zip(mean_before, mean_after))
    print(f"\nMean absolute difference: {diff:.6f}")
    if diff < 1e-5:
        print("✓ Conversion is lossless!")

def main():
    print("\n" + "=" * 70)
    print("Computer Vision Library - CNN Demo")
    print("=" * 70)
    
    test_conv_layers()
    test_simple_cnn()
    test_global_pooling()
    test_image_tensor_conversion()
    
    print("\n" + "=" * 70)
    print("All CNN tests completed successfully!")
    print("=" * 70)

if __name__ == "__main__":
    main()
