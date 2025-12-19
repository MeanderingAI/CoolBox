#!/usr/bin/env python3
"""
Computer Vision Library - Image Transformations Example

This example demonstrates the computer vision module with various
image transformations and pipelines.
"""

import sys
sys.path.insert(0, '.')
from ml_core import computer_vision as cv
import random

def create_sample_image(height=64, width=64):
    """Create a simple test image with a gradient pattern"""
    img = cv.Image(height, width, cv.ImageFormat.RGB)
    
    # Create a gradient pattern
    for i in range(height):
        for j in range(width):
            r = i / height
            g = j / width
            b = 0.5
            img.at(i, j, 0, r)
            img.at(i, j, 1, g)
            img.at(i, j, 2, b)
    
    return img

def print_image_info(img, name="Image"):
    """Print basic image information"""
    print(f"\n{name}:")
    print(f"  Size: {img.height()}x{img.width()}")
    print(f"  Channels: {img.channels()}")
    print(f"  Format: {img.format()}")
    mean_vals = img.mean()
    std_vals = img.std()
    min_val, max_val = img.min_max()
    print(f"  Mean: {[f'{m:.3f}' for m in mean_vals]}")
    print(f"  Std: {[f'{s:.3f}' for s in std_vals]}")
    print(f"  Range: [{min_val:.3f}, {max_val:.3f}]")

def test_basic_transforms():
    """Test basic image transformations"""
    print("=" * 70)
    print("Testing Basic Image Transformations")
    print("=" * 70)
    
    # Create test image
    img = create_sample_image(64, 64)
    print_image_info(img, "Original Image")
    
    # Test Resize
    resize = cv.Resize(32, 32, cv.InterpolationMode.BILINEAR)
    resized = resize.apply(img)
    print_image_info(resized, "Resized (32x32)")
    
    # Test CenterCrop
    crop = cv.CenterCrop(40, 40)
    cropped = crop.apply(img)
    print_image_info(cropped, "Center Cropped (40x40)")
    
    # Test HorizontalFlip
    flip = cv.HorizontalFlip()
    flipped = flip.apply(img)
    print_image_info(flipped, "Horizontally Flipped")
    
    # Test Rotate
    rotate = cv.Rotate(45.0)
    rotated = rotate.apply(img)
    print_image_info(rotated, "Rotated (45 degrees)")
    
    # Test Normalize
    normalize = cv.Normalize([0.5, 0.5, 0.5], [0.5, 0.5, 0.5])
    normalized = normalize.apply(img)
    print_image_info(normalized, "Normalized")
    
    # Test Brightness adjustment
    brighten = cv.AdjustBrightness(1.5)
    brightened = brighten.apply(img)
    print_image_info(brightened, "Brightened (1.5x)")
    
    # Test Contrast adjustment
    contrast = cv.AdjustContrast(1.5)
    contrasted = contrast.apply(img)
    print_image_info(contrasted, "Contrast Adjusted (1.5x)")
    
    # Test Gaussian Blur
    blur = cv.GaussianBlur(5, 1.0)
    blurred = blur.apply(img)
    print_image_info(blurred, "Gaussian Blurred (5x5, sigma=1.0)")
    
    # Test Padding
    pad = cv.Pad(10, 10, 10, 10, 0.0)
    padded = pad.apply(img)
    print_image_info(padded, "Padded (10px on all sides)")

def test_pipelines():
    """Test transformation pipelines"""
    print("\n" + "=" * 70)
    print("Testing Transformation Pipelines")
    print("=" * 70)
    
    # Create test image
    img = create_sample_image(128, 128)
    print_image_info(img, "Original Image")
    
    # Create a custom pipeline
    pipeline = cv.TransformPipeline()
    pipeline.add(cv.Resize(96, 96))
    pipeline.add(cv.CenterCrop(64, 64))
    pipeline.add(cv.HorizontalFlip())
    pipeline.add(cv.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225]))
    
    print(f"\nPipeline has {pipeline.size()} transforms")
    
    # Apply pipeline
    result = pipeline.apply(img)
    print_image_info(result, "After Pipeline")
    
    # Test predefined ImageNet pipeline
    imagenet_pipeline = cv.create_imagenet_pipeline(224)
    img_large = create_sample_image(256, 256)
    imagenet_result = imagenet_pipeline.apply(img_large)
    print_image_info(imagenet_result, "After ImageNet Pipeline")
    
    # Test inference pipeline
    inference_pipeline = cv.create_inference_pipeline(128)
    inference_result = inference_pipeline.apply(img)
    print_image_info(inference_result, "After Inference Pipeline")

def test_batch_processing():
    """Test batch image processing"""
    print("\n" + "=" * 70)
    print("Testing Batch Image Processing")
    print("=" * 70)
    
    # Create batch of images
    batch = []
    for i in range(5):
        img = create_sample_image(64, 64)
        batch.append(img)
    
    print(f"\nCreated batch of {len(batch)} images")
    
    # Create pipeline
    pipeline = cv.TransformPipeline()
    pipeline.add(cv.Resize(32, 32))
    pipeline.add(cv.Standardize())
    
    # Apply to batch
    results = pipeline.apply_batch(batch)
    
    print(f"Processed {len(results)} images")
    for i, result in enumerate(results):
        print(f"  Image {i}: {result.height()}x{result.width()}, "
              f"mean={result.mean()[0]:.3f}, std={result.std()[0]:.3f}")

def test_format_conversion():
    """Test image format conversions"""
    print("\n" + "=" * 70)
    print("Testing Format Conversions")
    print("=" * 70)
    
    # Create RGB image
    rgb_img = create_sample_image(64, 64)
    print_image_info(rgb_img, "RGB Image")
    
    # Convert to grayscale
    gray_img = rgb_img.to_grayscale()
    print_image_info(gray_img, "Grayscale Image")
    
    # Convert back to RGB
    rgb_again = gray_img.to_rgb()
    print_image_info(rgb_again, "RGB from Grayscale")

def test_random_transforms():
    """Test random transformations"""
    print("\n" + "=" * 70)
    print("Testing Random Transformations")
    print("=" * 70)
    
    img = create_sample_image(128, 128)
    
    # Random crop
    random_crop = cv.RandomCrop(64, 64, 42)  # Fixed seed for reproducibility
    cropped1 = random_crop.apply(img)
    cropped2 = random_crop.apply(img)
    print_image_info(cropped1, "Random Crop 1")
    print_image_info(cropped2, "Random Crop 2")
    
    # Random horizontal flip
    random_flip = cv.RandomHorizontalFlip(0.5, 42)
    for i in range(5):
        result = random_flip.apply(img)
        print(f"  Random flip {i}: Applied" if result.data() != img.data() else f"  Random flip {i}: Skipped")
    
    # Random rotation
    random_rot = cv.RandomRotation(-30.0, 30.0, cv.InterpolationMode.BILINEAR, 42)
    rotated = random_rot.apply(img)
    print_image_info(rotated, "Randomly Rotated")

def main():
    print("\n" + "=" * 70)
    print("Computer Vision Library - Transformations Demo")
    print("=" * 70)
    
    test_basic_transforms()
    test_pipelines()
    test_batch_processing()
    test_format_conversion()
    test_random_transforms()
    
    print("\n" + "=" * 70)
    print("All tests completed successfully!")
    print("=" * 70)

if __name__ == "__main__":
    main()
