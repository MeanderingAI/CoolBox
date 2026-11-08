#include "../include/vulkan_tensor_backend.h"

#include "vulkan_tensor_shader.h"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ml {
namespace deep_learning {
namespace detail {
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
};

static_assert(sizeof(DispatchParameters) == 7 * sizeof(std::uint32_t),
              "Unexpected Vulkan push-constant layout");

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

        std::lock_guard<std::mutex> lock(dispatch_mutex_);
        Buffer left_buffer(*this, left);
        const std::vector<double> right_data =
            right.empty() ? std::vector<double>{0.0} : right;
        Buffer right_buffer(*this, right_data);
        Buffer output_buffer(*this, output_size);

        VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
        VkCommandBuffer command_buffer = VK_NULL_HANDLE;
        try {
            VkDescriptorPoolSize pool_size{};
            pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            pool_size.descriptorCount = 3;

            VkDescriptorPoolCreateInfo pool_info{
                VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
            pool_info.maxSets = 1;
            pool_info.poolSizeCount = 1;
            pool_info.pPoolSizes = &pool_size;
            check_vulkan(
                vkCreateDescriptorPool(
                    device_, &pool_info, nullptr, &descriptor_pool),
                "vkCreateDescriptorPool");

            VkDescriptorSetAllocateInfo set_info{
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            set_info.descriptorPool = descriptor_pool;
            set_info.descriptorSetCount = 1;
            set_info.pSetLayouts = &descriptor_set_layout_;
            VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
            check_vulkan(
                vkAllocateDescriptorSets(device_, &set_info, &descriptor_set),
                "vkAllocateDescriptorSets");

            const std::array<VkDescriptorBufferInfo, 3> buffer_infos{{
                {left_buffer.buffer, 0, VK_WHOLE_SIZE},
                {right_buffer.buffer, 0, VK_WHOLE_SIZE},
                {output_buffer.buffer, 0, VK_WHOLE_SIZE},
            }};
            std::array<VkWriteDescriptorSet, 3> writes{};
            for (std::uint32_t index = 0; index < writes.size(); ++index) {
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

            VkCommandBufferAllocateInfo command_info{
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
            command_info.commandPool = command_pool_;
            command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            command_info.commandBufferCount = 1;
            check_vulkan(
                vkAllocateCommandBuffers(
                    device_, &command_info, &command_buffer),
                "vkAllocateCommandBuffers");

            VkCommandBufferBeginInfo begin_info{
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            check_vulkan(
                vkBeginCommandBuffer(command_buffer, &begin_info),
                "vkBeginCommandBuffer");

            vkCmdBindPipeline(
                command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
            vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                pipeline_layout_, 0, 1, &descriptor_set, 0, nullptr);

            std::uint64_t scalar_bits = 0;
            std::memcpy(&scalar_bits, &scalar, sizeof(scalar_bits));
            const DispatchParameters parameters{
                static_cast<std::uint32_t>(operation),
                static_cast<std::uint32_t>(output_size),
                static_cast<std::uint32_t>(rows),
                static_cast<std::uint32_t>(columns),
                static_cast<std::uint32_t>(inner),
                static_cast<std::uint32_t>(scalar_bits),
                static_cast<std::uint32_t>(scalar_bits >> 32),
            };
            vkCmdPushConstants(
                command_buffer, pipeline_layout_,
                VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(parameters),
                &parameters);

            const std::uint32_t group_count =
                (parameters.count + 255U) / 256U;
            vkCmdDispatch(command_buffer, group_count, 1, 1);
            check_vulkan(
                vkEndCommandBuffer(command_buffer),
                "vkEndCommandBuffer");

            VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffer;
            check_vulkan(
                vkQueueSubmit(queue_, 1, &submit_info, VK_NULL_HANDLE),
                "vkQueueSubmit");
            check_vulkan(vkQueueWaitIdle(queue_), "vkQueueWaitIdle");

            std::vector<double> result = output_buffer.read(output_size);
            vkFreeCommandBuffers(
                device_, command_pool_, 1, &command_buffer);
            vkDestroyDescriptorPool(
                device_, descriptor_pool, nullptr);
            return result;
        } catch (...) {
            if (command_buffer != VK_NULL_HANDLE) {
                vkFreeCommandBuffers(
                    device_, command_pool_, 1, &command_buffer);
            }
            if (descriptor_pool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(
                    device_, descriptor_pool, nullptr);
            }
            throw;
        }
    }

private:
    struct Buffer {
        VkDevice device = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize byte_size = 0;

        Buffer(
            VulkanTensorRuntime& runtime,
            const std::vector<double>& values)
            : device(runtime.device_),
              byte_size(values.size() * sizeof(double)) {
            create(runtime, byte_size);
            void* mapped = nullptr;
            check_vulkan(
                vkMapMemory(device, memory, 0, byte_size, 0, &mapped),
                "vkMapMemory");
            std::memcpy(mapped, values.data(), byte_size);
            vkUnmapMemory(device, memory);
        }

        Buffer(VulkanTensorRuntime& runtime, std::size_t element_count)
            : device(runtime.device_),
              byte_size(element_count * sizeof(double)) {
            create(runtime, byte_size);
        }

        ~Buffer() {
            if (buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(device, buffer, nullptr);
            }
            if (memory != VK_NULL_HANDLE) {
                vkFreeMemory(device, memory, nullptr);
            }
        }

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        std::vector<double> read(std::size_t element_count) {
            std::vector<double> result(element_count);
            void* mapped = nullptr;
            check_vulkan(
                vkMapMemory(device, memory, 0, byte_size, 0, &mapped),
                "vkMapMemory");
            std::memcpy(result.data(), mapped, byte_size);
            vkUnmapMemory(device, memory);
            return result;
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
            "Vulkan device has no host-visible coherent memory type");
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
        VkDeviceCreateInfo device_info{
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        device_info.pEnabledFeatures = &enabled_features;
        check_vulkan(
            vkCreateDevice(
                physical_device_, &device_info, nullptr, &device_),
            "vkCreateDevice");
        vkGetDeviceQueue(device_, queue_family_, 0, &queue_);

        std::array<VkDescriptorSetLayoutBinding, 3> bindings{};
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

        VkShaderModuleCreateInfo shader_info{
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        shader_info.codeSize = coolbox_tensor_ops_spv_size;
        shader_info.pCode = reinterpret_cast<const std::uint32_t*>(
            coolbox_tensor_ops_spv);
        check_vulkan(
            vkCreateShaderModule(
                device_, &shader_info, nullptr, &shader_module_),
            "vkCreateShaderModule");

        VkPipelineShaderStageCreateInfo stage_info{
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stage_info.module = shader_module_;
        stage_info.pName = "main";

        VkComputePipelineCreateInfo pipeline_info{
            VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        pipeline_info.stage = stage_info;
        pipeline_info.layout = pipeline_layout_;
        check_vulkan(
            vkCreateComputePipelines(
                device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                &pipeline_),
            "vkCreateComputePipelines");

        VkCommandPoolCreateInfo command_pool_info{
            VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        command_pool_info.flags =
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        command_pool_info.queueFamilyIndex = queue_family_;
        check_vulkan(
            vkCreateCommandPool(
                device_, &command_pool_info, nullptr, &command_pool_),
            "vkCreateCommandPool");
    }

    void destroy() {
        if (device_ != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device_);
            if (command_pool_ != VK_NULL_HANDLE) {
                vkDestroyCommandPool(
                    device_, command_pool_, nullptr);
            }
            if (pipeline_ != VK_NULL_HANDLE) {
                vkDestroyPipeline(device_, pipeline_, nullptr);
            }
            if (shader_module_ != VK_NULL_HANDLE) {
                vkDestroyShaderModule(
                    device_, shader_module_, nullptr);
            }
            if (pipeline_layout_ != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(
                    device_, pipeline_layout_, nullptr);
            }
            if (descriptor_set_layout_ != VK_NULL_HANDLE) {
                vkDestroyDescriptorSetLayout(
                    device_, descriptor_set_layout_, nullptr);
            }
            vkDestroyDevice(device_, nullptr);
            device_ = VK_NULL_HANDLE;
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
    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkShaderModule shader_module_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
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
