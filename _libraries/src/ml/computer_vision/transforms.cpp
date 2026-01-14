#include "ML/computer_vision/transforms.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace ml {
namespace cv {

// Helper function for bilinear interpolation
static float bilinear_interpolate(const Image& img, float y, float x, int c) {
    int y0 = static_cast<int>(std::floor(y));
    int y1 = y0 + 1;
    int x0 = static_cast<int>(std::floor(x));
    int x1 = x0 + 1;
    
    // Clamp to image boundaries
    y0 = std::max(0, std::min(y0, img.height() - 1));
    y1 = std::max(0, std::min(y1, img.height() - 1));
    x0 = std::max(0, std::min(x0, img.width() - 1));
    x1 = std::max(0, std::min(x1, img.width() - 1));
    
    float wy1 = y - y0;
    float wy0 = 1.0f - wy1;
    float wx1 = x - x0;
    float wx0 = 1.0f - wx1;
    
    float val = wy0 * wx0 * img.at(y0, x0, c) +
                wy0 * wx1 * img.at(y0, x1, c) +
                wy1 * wx0 * img.at(y1, x0, c) +
                wy1 * wx1 * img.at(y1, x1, c);
    
    return val;
}

// Resize
Resize::Resize(int height, int width, InterpolationMode mode)
    : height_(height), width_(width), mode_(mode) {}

Image Resize::apply(const Image& image) const {
    Image result(height_, width_, image.format());
    
    float scale_y = static_cast<float>(image.height()) / height_;
    float scale_x = static_cast<float>(image.width()) / width_;
    
    for (int i = 0; i < height_; ++i) {
        for (int j = 0; j < width_; ++j) {
            float src_y = (i + 0.5f) * scale_y - 0.5f;
            float src_x = (j + 0.5f) * scale_x - 0.5f;
            
            for (int c = 0; c < image.channels(); ++c) {
                if (mode_ == InterpolationMode::NEAREST) {
                    int y = static_cast<int>(std::round(src_y));
                    int x = static_cast<int>(std::round(src_x));
                    y = std::max(0, std::min(y, image.height() - 1));
                    x = std::max(0, std::min(x, image.width() - 1));
                    result.at(i, j, c) = image.at(y, x, c);
                } else {  // BILINEAR
                    result.at(i, j, c) = bilinear_interpolate(image, src_y, src_x, c);
                }
            }
        }
    }
    
    return result;
}

std::unique_ptr<Transform> Resize::clone() const {
    return std::make_unique<Resize>(height_, width_, mode_);
}

// CenterCrop
CenterCrop::CenterCrop(int height, int width) : height_(height), width_(width) {}

Image CenterCrop::apply(const Image& image) const {
    if (height_ > image.height() || width_ > image.width()) {
        throw std::invalid_argument("Crop size larger than image");
    }
    
    int start_y = (image.height() - height_) / 2;
    int start_x = (image.width() - width_) / 2;
    
    Image result(height_, width_, image.format());
    
    for (int i = 0; i < height_; ++i) {
        for (int j = 0; j < width_; ++j) {
            for (int c = 0; c < image.channels(); ++c) {
                result.at(i, j, c) = image.at(start_y + i, start_x + j, c);
            }
        }
    }
    
    return result;
}

std::unique_ptr<Transform> CenterCrop::clone() const {
    return std::make_unique<CenterCrop>(height_, width_);
}

// RandomCrop
RandomCrop::RandomCrop(int height, int width, unsigned int seed)
    : height_(height), width_(width), rng_(seed == 0 ? std::random_device{}() : seed) {}

Image RandomCrop::apply(const Image& image) const {
    if (height_ > image.height() || width_ > image.width()) {
        throw std::invalid_argument("Crop size larger than image");
    }
    
    std::uniform_int_distribution<int> dist_y(0, image.height() - height_);
    std::uniform_int_distribution<int> dist_x(0, image.width() - width_);
    
    int start_y = dist_y(rng_);
    int start_x = dist_x(rng_);
    
    Image result(height_, width_, image.format());
    
    for (int i = 0; i < height_; ++i) {
        for (int j = 0; j < width_; ++j) {
            for (int c = 0; c < image.channels(); ++c) {
                result.at(i, j, c) = image.at(start_y + i, start_x + j, c);
            }
        }
    }
    
    return result;
}

std::unique_ptr<Transform> RandomCrop::clone() const {
    return std::make_unique<RandomCrop>(height_, width_, 0);
}

// HorizontalFlip
Image HorizontalFlip::apply(const Image& image) const {
    Image result(image.height(), image.width(), image.format());
    
    for (int i = 0; i < image.height(); ++i) {
        for (int j = 0; j < image.width(); ++j) {
            for (int c = 0; c < image.channels(); ++c) {
                result.at(i, j, c) = image.at(i, image.width() - 1 - j, c);
            }
        }
    }
    
    return result;
}

std::unique_ptr<Transform> HorizontalFlip::clone() const {
    return std::make_unique<HorizontalFlip>();
}

// VerticalFlip
Image VerticalFlip::apply(const Image& image) const {
    Image result(image.height(), image.width(), image.format());
    
    for (int i = 0; i < image.height(); ++i) {
        for (int j = 0; j < image.width(); ++j) {
            for (int c = 0; c < image.channels(); ++c) {
                result.at(i, j, c) = image.at(image.height() - 1 - i, j, c);
            }
        }
    }
    
    return result;
}

std::unique_ptr<Transform> VerticalFlip::clone() const {
    return std::make_unique<VerticalFlip>();
}

// RandomHorizontalFlip
RandomHorizontalFlip::RandomHorizontalFlip(float probability, unsigned int seed)
    : probability_(probability), 
      rng_(seed == 0 ? std::random_device{}() : seed),
      dist_(0.0f, 1.0f) {}

Image RandomHorizontalFlip::apply(const Image& image) const {
    if (dist_(rng_) < probability_) {
        return HorizontalFlip().apply(image);
    }
    return image.clone();
}

std::unique_ptr<Transform> RandomHorizontalFlip::clone() const {
    return std::make_unique<RandomHorizontalFlip>(probability_, 0);
}

// Normalize
Normalize::Normalize(const std::vector<float>& mean, const std::vector<float>& std)
    : mean_(mean), std_(std) {
    if (mean.size() != std.size()) {
        throw std::invalid_argument("Mean and std must have same size");
    }
}

Image Normalize::apply(const Image& image) const {
    if (mean_.size() != static_cast<size_t>(image.channels())) {
        throw std::invalid_argument("Mean/std size must match image channels");
    }
    
    Image result = image.clone();
    
    for (int i = 0; i < image.height(); ++i) {
        for (int j = 0; j < image.width(); ++j) {
            for (int c = 0; c < image.channels(); ++c) {
                result.at(i, j, c) = (image.at(i, j, c) - mean_[c]) / std_[c];
            }
        }
    }
    
    return result;
}

std::unique_ptr<Transform> Normalize::clone() const {
    return std::make_unique<Normalize>(mean_, std_);
}

// Standardize
Image Standardize::apply(const Image& image) const {
    std::vector<float> mean = image.mean();
    std::vector<float> std = image.std();
    
    // Avoid division by zero
    for (float& s : std) {
        if (s < 1e-7f) s = 1.0f;
    }
    
    return Normalize(mean, std).apply(image);
}

std::unique_ptr<Transform> Standardize::clone() const {
    return std::make_unique<Standardize>();
}

// Rotate
Rotate::Rotate(float angle_degrees, InterpolationMode mode)
    : angle_degrees_(angle_degrees), mode_(mode) {}

Image Rotate::apply(const Image& image) const {
    Image result(image.height(), image.width(), image.format());
    result.fill(0.0f);
    
    float angle_rad = angle_degrees_ * M_PI / 180.0f;
    float cos_a = std::cos(angle_rad);
    float sin_a = std::sin(angle_rad);
    
    int center_y = image.height() / 2;
    int center_x = image.width() / 2;
    
    for (int i = 0; i < result.height(); ++i) {
        for (int j = 0; j < result.width(); ++j) {
            // Translate to origin
            int y = i - center_y;
            int x = j - center_x;
            
            // Rotate (inverse)
            float src_y = cos_a * y + sin_a * x + center_y;
            float src_x = -sin_a * y + cos_a * x + center_x;
            
            if (src_y >= 0 && src_y < image.height() && 
                src_x >= 0 && src_x < image.width()) {
                for (int c = 0; c < image.channels(); ++c) {
                    if (mode_ == InterpolationMode::NEAREST) {
                        int sy = static_cast<int>(std::round(src_y));
                        int sx = static_cast<int>(std::round(src_x));
                        result.at(i, j, c) = image.at(sy, sx, c);
                    } else {
                        result.at(i, j, c) = bilinear_interpolate(image, src_y, src_x, c);
                    }
                }
            }
        }
    }
    
    return result;
}

std::unique_ptr<Transform> Rotate::clone() const {
    return std::make_unique<Rotate>(angle_degrees_, mode_);
}

// RandomRotation
RandomRotation::RandomRotation(float min_angle, float max_angle, 
                               InterpolationMode mode, unsigned int seed)
    : min_angle_(min_angle), max_angle_(max_angle), mode_(mode),
      rng_(seed == 0 ? std::random_device{}() : seed),
      dist_(min_angle, max_angle) {}

Image RandomRotation::apply(const Image& image) const {
    float angle = dist_(rng_);
    return Rotate(angle, mode_).apply(image);
}

std::unique_ptr<Transform> RandomRotation::clone() const {
    return std::make_unique<RandomRotation>(min_angle_, max_angle_, mode_, 0);
}

// AdjustBrightness
AdjustBrightness::AdjustBrightness(float factor) : factor_(factor) {}

Image AdjustBrightness::apply(const Image& image) const {
    Image result = image.clone();
    
    for (int i = 0; i < image.height(); ++i) {
        for (int j = 0; j < image.width(); ++j) {
            for (int c = 0; c < image.channels(); ++c) {
                result.at(i, j, c) = std::max(0.0f, std::min(1.0f, 
                    image.at(i, j, c) * factor_));
            }
        }
    }
    
    return result;
}

std::unique_ptr<Transform> AdjustBrightness::clone() const {
    return std::make_unique<AdjustBrightness>(factor_);
}

// AdjustContrast
AdjustContrast::AdjustContrast(float factor) : factor_(factor) {}

Image AdjustContrast::apply(const Image& image) const {
    std::vector<float> mean = image.mean();
    Image result = image.clone();
    
    for (int i = 0; i < image.height(); ++i) {
        for (int j = 0; j < image.width(); ++j) {
            for (int c = 0; c < image.channels(); ++c) {
                float val = mean[c] + factor_ * (image.at(i, j, c) - mean[c]);
                result.at(i, j, c) = std::max(0.0f, std::min(1.0f, val));
            }
        }
    }
    
    return result;
}

std::unique_ptr<Transform> AdjustContrast::clone() const {
    return std::make_unique<AdjustContrast>(factor_);
}

// GaussianBlur
GaussianBlur::GaussianBlur(int kernel_size, float sigma)
    : kernel_size_(kernel_size), sigma_(sigma) {
    if (kernel_size_ % 2 == 0) {
        throw std::invalid_argument("Kernel size must be odd");
    }
    create_gaussian_kernel();
}

void GaussianBlur::create_gaussian_kernel() {
    int half = kernel_size_ / 2;
    kernel_.resize(kernel_size_ * kernel_size_);
    float sum = 0.0f;
    
    for (int i = 0; i < kernel_size_; ++i) {
        for (int j = 0; j < kernel_size_; ++j) {
            int y = i - half;
            int x = j - half;
            float val = std::exp(-(x*x + y*y) / (2.0f * sigma_ * sigma_));
            kernel_[i * kernel_size_ + j] = val;
            sum += val;
        }
    }
    
    // Normalize
    for (float& val : kernel_) {
        val /= sum;
    }
}

Image GaussianBlur::apply(const Image& image) const {
    Image result(image.height(), image.width(), image.format());
    int half = kernel_size_ / 2;
    
    for (int i = 0; i < image.height(); ++i) {
        for (int j = 0; j < image.width(); ++j) {
            for (int c = 0; c < image.channels(); ++c) {
                float val = 0.0f;
                
                for (int ky = 0; ky < kernel_size_; ++ky) {
                    for (int kx = 0; kx < kernel_size_; ++kx) {
                        int y = i + ky - half;
                        int x = j + kx - half;
                        
                        // Clamp to boundaries
                        y = std::max(0, std::min(y, image.height() - 1));
                        x = std::max(0, std::min(x, image.width() - 1));
                        
                        val += image.at(y, x, c) * kernel_[ky * kernel_size_ + kx];
                    }
                }
                
                result.at(i, j, c) = val;
            }
        }
    }
    
    return result;
}

std::unique_ptr<Transform> GaussianBlur::clone() const {
    return std::make_unique<GaussianBlur>(kernel_size_, sigma_);
}

// Pad
Pad::Pad(int top, int bottom, int left, int right, float fill_value)
    : top_(top), bottom_(bottom), left_(left), right_(right), fill_value_(fill_value) {}

Image Pad::apply(const Image& image) const {
    int new_height = image.height() + top_ + bottom_;
    int new_width = image.width() + left_ + right_;
    
    Image result(new_height, new_width, image.format());
    result.fill(fill_value_);
    
    for (int i = 0; i < image.height(); ++i) {
        for (int j = 0; j < image.width(); ++j) {
            for (int c = 0; c < image.channels(); ++c) {
                result.at(i + top_, j + left_, c) = image.at(i, j, c);
            }
        }
    }
    
    return result;
}

std::unique_ptr<Transform> Pad::clone() const {
    return std::make_unique<Pad>(top_, bottom_, left_, right_, fill_value_);
}

} // namespace ml
} // namespace cv
