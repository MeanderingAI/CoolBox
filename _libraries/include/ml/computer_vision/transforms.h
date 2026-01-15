#ifndef COMPUTER_VISION_TRANSFORMS_H
#define COMPUTER_VISION_TRANSFORMS_H

#include "image.h"
#include <memory>
#include <random>

namespace ml {
namespace cv {

// Base transform class
class Transform {
public:
    virtual ~Transform() = default;
    virtual Image apply(const Image& image) const = 0;
    virtual std::unique_ptr<Transform> clone() const = 0;
};

// Resize transform
class Resize : public Transform {
public:
    Resize(int height, int width, InterpolationMode mode = InterpolationMode::BILINEAR);
    Image apply(const Image& image) const override;
    std::unique_ptr<Transform> clone() const override;
    
private:
    int height_;
    int width_;
    InterpolationMode mode_;
};

// Center crop
class CenterCrop : public Transform {
public:
    CenterCrop(int height, int width);
    Image apply(const Image& image) const override;
    std::unique_ptr<Transform> clone() const override;
    
private:
    int height_;
    int width_;
};

// Random crop
class RandomCrop : public Transform {
public:
    RandomCrop(int height, int width, unsigned int seed = 0);
    Image apply(const Image& image) const override;
    std::unique_ptr<Transform> clone() const override;
    
private:
    int height_;
    int width_;
    mutable std::mt19937 rng_;
};

// Horizontal flip
class HorizontalFlip : public Transform {
public:
    HorizontalFlip() = default;
    Image apply(const Image& image) const override;
    std::unique_ptr<Transform> clone() const override;
};

// Vertical flip
class VerticalFlip : public Transform {
public:
    VerticalFlip() = default;
    Image apply(const Image& image) const override;
    std::unique_ptr<Transform> clone() const override;
};

// Random horizontal flip
class RandomHorizontalFlip : public Transform {
public:
    RandomHorizontalFlip(float probability = 0.5f, unsigned int seed = 0);
    Image apply(const Image& image) const override;
    std::unique_ptr<Transform> clone() const override;
    
private:
    float probability_;
    mutable std::mt19937 rng_;
    mutable std::uniform_real_distribution<float> dist_;
};

// Normalize (subtract mean, divide by std)
class Normalize : public Transform {
public:
    Normalize(const std::vector<float>& mean, const std::vector<float>& std);
    Image apply(const Image& image) const override;
    std::unique_ptr<Transform> clone() const override;
    
private:
    std::vector<float> mean_;
    std::vector<float> std_;
};

// Standardize (zero mean, unit variance)
class Standardize : public Transform {
public:
    Standardize() = default;
    Image apply(const Image& image) const override;
    std::unique_ptr<Transform> clone() const override;
};

// Rotate
class Rotate : public Transform {
public:
    Rotate(float angle_degrees, InterpolationMode mode = InterpolationMode::BILINEAR);
    Image apply(const Image& image) const override;
    std::unique_ptr<Transform> clone() const override;
    
private:
    float angle_degrees_;
    InterpolationMode mode_;
};

// Random rotation
class RandomRotation : public Transform {
public:
    RandomRotation(float min_angle, float max_angle, 
                   InterpolationMode mode = InterpolationMode::BILINEAR,
                   unsigned int seed = 0);
    Image apply(const Image& image) const override;
    std::unique_ptr<Transform> clone() const override;
    
private:
    float min_angle_;
    float max_angle_;
    InterpolationMode mode_;
    mutable std::mt19937 rng_;
    mutable std::uniform_real_distribution<float> dist_;
};

// Brightness adjustment
class AdjustBrightness : public Transform {
public:
    AdjustBrightness(float factor);  // factor > 1 brightens, < 1 darkens
    Image apply(const Image& image) const override;
    std::unique_ptr<Transform> clone() const override;
    
private:
    float factor_;
};

// Contrast adjustment
class AdjustContrast : public Transform {
public:
    AdjustContrast(float factor);  // factor > 1 increases contrast
    Image apply(const Image& image) const override;
    std::unique_ptr<Transform> clone() const override;
    
private:
    float factor_;
};

// Gaussian blur
class GaussianBlur : public Transform {
public:
    GaussianBlur(int kernel_size, float sigma);
    Image apply(const Image& image) const override;
    std::unique_ptr<Transform> clone() const override;
    
private:
    int kernel_size_;
    float sigma_;
    std::vector<float> kernel_;
    
    void create_gaussian_kernel();
};

// Pad image
class Pad : public Transform {
public:
    Pad(int top, int bottom, int left, int right, float fill_value = 0.0f);
    Image apply(const Image& image) const override;
    std::unique_ptr<Transform> clone() const override;
    
private:
    int top_, bottom_, left_, right_;
    float fill_value_;
};

} // namespace ml
} // namespace cv

#endif // COMPUTER_VISION_TRANSFORMS_H
