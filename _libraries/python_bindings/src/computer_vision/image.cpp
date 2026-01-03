#include "../../libraries/include/computer_vision/image.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace ml {
namespace cv {

Image::Image() : height_(0), width_(0), channels_(0), format_(ImageFormat::RGB) {}

Image::Image(int height, int width, ImageFormat format)
    : height_(height), width_(width), format_(format) {
    switch (format) {
        case ImageFormat::GRAYSCALE:
            channels_ = 1;
            break;
        case ImageFormat::RGB:
            channels_ = 3;
            break;
        case ImageFormat::RGBA:
            channels_ = 4;
            break;
    }
    data_.resize(height_ * width_ * channels_, 0.0f);
}

Image::Image(int height, int width, int channels, const std::vector<float>& data)
    : height_(height), width_(width), channels_(channels), data_(data) {
    if (data.size() != static_cast<size_t>(height * width * channels)) {
        throw std::invalid_argument("Data size doesn't match dimensions");
    }
    update_format();
}

Image::Image(const Image& other)
    : height_(other.height_), width_(other.width_), 
      channels_(other.channels_), format_(other.format_), 
      data_(other.data_) {}

Image& Image::operator=(const Image& other) {
    if (this != &other) {
        height_ = other.height_;
        width_ = other.width_;
        channels_ = other.channels_;
        format_ = other.format_;
        data_ = other.data_;
    }
    return *this;
}

Image::~Image() {}

void Image::update_format() {
    switch (channels_) {
        case 1:
            format_ = ImageFormat::GRAYSCALE;
            break;
        case 3:
            format_ = ImageFormat::RGB;
            break;
        case 4:
            format_ = ImageFormat::RGBA;
            break;
        default:
            format_ = ImageFormat::RGB;
    }
}

float& Image::at(int row, int col, int channel) {
    if (row < 0 || row >= height_ || col < 0 || col >= width_ || 
        channel < 0 || channel >= channels_) {
        throw std::out_of_range("Image index out of range");
    }
    return data_[(row * width_ + col) * channels_ + channel];
}

const float& Image::at(int row, int col, int channel) const {
    if (row < 0 || row >= height_ || col < 0 || col >= width_ || 
        channel < 0 || channel >= channels_) {
        throw std::out_of_range("Image index out of range");
    }
    return data_[(row * width_ + col) * channels_ + channel];
}

float* Image::pixel_ptr(int row, int col) {
    if (row < 0 || row >= height_ || col < 0 || col >= width_) {
        throw std::out_of_range("Image index out of range");
    }
    return &data_[(row * width_ + col) * channels_];
}

const float* Image::pixel_ptr(int row, int col) const {
    if (row < 0 || row >= height_ || col < 0 || col >= width_) {
        throw std::out_of_range("Image index out of range");
    }
    return &data_[(row * width_ + col) * channels_];
}

Image Image::load(const std::string& filename) {
    // Stub implementation - would use stb_image or similar
    throw std::runtime_error("Image loading not implemented. Use create Image manually.");
}

void Image::save(const std::string& filename) const {
    // Stub implementation - would use stb_image_write or similar
    throw std::runtime_error("Image saving not implemented.");
}

Image Image::clone() const {
    return Image(*this);
}

void Image::fill(float value) {
    std::fill(data_.begin(), data_.end(), value);
}

void Image::fill(const std::vector<float>& values) {
    if (values.size() != static_cast<size_t>(channels_)) {
        throw std::invalid_argument("Values size must match number of channels");
    }
    for (int i = 0; i < height_ * width_; ++i) {
        for (int c = 0; c < channels_; ++c) {
            data_[i * channels_ + c] = values[c];
        }
    }
}

Image Image::to_grayscale() const {
    if (format_ == ImageFormat::GRAYSCALE) {
        return clone();
    }
    
    Image result(height_, width_, ImageFormat::GRAYSCALE);
    
    for (int i = 0; i < height_; ++i) {
        for (int j = 0; j < width_; ++j) {
            // Weighted RGB to grayscale conversion
            float gray = 0.299f * at(i, j, 0) + 
                        0.587f * at(i, j, 1) + 
                        0.114f * at(i, j, 2);
            result.at(i, j, 0) = gray;
        }
    }
    
    return result;
}

Image Image::to_rgb() const {
    if (format_ == ImageFormat::RGB) {
        return clone();
    }
    
    Image result(height_, width_, ImageFormat::RGB);
    
    if (format_ == ImageFormat::GRAYSCALE) {
        for (int i = 0; i < height_; ++i) {
            for (int j = 0; j < width_; ++j) {
                float gray = at(i, j, 0);
                result.at(i, j, 0) = gray;
                result.at(i, j, 1) = gray;
                result.at(i, j, 2) = gray;
            }
        }
    } else if (format_ == ImageFormat::RGBA) {
        for (int i = 0; i < height_; ++i) {
            for (int j = 0; j < width_; ++j) {
                result.at(i, j, 0) = at(i, j, 0);
                result.at(i, j, 1) = at(i, j, 1);
                result.at(i, j, 2) = at(i, j, 2);
            }
        }
    }
    
    return result;
}

std::vector<float> Image::mean() const {
    std::vector<float> means(channels_, 0.0f);
    
    for (int c = 0; c < channels_; ++c) {
        for (int i = 0; i < height_; ++i) {
            for (int j = 0; j < width_; ++j) {
                means[c] += at(i, j, c);
            }
        }
        means[c] /= (height_ * width_);
    }
    
    return means;
}

std::vector<float> Image::std() const {
    std::vector<float> means = mean();
    std::vector<float> stds(channels_, 0.0f);
    
    for (int c = 0; c < channels_; ++c) {
        for (int i = 0; i < height_; ++i) {
            for (int j = 0; j < width_; ++j) {
                float diff = at(i, j, c) - means[c];
                stds[c] += diff * diff;
            }
        }
        stds[c] = std::sqrt(stds[c] / (height_ * width_));
    }
    
    return stds;
}

std::pair<float, float> Image::min_max() const {
    if (data_.empty()) {
        return {0.0f, 0.0f};
    }
    
    float min_val = data_[0];
    float max_val = data_[0];
    
    for (float val : data_) {
        min_val = std::min(min_val, val);
        max_val = std::max(max_val, val);
    }
    
    return {min_val, max_val};
}

} // namespace ml
} // namespace cv
