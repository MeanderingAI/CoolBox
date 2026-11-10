#include "../include/vulkan_tensor_backend.h"

#include "vulkan_tensor_shader_fp32.h"
#include "vulkan_tensor_shader_fp64.h"
#include "vulkan_tensor_shader_lowp.h"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ml {
namespace deep_learning {
namespace detail {

struct VulkanBufferAllocation {
    VkDevice device = VK_NULL_HANDLE;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
    VkDeviceSize capacity = 0;
    std::shared_ptr<std::atomic<bool>> device_alive;

    ~VulkanBufferAllocation() {
        if (device == VK_NULL_HANDLE ||
            (device_alive && !device_alive->load())) {
            return;
        }
        if (mapped != nullptr) {
            vkUnmapMemory(device, memory);
        }
        if (buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, buffer, nullptr);
        }
        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, memory, nullptr);
        }
    }
};

struct VulkanStoragePool {
    explicit VulkanStoragePool(VkDevice device)
        : device(device),
          device_alive(std::make_shared<std::atomic<bool>>(true)) {}

    ~VulkanStoragePool() {
        shutdown();
    }

    std::unique_ptr<VulkanBufferAllocation> acquire(
        VkDeviceSize requested_size) {
        std::lock_guard<std::mutex> lock(mutex);
        const auto candidate = available.lower_bound(requested_size);
        if (candidate == available.end()) {
            return {};
        }
        std::unique_ptr<VulkanBufferAllocation> allocation =
            std::move(candidate->second);
        cached_bytes -= allocation->capacity;
        available.erase(candidate);
        return allocation;
    }

    void release(
        std::unique_ptr<VulkanBufferAllocation> allocation) {
        if (!allocation) {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex);
        if (!accepting || allocation->device == VK_NULL_HANDLE) {
            return;
        }
        if (allocation->capacity >
            MAX_CACHED_BYTES -
                std::min(cached_bytes, MAX_CACHED_BYTES)) {
            return;
        }
        cached_bytes += allocation->capacity;
        available.emplace(
            allocation->capacity, std::move(allocation));
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex);
        if (!accepting && available.empty()) {
            return;
        }
        accepting = false;
        available.clear();
        cached_bytes = 0;
    }

    void mark_device_destroyed() {
        device_alive->store(false);
    }

    VkDevice device = VK_NULL_HANDLE;
    std::shared_ptr<std::atomic<bool>> device_alive;
    std::mutex mutex;
    std::multimap<
        VkDeviceSize,
        std::unique_ptr<VulkanBufferAllocation>> available;
    VkDeviceSize cached_bytes = 0;
    bool accepting = true;

    static constexpr VkDeviceSize MAX_CACHED_BYTES =
        1024ULL * 1024ULL * 1024ULL;
};

struct VulkanTensorStorage {
    VkDevice device = VK_NULL_HANDLE;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
    std::size_t element_count = 0;
    TensorPrecision precision = TensorPrecision::FP64;
    std::shared_ptr<VulkanStoragePool> pool;
    std::unique_ptr<VulkanBufferAllocation> allocation;

    ~VulkanTensorStorage() {
        if (pool && allocation) {
            pool->release(std::move(allocation));
        }
    }
};

namespace {

void check_vulkan(VkResult result, const char* operation) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            std::string(operation) + " failed with Vulkan error " +
            std::to_string(static_cast<int>(result)));
    }
}

std::string lowercase(std::string value) {
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

struct DispatchParameters {
    std::uint32_t operation;
    std::uint32_t count;
    std::uint32_t rows;
    std::uint32_t columns;
    std::uint32_t inner;
    std::uint32_t scalar_low;
    std::uint32_t scalar_high;
    std::uint32_t encoding;
    std::uint32_t extra0;
    std::uint32_t extra1;
    std::uint32_t extra2;
    std::uint32_t extra3;
};

static_assert(sizeof(DispatchParameters) == 12 * sizeof(std::uint32_t),
              "Unexpected Vulkan push-constant layout");

std::uint16_t float_to_half(float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    const std::uint32_t sign = (bits >> 16) & 0x8000U;
    std::int32_t exponent =
        static_cast<std::int32_t>((bits >> 23) & 0xffU) - 127 + 15;
    std::uint32_t mantissa = bits & 0x7fffffU;
    if (exponent <= 0) {
        if (exponent < -10) {
            return static_cast<std::uint16_t>(sign);
        }
        mantissa = (mantissa | 0x800000U) >> (1 - exponent);
        return static_cast<std::uint16_t>(
            sign + ((mantissa + 0x1000U) >> 13));
    }
    if (exponent >= 31) {
        return static_cast<std::uint16_t>(
            sign | (mantissa == 0 ? 0x7c00U : 0x7e00U));
    }
    return static_cast<std::uint16_t>(
        sign | (static_cast<std::uint32_t>(exponent) << 10) |
        ((mantissa + 0x1000U) >> 13));
}

float half_to_float(std::uint16_t value) {
    const std::uint32_t sign =
        static_cast<std::uint32_t>(value & 0x8000U) << 16;
    std::int32_t exponent =
        static_cast<std::int32_t>((value >> 10) & 0x1fU);
    std::uint32_t mantissa = value & 0x3ffU;
    std::uint32_t bits = 0;
    if (exponent == 0) {
        if (mantissa == 0) {
            bits = sign;
        } else {
            exponent = 1;
            while ((mantissa & 0x400U) == 0) {
                mantissa <<= 1;
                --exponent;
            }
            mantissa &= 0x3ffU;
            bits = sign |
                (static_cast<std::uint32_t>(exponent + 112) << 23) |
                (mantissa << 13);
        }
    } else if (exponent == 31) {
        bits = sign | 0x7f800000U | (mantissa << 13);
    } else {
        bits = sign |
            (static_cast<std::uint32_t>(exponent + 112) << 23) |
            (mantissa << 13);
    }
    float result = 0.0F;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

std::uint16_t float_to_bfloat16(float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    const std::uint32_t rounding =
        0x7fffU + ((bits >> 16) & 1U);
    return static_cast<std::uint16_t>((bits + rounding) >> 16);
}

float bfloat16_to_float(std::uint16_t value) {
    const std::uint32_t bits =
        static_cast<std::uint32_t>(value) << 16;
    float result = 0.0F;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

std::size_t precision_size(TensorPrecision precision) {
    switch (precision) {
        case TensorPrecision::FP64:
            return sizeof(double);
        case TensorPrecision::FP32:
            return sizeof(float);
        case TensorPrecision::FP16:
        case TensorPrecision::BF16:
            return sizeof(std::uint16_t);
        case TensorPrecision::AUTO:
            break;
    }
    throw std::invalid_argument("Unresolved tensor precision");
}

std::vector<std::uint8_t> encode_values(
    const double* values,
    std::size_t count,
    TensorPrecision precision) {
    std::vector<std::uint8_t> encoded(
        count * precision_size(precision));
    if (precision == TensorPrecision::FP64) {
        std::memcpy(
            encoded.data(), values, encoded.size());
    } else if (precision == TensorPrecision::FP32) {
        for (std::size_t index = 0; index < count; ++index) {
            const float value =
                static_cast<float>(values[index]);
            std::memcpy(
                encoded.data() + index * sizeof(float),
                &value, sizeof(value));
        }
    } else {
        for (std::size_t index = 0; index < count; ++index) {
            const float value = static_cast<float>(values[index]);
            const std::uint16_t converted =
                precision == TensorPrecision::FP16
                    ? float_to_half(value)
                    : float_to_bfloat16(value);
            std::memcpy(
                encoded.data() + index * sizeof(converted),
                &converted, sizeof(converted));
        }
    }
    return encoded;
}

std::vector<double> decode_values(
    const void* values,
    std::size_t count,
    TensorPrecision precision) {
    std::vector<double> result(count);
    if (precision == TensorPrecision::FP64) {
        std::memcpy(
            result.data(), values, count * sizeof(double));
    } else if (precision == TensorPrecision::FP32) {
        const auto* input =
            static_cast<const std::uint8_t*>(values);
        for (std::size_t index = 0; index < count; ++index) {
            float value = 0.0F;
            std::memcpy(
                &value, input + index * sizeof(value),
                sizeof(value));
            result[index] = value;
        }
    } else {
        const auto* input =
            static_cast<const std::uint8_t*>(values);
        for (std::size_t index = 0; index < count; ++index) {
            std::uint16_t value = 0;
            std::memcpy(
                &value, input + index * sizeof(value),
                sizeof(value));
            result[index] =
                precision == TensorPrecision::FP16
                    ? half_to_float(value)
                    : bfloat16_to_float(value);
        }
    }
    return result;
}

class VulkanTensorRuntime {
public:
    VulkanTensorRuntime() {
        try {
            initialize();
            available_ = true;
        } catch (const std::exception& error) {
            unavailable_reason_ = error.what();
            destroy();
        }
    }

    ~VulkanTensorRuntime() {
        destroy();
    }

    VulkanTensorRuntime(const VulkanTensorRuntime&) = delete;
    VulkanTensorRuntime& operator=(const VulkanTensorRuntime&) = delete;

    bool available() const {
        return available_;
    }

    const std::string& unavailable_reason() const {
        return unavailable_reason_;
    }

    const std::string& device_name() const {
        return device_name_;
    }

    bool precision_available(TensorPrecision precision) const {
        return precision == TensorPrecision::AUTO ||
            precision == TensorPrecision::FP64 ||
            precision == TensorPrecision::FP32 ||
            ((precision == TensorPrecision::FP16 ||
              precision == TensorPrecision::BF16) &&
             low_precision_available_);
    }

    TensorPrecision resolved_precision() const {
        TensorPrecision precision =
            requested_tensor_precision.load();
        if (precision == TensorPrecision::AUTO) {
            precision = TensorPrecision::FP32;
        }
        if (!precision_available(precision)) {
            throw std::runtime_error(
                "Requested Vulkan tensor precision is unavailable");
        }
        return precision;
    }

    VulkanTensorStoragePtr upload(
        const std::vector<double>& values) {
        const TensorPrecision precision = resolved_precision();
        const std::vector<std::uint8_t> encoded =
            encode_values(values.data(), values.size(), precision);
        std::lock_guard<std::mutex> lock(dispatch_mutex_);
        if (batch_output_bytes_ != 0 &&
            encoded.size() >
                MAX_BATCH_OUTPUT_BYTES -
                    std::min(
                        batch_output_bytes_,
                        MAX_BATCH_OUTPUT_BYTES)) {
            flush_batch();
        }
        VulkanTensorStoragePtr storage =
            create_storage(values.size(), precision);
        VulkanTensorStoragePtr staging =
            create_staging_storage(values.size(), precision);
        std::memcpy(
            staging->mapped, encoded.data(), encoded.size());

        begin_batch();
        SubmissionSlot& slot = submission_slots_[active_slot_];
        VkMemoryBarrier host_barrier{
            VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        host_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        host_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(
            slot.command_buffer,
            VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 1, &host_barrier, 0, nullptr, 0, nullptr);
        VkBufferCopy copy_region{};
        copy_region.size =
            static_cast<VkDeviceSize>(encoded.size());
        vkCmdCopyBuffer(
            slot.command_buffer,
            staging->buffer,
            storage->buffer,
            1,
            &copy_region);
        VkMemoryBarrier compute_barrier{
            VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        compute_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        compute_barrier.dstAccessMask =
            VK_ACCESS_SHADER_READ_BIT |
            VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(
            slot.command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 1, &compute_barrier, 0, nullptr, 0, nullptr);
        slot.storage.push_back(staging);
        slot.storage.push_back(storage);
        batch_output_bytes_ += encoded.size();
        return storage;
    }

    VulkanTensorStoragePtr execute_resident(
        VulkanTensorOperation operation,
        const VulkanTensorStoragePtr& left,
        const VulkanTensorStoragePtr& right,
        std::size_t output_size,
        std::size_t rows,
        std::size_t columns,
        std::size_t inner,
        double scalar,
        const VulkanTensorStoragePtr& auxiliary = {},
        const VulkanTensorStoragePtr& output_override = {},
        std::uint32_t extra0 = 0,
        std::uint32_t extra1 = 0,
        std::uint32_t extra2 = 0,
        std::uint32_t extra3 = 0) {
        if (!left) {
            throw std::invalid_argument(
                "Resident Vulkan operation requires a left tensor");
        }
        const TensorPrecision precision = resolved_precision();
        if (left->precision != precision ||
            (right && right->precision != precision) ||
            (auxiliary && auxiliary->precision != precision) ||
            (output_override &&
             output_override->precision != precision)) {
            throw std::invalid_argument(
                "Resident Vulkan tensors use different precisions");
        }
        std::lock_guard<std::mutex> lock(dispatch_mutex_);
        const std::size_t output_bytes =
            output_size * precision_size(precision);
        if (batch_dispatch_count_ >= MAX_BATCH_DISPATCHES ||
            (batch_output_bytes_ != 0 &&
             output_bytes >
                 MAX_BATCH_OUTPUT_BYTES -
                     std::min(
                         batch_output_bytes_,
                         MAX_BATCH_OUTPUT_BYTES))) {
            flush_batch();
        }
        begin_batch();
        SubmissionSlot& slot = submission_slots_[active_slot_];
        VulkanTensorStoragePtr output = output_override
            ? output_override
            : create_storage(output_size, precision);

        VkDescriptorSetAllocateInfo set_info{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        set_info.descriptorPool = slot.descriptor_pool;
        set_info.descriptorSetCount = 1;
        set_info.pSetLayouts = &descriptor_set_layout_;
        VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
        check_vulkan(
            vkAllocateDescriptorSets(
                device_, &set_info, &descriptor_set),
            "vkAllocateDescriptorSets");
        const VkBuffer right_buffer =
            right ? right->buffer : left->buffer;
        const VkBuffer auxiliary_buffer =
            auxiliary ? auxiliary->buffer : left->buffer;
        const std::array<VkDescriptorBufferInfo, 4> buffer_infos{{
            {left->buffer, 0, VK_WHOLE_SIZE},
            {right_buffer, 0, VK_WHOLE_SIZE},
            {auxiliary_buffer, 0, VK_WHOLE_SIZE},
            {output->buffer, 0, VK_WHOLE_SIZE},
        }};
        std::array<VkWriteDescriptorSet, 4> writes{};
        for (std::uint32_t index = 0;
             index < writes.size();
             ++index) {
            writes[index].sType =
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[index].dstSet = descriptor_set;
            writes[index].dstBinding = index;
            writes[index].descriptorCount = 1;
            writes[index].descriptorType =
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[index].pBufferInfo = &buffer_infos[index];
        }
        vkUpdateDescriptorSets(
            device_, static_cast<std::uint32_t>(writes.size()),
            writes.data(), 0, nullptr);
        const std::size_t pipeline_index =
            precision == TensorPrecision::FP64
                ? 0
                : precision == TensorPrecision::FP32 ? 1 : 2;
        vkCmdBindPipeline(
            slot.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
            pipelines_[pipeline_index]);
        vkCmdBindDescriptorSets(
            slot.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
            pipeline_layout_, 0, 1, &descriptor_set, 0, nullptr);

        std::uint64_t scalar_bits = 0;
        if (precision == TensorPrecision::FP64) {
            std::memcpy(&scalar_bits, &scalar, sizeof(scalar));
        } else {
            const float scalar_float =
                static_cast<float>(scalar);
            std::memcpy(
                &scalar_bits, &scalar_float,
                sizeof(scalar_float));
        }
        std::size_t work_items = output_size;
        if (operation == VulkanTensorOperation::ROW_SOFTMAX ||
            operation == VulkanTensorOperation::LAYER_NORM ||
            operation == VulkanTensorOperation::LAYER_NORM_BACKWARD) {
            work_items = rows;
        } else if (
            operation ==
                VulkanTensorOperation::LAYER_NORM_GAMMA_GRADIENT ||
            operation == VulkanTensorOperation::ROW_SUM) {
            work_items = columns;
        } else if (
            operation == VulkanTensorOperation::SQUARED_SUM ||
            operation == VulkanTensorOperation::SUM_REDUCTION) {
            work_items = rows;
        } else if (
            operation == VulkanTensorOperation::CROSS_ENTROPY) {
            work_items = rows;
        }
        const DispatchParameters parameters{
            static_cast<std::uint32_t>(operation),
            static_cast<std::uint32_t>(work_items),
            static_cast<std::uint32_t>(rows),
            static_cast<std::uint32_t>(columns),
            static_cast<std::uint32_t>(inner),
            static_cast<std::uint32_t>(scalar_bits),
            static_cast<std::uint32_t>(scalar_bits >> 32),
            static_cast<std::uint32_t>(precision),
            extra0,
            extra1,
            extra2,
            extra3,
        };
        vkCmdPushConstants(
            slot.command_buffer, pipeline_layout_,
            VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(parameters),
            &parameters);
        if (operation == VulkanTensorOperation::MATMUL ||
            operation == VulkanTensorOperation::MATMUL_BIAS ||
            operation ==
                VulkanTensorOperation::MATMUL_RIGHT_TRANSPOSED ||
            operation ==
                VulkanTensorOperation::MATMUL_LEFT_TRANSPOSED) {
            vkCmdDispatch(
                slot.command_buffer,
                (parameters.columns + 15U) / 16U,
                (parameters.rows + 15U) / 16U,
                1);
        } else {
            vkCmdDispatch(
                slot.command_buffer,
                (parameters.count + 255U) / 256U, 1, 1);
        }

        VkMemoryBarrier barrier{
            VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask =
            VK_ACCESS_SHADER_READ_BIT |
            VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(
            slot.command_buffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 1, &barrier, 0, nullptr, 0, nullptr);

        slot.storage.push_back(left);
        if (right) {
            slot.storage.push_back(right);
        }
        if (auxiliary) {
            slot.storage.push_back(auxiliary);
        }
        slot.storage.push_back(output);
        batch_output_bytes_ += output_bytes;
        ++batch_dispatch_count_;
        active_tensor_precision.store(precision);
        return output;
    }

    void adamw_step_in_place(
        const VulkanTensorStoragePtr& values,
        const VulkanTensorStoragePtr& gradients,
        const VulkanTensorStoragePtr& first_moment,
        const VulkanTensorStoragePtr& second_moment,
        double learning_rate,
        double beta1,
        double beta2,
        double beta1_correction,
        double beta2_correction,
        double epsilon,
        double weight_decay,
        double gradient_scale) {
        const auto float_bits = [](double value) {
            const float converted = static_cast<float>(value);
            std::uint32_t bits = 0;
            std::memcpy(&bits, &converted, sizeof(bits));
            return bits;
        };
        execute_resident(
            VulkanTensorOperation::FUSED_ADAMW_STEP,
            values,
            gradients,
            values->element_count,
            float_bits(beta1),
            float_bits(beta2),
            float_bits(epsilon),
            learning_rate,
            first_moment,
            second_moment,
            float_bits(beta1_correction),
            float_bits(beta2_correction),
            float_bits(weight_decay),
            float_bits(gradient_scale));
    }

    void synchronize() {
        std::lock_guard<std::mutex> lock(dispatch_mutex_);
        flush_batch();
        wait_for_submissions();
    }

    std::vector<double> download(
        const VulkanTensorStoragePtr& storage) {
        if (!storage) {
            throw std::invalid_argument(
                "Cannot download an empty Vulkan tensor");
        }
        std::lock_guard<std::mutex> lock(dispatch_mutex_);
        VulkanTensorStoragePtr staging =
            create_staging_storage(
                storage->element_count,
                storage->precision);
        begin_batch();
        SubmissionSlot& slot = submission_slots_[active_slot_];
        VkMemoryBarrier transfer_barrier{
            VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        transfer_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        transfer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(
            slot.command_buffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 1, &transfer_barrier, 0, nullptr, 0, nullptr);
        VkBufferCopy copy_region{};
        copy_region.size =
            storage->element_count *
            precision_size(storage->precision);
        vkCmdCopyBuffer(
            slot.command_buffer,
            storage->buffer,
            staging->buffer,
            1,
            &copy_region);
        VkMemoryBarrier host_barrier{
            VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        host_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        host_barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(
            slot.command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_HOST_BIT,
            0, 1, &host_barrier, 0, nullptr, 0, nullptr);
        slot.storage.push_back(storage);
        slot.storage.push_back(staging);
        flush_batch();
        wait_for_submissions();
        return decode_values(
            staging->mapped, storage->element_count,
            storage->precision);
    }

    bool storage_matches_precision(
        const VulkanTensorStoragePtr& storage) const {
        return storage &&
            storage->precision == resolved_precision();
    }

    std::vector<double> execute(
        VulkanTensorOperation operation,
        const std::vector<double>& left,
        const std::vector<double>& right,
        std::size_t output_size,
        std::size_t rows,
        std::size_t columns,
        std::size_t inner,
        double scalar) {
        if (!available_) {
            throw std::runtime_error(
                "Vulkan tensor backend is unavailable: " +
                unavailable_reason_);
        }
        if (output_size == 0) {
            return {};
        }
        if (output_size > std::numeric_limits<std::uint32_t>::max() ||
            rows > std::numeric_limits<std::uint32_t>::max() ||
            columns > std::numeric_limits<std::uint32_t>::max() ||
            inner > std::numeric_limits<std::uint32_t>::max()) {
            throw std::overflow_error(
                "Tensor dimensions exceed Vulkan shader limits");
        }

        const VulkanTensorStoragePtr left_storage = upload(left);
        const VulkanTensorStoragePtr right_storage =
            right.empty() ? VulkanTensorStoragePtr{} : upload(right);
        return download(execute_resident(
            operation, left_storage, right_storage,
            output_size, rows, columns, inner, scalar));
    }

private:
    struct SubmissionSlot {
        VkCommandBuffer command_buffer = VK_NULL_HANDLE;
        VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
        VkFence fence = VK_NULL_HANDLE;
        bool in_flight = false;
        std::vector<VulkanTensorStoragePtr> storage;
    };

    VulkanTensorStoragePtr create_storage(
        std::size_t element_count,
        TensorPrecision precision) {
        return create_buffer_storage(
            element_count,
            precision,
            device_storage_pool_,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            false);
    }

    VulkanTensorStoragePtr create_staging_storage(
        std::size_t element_count,
        TensorPrecision precision) {
        return create_buffer_storage(
            element_count,
            precision,
            staging_storage_pool_,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            true);
    }

    VulkanTensorStoragePtr create_buffer_storage(
        std::size_t element_count,
        TensorPrecision precision,
        const std::shared_ptr<VulkanStoragePool>& pool,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags memory_properties,
        bool map_memory) {
        VulkanTensorStoragePtr storage =
            std::make_shared<VulkanTensorStorage>();
        storage->device = device_;
        storage->element_count = element_count;
        storage->precision = precision;
        const VkDeviceSize byte_size =
            element_count * precision_size(precision);
        std::unique_ptr<VulkanBufferAllocation> allocation =
            pool->acquire(byte_size);
        if (allocation) {
            storage->buffer = allocation->buffer;
            storage->memory = allocation->memory;
            storage->mapped = allocation->mapped;
            storage->pool = pool;
            storage->allocation = std::move(allocation);
            return storage;
        }

        allocation =
            std::make_unique<VulkanBufferAllocation>();
        allocation->device = device_;
        allocation->device_alive = pool->device_alive;
        allocation->capacity = allocation_size_class(byte_size);

        VkBufferCreateInfo buffer_info{
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        buffer_info.size = allocation->capacity;
        buffer_info.usage = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        check_vulkan(
            vkCreateBuffer(
                device_, &buffer_info, nullptr, &allocation->buffer),
            "vkCreateBuffer");
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(
            device_, allocation->buffer, &requirements);
        VkMemoryAllocateInfo allocation_info{
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation_info.allocationSize = requirements.size;
        allocation_info.memoryTypeIndex = find_memory_type(
            requirements.memoryTypeBits, memory_properties);
        check_vulkan(
            vkAllocateMemory(
                device_, &allocation_info, nullptr, &allocation->memory),
            "vkAllocateMemory");
        check_vulkan(
            vkBindBufferMemory(
                device_, allocation->buffer, allocation->memory, 0),
            "vkBindBufferMemory");
        if (map_memory) {
            check_vulkan(
                vkMapMemory(
                    device_, allocation->memory, 0,
                    allocation->capacity, 0,
                    &allocation->mapped),
                "vkMapMemory");
        }
        storage->buffer = allocation->buffer;
        storage->memory = allocation->memory;
        storage->mapped = allocation->mapped;
        storage->pool = pool;
        storage->allocation = std::move(allocation);
        return storage;
    }

    static VkDeviceSize allocation_size_class(
        VkDeviceSize requested_size) {
        constexpr VkDeviceSize MINIMUM_SIZE = 256;
        VkDeviceSize size = MINIMUM_SIZE;
        while (size < requested_size &&
               size <=
                   std::numeric_limits<VkDeviceSize>::max() / 2) {
            size *= 2;
        }
        return std::max(size, requested_size);
    }

    void begin_batch() {
        if (batch_open_) {
            return;
        }
        SubmissionSlot& slot = submission_slots_[active_slot_];
        wait_for_slot(slot);
        check_vulkan(
            vkResetCommandBuffer(slot.command_buffer, 0),
            "vkResetCommandBuffer");
        VkCommandBufferBeginInfo begin_info{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check_vulkan(
            vkBeginCommandBuffer(slot.command_buffer, &begin_info),
            "vkBeginCommandBuffer");
        batch_open_ = true;
    }

    void flush_batch() {
        if (!batch_open_) {
            return;
        }
        SubmissionSlot& slot = submission_slots_[active_slot_];
        check_vulkan(
            vkEndCommandBuffer(slot.command_buffer),
            "vkEndCommandBuffer");
        check_vulkan(
            vkResetFences(device_, 1, &slot.fence),
            "vkResetFences");
        VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &slot.command_buffer;
        check_vulkan(
            vkQueueSubmit(queue_, 1, &submit_info, slot.fence),
            "vkQueueSubmit");
        slot.in_flight = true;
        batch_dispatch_count_ = 0;
        batch_output_bytes_ = 0;
        batch_open_ = false;
        active_slot_ =
            (active_slot_ + 1) % submission_slots_.size();
    }

    void wait_for_slot(SubmissionSlot& slot) {
        if (!slot.in_flight) {
            return;
        }
        check_vulkan(
            vkWaitForFences(
                device_, 1, &slot.fence, VK_TRUE, UINT64_MAX),
            "vkWaitForFences");
        check_vulkan(
            vkResetDescriptorPool(
                device_, slot.descriptor_pool, 0),
            "vkResetDescriptorPool");
        slot.storage.clear();
        slot.in_flight = false;
    }

    void wait_for_submissions() {
        for (SubmissionSlot& slot : submission_slots_) {
            wait_for_slot(slot);
        }
    }

    struct Buffer {
        VkDevice device = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize capacity = 0;
        void* mapped = nullptr;

        void destroy() {
            if (mapped != nullptr) {
                vkUnmapMemory(device, memory);
                mapped = nullptr;
            }
            if (buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(device, buffer, nullptr);
                buffer = VK_NULL_HANDLE;
            }
            if (memory != VK_NULL_HANDLE) {
                vkFreeMemory(device, memory, nullptr);
                memory = VK_NULL_HANDLE;
            }
            capacity = 0;
        }

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        Buffer() = default;

        const void* data() const {
            return mapped;
        }

        void ensure(
            VulkanTensorRuntime& runtime,
            std::size_t requested_size) {
            if (requested_size <= capacity) {
                return;
            }
            destroy();
            device = runtime.device_;
            create(runtime, requested_size);
        }

        void write(
            VulkanTensorRuntime& runtime,
            const void* values,
            std::size_t byte_size) {
            ensure(runtime, byte_size);
            std::memcpy(mapped, values, byte_size);
        }

    private:
        void create(
            VulkanTensorRuntime& runtime,
            VkDeviceSize requested_size) {
            if (requested_size == 0) {
                throw std::invalid_argument(
                    "Vulkan tensor buffers cannot be empty");
            }

            VkBufferCreateInfo buffer_info{
                VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            buffer_info.size = requested_size;
            buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            check_vulkan(
                vkCreateBuffer(
                    device, &buffer_info, nullptr, &buffer),
                "vkCreateBuffer");

            VkMemoryRequirements requirements{};
            vkGetBufferMemoryRequirements(
                device, buffer, &requirements);

            VkMemoryAllocateInfo allocation_info{
                VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            allocation_info.allocationSize = requirements.size;
            allocation_info.memoryTypeIndex = runtime.find_memory_type(
                requirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            check_vulkan(
                vkAllocateMemory(
                    device, &allocation_info, nullptr, &memory),
                "vkAllocateMemory");
            check_vulkan(
                vkBindBufferMemory(device, buffer, memory, 0),
                "vkBindBufferMemory");
            check_vulkan(
                vkMapMemory(
                    device, memory, 0, requested_size, 0, &mapped),
                "vkMapMemory");
            capacity = requested_size;
        }
    };

    std::uint32_t find_memory_type(
        std::uint32_t type_filter,
        VkMemoryPropertyFlags properties) const {
        VkPhysicalDeviceMemoryProperties memory_properties{};
        vkGetPhysicalDeviceMemoryProperties(
            physical_device_, &memory_properties);
        for (std::uint32_t index = 0;
             index < memory_properties.memoryTypeCount; ++index) {
            if ((type_filter & (1U << index)) != 0 &&
                (memory_properties.memoryTypes[index].propertyFlags &
                 properties) == properties) {
                return index;
            }
        }
        throw std::runtime_error(
            "Vulkan device has no compatible memory type");
    }

    void initialize() {
        VkApplicationInfo application_info{
            VK_STRUCTURE_TYPE_APPLICATION_INFO};
        application_info.pApplicationName = "CoolBox Tensor";
        application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        application_info.pEngineName = "CoolBox";
        application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        application_info.apiVersion = VK_API_VERSION_1_1;

        VkInstanceCreateInfo instance_info{
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        instance_info.pApplicationInfo = &application_info;
        check_vulkan(
            vkCreateInstance(&instance_info, nullptr, &instance_),
            "vkCreateInstance");

        std::uint32_t device_count = 0;
        check_vulkan(
            vkEnumeratePhysicalDevices(
                instance_, &device_count, nullptr),
            "vkEnumeratePhysicalDevices");
        if (device_count == 0) {
            throw std::runtime_error(
                "No Vulkan physical devices were found");
        }
        std::vector<VkPhysicalDevice> devices(device_count);
        check_vulkan(
            vkEnumeratePhysicalDevices(
                instance_, &device_count, devices.data()),
            "vkEnumeratePhysicalDevices");

        const char* selector_value =
            std::getenv("COOLBOX_VULKAN_DEVICE");
        const std::string selector =
            selector_value == nullptr
                ? std::string{}
                : lowercase(selector_value);

        int best_score = std::numeric_limits<int>::min();
        for (VkPhysicalDevice candidate : devices) {
            VkPhysicalDeviceFeatures features{};
            vkGetPhysicalDeviceFeatures(candidate, &features);
            if (features.shaderFloat64 != VK_TRUE) {
                continue;
            }

            std::uint32_t family_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(
                candidate, &family_count, nullptr);
            std::vector<VkQueueFamilyProperties> families(family_count);
            vkGetPhysicalDeviceQueueFamilyProperties(
                candidate, &family_count, families.data());

            std::uint32_t compute_family =
                std::numeric_limits<std::uint32_t>::max();
            for (std::uint32_t index = 0; index < family_count; ++index) {
                if ((families[index].queueFlags &
                     VK_QUEUE_COMPUTE_BIT) != 0) {
                    compute_family = index;
                    break;
                }
            }
            if (compute_family ==
                std::numeric_limits<std::uint32_t>::max()) {
                continue;
            }

            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(candidate, &properties);
            const std::string candidate_name = properties.deviceName;
            if (!selector.empty() &&
                lowercase(candidate_name).find(selector) ==
                    std::string::npos) {
                continue;
            }

            int score = 0;
            if (properties.deviceType ==
                VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                score = 300;
            } else if (properties.deviceType ==
                       VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                score = 200;
            } else {
                score = 100;
            }
            if (score > best_score) {
                best_score = score;
                physical_device_ = candidate;
                queue_family_ = compute_family;
                device_name_ = candidate_name;
            }
        }

        if (physical_device_ == VK_NULL_HANDLE) {
            if (!selector.empty()) {
                throw std::runtime_error(
                    "No Vulkan compute device matching '" + selector +
                    "' supports shaderFloat64");
            }
            throw std::runtime_error(
                "No Vulkan compute device with shaderFloat64 support "
                "was found");
        }

        const float queue_priority = 1.0F;
        VkDeviceQueueCreateInfo queue_info{
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queue_info.queueFamilyIndex = queue_family_;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &queue_priority;

        VkPhysicalDeviceFeatures enabled_features{};
        enabled_features.shaderFloat64 = VK_TRUE;
        VkPhysicalDevice16BitStorageFeatures storage_features{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES};
        VkPhysicalDeviceFeatures2 available_features{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        available_features.pNext = &storage_features;
        vkGetPhysicalDeviceFeatures2(
            physical_device_, &available_features);
        low_precision_available_ =
            storage_features.storageBuffer16BitAccess == VK_TRUE &&
            available_features.features.shaderInt16 == VK_TRUE;
        enabled_features.shaderInt16 =
            low_precision_available_ ? VK_TRUE : VK_FALSE;
        VkPhysicalDevice16BitStorageFeatures enabled_storage{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES};
        enabled_storage.storageBuffer16BitAccess =
            low_precision_available_ ? VK_TRUE : VK_FALSE;
        VkDeviceCreateInfo device_info{
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        device_info.pEnabledFeatures = &enabled_features;
        device_info.pNext =
            low_precision_available_ ? &enabled_storage : nullptr;
        check_vulkan(
            vkCreateDevice(
                physical_device_, &device_info, nullptr, &device_),
            "vkCreateDevice");
        vkGetDeviceQueue(device_, queue_family_, 0, &queue_);
        device_storage_pool_ =
            std::make_shared<VulkanStoragePool>(device_);
        staging_storage_pool_ =
            std::make_shared<VulkanStoragePool>(device_);

        std::array<VkDescriptorSetLayoutBinding, 4> bindings{};
        for (std::uint32_t index = 0; index < bindings.size(); ++index) {
            bindings[index].binding = index;
            bindings[index].descriptorType =
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[index].descriptorCount = 1;
            bindings[index].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        }
        VkDescriptorSetLayoutCreateInfo descriptor_info{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        descriptor_info.bindingCount =
            static_cast<std::uint32_t>(bindings.size());
        descriptor_info.pBindings = bindings.data();
        check_vulkan(
            vkCreateDescriptorSetLayout(
                device_, &descriptor_info, nullptr,
                &descriptor_set_layout_),
            "vkCreateDescriptorSetLayout");

        VkPushConstantRange push_range{};
        push_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        push_range.offset = 0;
        push_range.size = sizeof(DispatchParameters);
        VkPipelineLayoutCreateInfo layout_info{
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layout_info.setLayoutCount = 1;
        layout_info.pSetLayouts = &descriptor_set_layout_;
        layout_info.pushConstantRangeCount = 1;
        layout_info.pPushConstantRanges = &push_range;
        check_vulkan(
            vkCreatePipelineLayout(
                device_, &layout_info, nullptr, &pipeline_layout_),
            "vkCreatePipelineLayout");

        const std::array<const unsigned char*, 3> shader_codes{{
            coolbox_tensor_ops_fp64_spv,
            coolbox_tensor_ops_fp32_spv,
            coolbox_tensor_ops_lowp_spv,
        }};
        const std::array<std::size_t, 3> shader_sizes{{
            coolbox_tensor_ops_fp64_spv_size,
            coolbox_tensor_ops_fp32_spv_size,
            coolbox_tensor_ops_lowp_spv_size,
        }};
        for (std::size_t index = 0;
             index < shader_modules_.size();
             ++index) {
            if (index == 2 && !low_precision_available_) {
                continue;
            }
            VkShaderModuleCreateInfo shader_info{
                VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            shader_info.codeSize = shader_sizes[index];
            shader_info.pCode =
                reinterpret_cast<const std::uint32_t*>(
                    shader_codes[index]);
            check_vulkan(
                vkCreateShaderModule(
                    device_, &shader_info, nullptr,
                    &shader_modules_[index]),
                "vkCreateShaderModule");

            VkPipelineShaderStageCreateInfo stage_info{
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
            stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            stage_info.module = shader_modules_[index];
            stage_info.pName = "main";
            VkComputePipelineCreateInfo pipeline_info{
                VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
            pipeline_info.stage = stage_info;
            pipeline_info.layout = pipeline_layout_;
            check_vulkan(
                vkCreateComputePipelines(
                    device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                    &pipelines_[index]),
                "vkCreateComputePipelines");
        }

        VkCommandPoolCreateInfo command_pool_info{
            VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        command_pool_info.flags =
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        command_pool_info.queueFamilyIndex = queue_family_;
        check_vulkan(
            vkCreateCommandPool(
                device_, &command_pool_info, nullptr, &command_pool_),
            "vkCreateCommandPool");

        VkCommandBufferAllocateInfo command_info{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        command_info.commandPool = command_pool_;
        command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_info.commandBufferCount =
            static_cast<std::uint32_t>(submission_slots_.size());
        std::array<VkCommandBuffer, SUBMISSION_SLOT_COUNT>
            command_buffers{};
        check_vulkan(
            vkAllocateCommandBuffers(
                device_, &command_info, command_buffers.data()),
            "vkAllocateCommandBuffers");

        for (std::size_t index = 0;
             index < submission_slots_.size();
             ++index) {
            SubmissionSlot& slot = submission_slots_[index];
            slot.command_buffer = command_buffers[index];

            VkDescriptorPoolSize pool_size{};
            pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            pool_size.descriptorCount =
                4 * MAX_BATCH_DISPATCHES;
            VkDescriptorPoolCreateInfo pool_info{
                VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
            pool_info.maxSets = MAX_BATCH_DISPATCHES;
            pool_info.poolSizeCount = 1;
            pool_info.pPoolSizes = &pool_size;
            check_vulkan(
                vkCreateDescriptorPool(
                    device_, &pool_info, nullptr,
                    &slot.descriptor_pool),
                "vkCreateDescriptorPool");

            VkFenceCreateInfo fence_info{
                VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            check_vulkan(
                vkCreateFence(
                    device_, &fence_info, nullptr, &slot.fence),
                "vkCreateFence");
        }
    }

    void destroy() {
        if (device_ != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device_);
            if (batch_open_) {
                vkResetCommandBuffer(
                    submission_slots_[active_slot_].command_buffer,
                    0);
                batch_open_ = false;
            }
            for (SubmissionSlot& slot : submission_slots_) {
                slot.storage.clear();
                if (slot.fence != VK_NULL_HANDLE) {
                    vkDestroyFence(device_, slot.fence, nullptr);
                    slot.fence = VK_NULL_HANDLE;
                }
                if (slot.descriptor_pool != VK_NULL_HANDLE) {
                    vkDestroyDescriptorPool(
                        device_, slot.descriptor_pool, nullptr);
                    slot.descriptor_pool = VK_NULL_HANDLE;
                }
                slot.command_buffer = VK_NULL_HANDLE;
                slot.in_flight = false;
            }
            if (device_storage_pool_) {
                device_storage_pool_->shutdown();
            }
            if (staging_storage_pool_) {
                staging_storage_pool_->shutdown();
            }
            left_buffer_.destroy();
            right_buffer_.destroy();
            output_buffer_.destroy();
            if (command_pool_ != VK_NULL_HANDLE) {
                vkDestroyCommandPool(
                    device_, command_pool_, nullptr);
            }
            for (VkPipeline pipeline : pipelines_) {
                if (pipeline != VK_NULL_HANDLE) {
                    vkDestroyPipeline(device_, pipeline, nullptr);
                }
            }
            for (VkShaderModule shader_module : shader_modules_) {
                if (shader_module != VK_NULL_HANDLE) {
                    vkDestroyShaderModule(
                        device_, shader_module, nullptr);
                }
            }
            if (pipeline_layout_ != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(
                    device_, pipeline_layout_, nullptr);
            }
            if (descriptor_set_layout_ != VK_NULL_HANDLE) {
                vkDestroyDescriptorSetLayout(
                    device_, descriptor_set_layout_, nullptr);
            }
            if (device_storage_pool_) {
                device_storage_pool_->mark_device_destroyed();
            }
            if (staging_storage_pool_) {
                staging_storage_pool_->mark_device_destroyed();
            }
            vkDestroyDevice(device_, nullptr);
            device_ = VK_NULL_HANDLE;
            device_storage_pool_.reset();
            staging_storage_pool_.reset();
        }
        if (instance_ != VK_NULL_HANDLE) {
            vkDestroyInstance(instance_, nullptr);
            instance_ = VK_NULL_HANDLE;
        }
        available_ = false;
    }

    bool available_ = false;
    std::string unavailable_reason_;
    std::string device_name_;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue queue_ = VK_NULL_HANDLE;
    std::uint32_t queue_family_ = 0;
    bool low_precision_available_ = false;
    static constexpr std::size_t SUBMISSION_SLOT_COUNT = 3;
    static constexpr std::uint32_t MAX_BATCH_DISPATCHES = 256;
    static constexpr std::size_t MAX_BATCH_OUTPUT_BYTES =
        128ULL * 1024ULL * 1024ULL;
    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    std::array<VkShaderModule, 3> shader_modules_{};
    std::array<VkPipeline, 3> pipelines_{};
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    std::array<SubmissionSlot, SUBMISSION_SLOT_COUNT>
        submission_slots_{};
    std::shared_ptr<VulkanStoragePool> device_storage_pool_;
    std::shared_ptr<VulkanStoragePool> staging_storage_pool_;
    std::size_t active_slot_ = 0;
    Buffer left_buffer_;
    Buffer right_buffer_;
    Buffer output_buffer_;
    bool batch_open_ = false;
    std::uint32_t batch_dispatch_count_ = 0;
    std::size_t batch_output_bytes_ = 0;
    std::mutex dispatch_mutex_;
};

VulkanTensorRuntime& runtime() {
    static VulkanTensorRuntime instance;
    return instance;
}

} // namespace

bool vulkan_tensor_available() {
    return runtime().available();
}

std::string vulkan_tensor_unavailable_reason() {
    return runtime().unavailable_reason();
}

std::string vulkan_tensor_device_name() {
    return runtime().device_name();
}

bool vulkan_tensor_precision_available(TensorPrecision precision) {
    return runtime().available() &&
        runtime().precision_available(precision);
}

VulkanTensorStoragePtr vulkan_tensor_upload(
    const std::vector<double>& values) {
    return runtime().upload(values);
}

VulkanTensorStoragePtr vulkan_tensor_execute_resident(
    VulkanTensorOperation operation,
    const VulkanTensorStoragePtr& left,
    const VulkanTensorStoragePtr& right,
    std::size_t output_size,
    std::size_t rows,
    std::size_t columns,
    std::size_t inner,
    double scalar,
    const VulkanTensorStoragePtr& auxiliary) {
    return runtime().execute_resident(
        operation, left, right, output_size,
        rows, columns, inner, scalar, auxiliary);
}

std::vector<double> vulkan_tensor_download(
    const VulkanTensorStoragePtr& storage) {
    return runtime().download(storage);
}

bool vulkan_tensor_storage_matches_precision(
    const VulkanTensorStoragePtr& storage) {
    return runtime().storage_matches_precision(storage);
}

void vulkan_tensor_adamw_step_in_place(
    const VulkanTensorStoragePtr& values,
    const VulkanTensorStoragePtr& gradients,
    const VulkanTensorStoragePtr& first_moment,
    const VulkanTensorStoragePtr& second_moment,
    double learning_rate,
    double beta1,
    double beta2,
    double beta1_correction,
    double beta2_correction,
    double epsilon,
    double weight_decay,
    double gradient_scale) {
    runtime().adamw_step_in_place(
        values, gradients, first_moment, second_moment,
        learning_rate, beta1, beta2,
        beta1_correction, beta2_correction,
        epsilon, weight_decay, gradient_scale);
}

void vulkan_tensor_synchronize() {
    runtime().synchronize();
}

std::vector<double> vulkan_tensor_execute(
    VulkanTensorOperation operation,
    const std::vector<double>& left,
    const std::vector<double>& right,
    std::size_t output_size,
    std::size_t rows,
    std::size_t columns,
    std::size_t inner,
    double scalar) {
    return runtime().execute(
        operation, left, right, output_size, rows, columns, inner,
        scalar);
}

} // namespace detail
} // namespace deep_learning
} // namespace ml
