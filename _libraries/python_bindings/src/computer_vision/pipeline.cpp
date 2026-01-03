#include "../../libraries/include/computer_vision/pipeline.h"
#include <stdexcept>

namespace ml {
namespace cv {

void TransformPipeline::add(std::unique_ptr<Transform> transform) {
    if (!transform) {
        throw std::invalid_argument("Cannot add null transform");
    }
    transforms_.push_back(std::move(transform));
}

Image TransformPipeline::apply(const Image& image) const {
    Image result = image.clone();
    
    for (const auto& transform : transforms_) {
        result = transform->apply(result);
    }
    
    return result;
}

std::vector<Image> TransformPipeline::apply_batch(const std::vector<Image>& images) const {
    std::vector<Image> results;
    results.reserve(images.size());
    
    for (const auto& image : images) {
        results.push_back(apply(image));
    }
    
    return results;
}

TransformPipeline TransformPipeline::clone() const {
    TransformPipeline pipeline;
    
    for (const auto& transform : transforms_) {
        pipeline.add(transform->clone());
    }
    
    return pipeline;
}

// Predefined pipelines

TransformPipeline create_imagenet_pipeline(int image_size) {
    TransformPipeline pipeline;
    
    // Resize to slightly larger
    pipeline.add<Resize>(int(image_size * 1.15f), int(image_size * 1.15f));
    
    // Center crop to target size
    pipeline.add<CenterCrop>(image_size, image_size);
    
    // Normalize with ImageNet statistics
    pipeline.add<Normalize>(
        std::vector<float>{0.485f, 0.456f, 0.406f},  // mean
        std::vector<float>{0.229f, 0.224f, 0.225f}   // std
    );
    
    return pipeline;
}

TransformPipeline create_training_augmentation_pipeline(
    int image_size,
    bool random_flip,
    bool random_rotation,
    bool random_brightness,
    bool random_contrast
) {
    TransformPipeline pipeline;
    
    // Random crop
    pipeline.add<RandomCrop>(image_size, image_size);
    
    // Random horizontal flip
    if (random_flip) {
        pipeline.add<RandomHorizontalFlip>(0.5f);
    }
    
    // Random rotation
    if (random_rotation) {
        pipeline.add<RandomRotation>(-15.0f, 15.0f);
    }
    
    // Random brightness
    if (random_brightness) {
        // This would need a random brightness transform
        // For now, apply fixed brightness adjustment
        pipeline.add<AdjustBrightness>(1.0f);
    }
    
    // Random contrast
    if (random_contrast) {
        // This would need a random contrast transform
        pipeline.add<AdjustContrast>(1.0f);
    }
    
    // Standardize
    pipeline.add<Standardize>();
    
    return pipeline;
}

TransformPipeline create_inference_pipeline(
    int image_size,
    const std::vector<float>& mean,
    const std::vector<float>& std
) {
    TransformPipeline pipeline;
    
    // Resize to target size
    pipeline.add<Resize>(image_size, image_size);
    
    // Normalize
    pipeline.add<Normalize>(mean, std);
    
    return pipeline;
}

} // namespace ml
} // namespace cv
