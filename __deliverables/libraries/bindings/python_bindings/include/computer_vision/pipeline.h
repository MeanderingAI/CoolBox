#ifndef COMPUTER_VISION_PIPELINE_H
#define COMPUTER_VISION_PIPELINE_H

#include "transforms.h"
#include <vector>
#include <memory>

namespace ml {
namespace cv {

class TransformPipeline {
public:
    TransformPipeline() = default;
    ~TransformPipeline() = default;
    
    // Delete copy constructor and copy assignment (unique_ptr is non-copyable)
    TransformPipeline(const TransformPipeline&) = delete;
    TransformPipeline& operator=(const TransformPipeline&) = delete;
    
    // Move constructor and move assignment
    TransformPipeline(TransformPipeline&&) = default;
    TransformPipeline& operator=(TransformPipeline&&) = default;
    
    // Add transforms to pipeline
    void add(std::unique_ptr<Transform> transform);
    
    template<typename T, typename... Args>
    void add(Args&&... args) {
        add(std::make_unique<T>(std::forward<Args>(args)...));
    }
    
    // Apply all transforms in sequence
    Image apply(const Image& image) const;
    
    // Apply to batch of images
    std::vector<Image> apply_batch(const std::vector<Image>& images) const;
    
    // Get number of transforms
    size_t size() const { return transforms_.size(); }
    
    // Clear all transforms
    void clear() { transforms_.clear(); }
    
    // Clone the pipeline
    TransformPipeline clone() const;
    
private:
    std::vector<std::unique_ptr<Transform>> transforms_;
};

// Predefined pipelines for common use cases

// ImageNet-style preprocessing
TransformPipeline create_imagenet_pipeline(int image_size = 224);

// Data augmentation pipeline for training
TransformPipeline create_training_augmentation_pipeline(
    int image_size,
    bool random_flip = true,
    bool random_rotation = true,
    bool random_brightness = true,
    bool random_contrast = true
);

// Simple preprocessing pipeline for inference
TransformPipeline create_inference_pipeline(
    int image_size,
    const std::vector<float>& mean = {0.485f, 0.456f, 0.406f},
    const std::vector<float>& std = {0.229f, 0.224f, 0.225f}
);

} // namespace cv
} // namespace ml

#endif // COMPUTER_VISION_PIPELINE_H
