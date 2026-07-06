#ifndef COMPUTER_VISION_IMAGE_H
#define COMPUTER_VISION_IMAGE_H

#include <vector>
#include <string>
#include <memory>

namespace ml {
namespace cv {

enum class ImageFormat {
    GRAYSCALE,  // Single channel
    RGB,        // 3 channels
    RGBA        // 4 channels
};

enum class InterpolationMode {
    NEAREST,
    BILINEAR,
    BICUBIC
};

class Image {
public:
    // Constructors
    Image();
    Image(int height, int width, ImageFormat format = ImageFormat::RGB);
    Image(int height, int width, int channels, const std::vector<float>& data);
    Image(const Image& other);
    Image& operator=(const Image& other);
    ~Image();

    // Basic properties
    int height() const { return height_; }
    int width() const { return width_; }
    int channels() const { return channels_; }
    ImageFormat format() const { return format_; }
    size_t size() const { return data_.size(); }

    // Data access
    std::vector<float>& data() { return data_; }
    const std::vector<float>& data() const { return data_; }
    
    float& at(int row, int col, int channel = 0);
    const float& at(int row, int col, int channel = 0) const;
    
    float* pixel_ptr(int row, int col);
    const float* pixel_ptr(int row, int col) const;

    // I/O operations (stub - would need image library like stb_image)
    static Image load(const std::string& filename);
    void save(const std::string& filename) const;
    
    // Utility operations
    Image clone() const;
    void fill(float value);
    void fill(const std::vector<float>& values); // Per-channel fill
    
    // Format conversion
    Image to_grayscale() const;
    Image to_rgb() const;
    
    // Statistics
    std::vector<float> mean() const;          // Per-channel mean
    std::vector<float> std() const;           // Per-channel standard deviation
    std::pair<float, float> min_max() const;  // Global min/max
    
private:
    int height_;
    int width_;
    int channels_;
    ImageFormat format_;
    std::vector<float> data_;
    
    void update_format();
};

} // namespace cv
} // namespace ml

#endif // COMPUTER_VISION_IMAGE_H
