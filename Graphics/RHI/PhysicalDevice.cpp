#include "Graphics/RHI/PhysicalDevice.h"

#include "Graphics/RHI/Instance.h"

namespace cgs::graphics::rhi
{
    PhysicalDevice::PhysicalDevice(const CreateInfo& createInfo) noexcept
        : mInstance(createInfo.Instance)
        , mPhysicalDevice(createInfo.PhysicalDevice)
    {
        assert(mPhysicalDevice != VK_NULL_HANDLE);
    }

    PhysicalDevice::~PhysicalDevice() noexcept
    {
        mPhysicalDevice = VK_NULL_HANDLE;
    }

    float PhysicalDevice::EvaluateScore() const noexcept
    {
        float score = 0.0f;
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(mPhysicalDevice, &properties);

        float deviceTypeScore = 0.0f;
        switch (properties.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                deviceTypeScore = 0.0f; // Lowest score for unknown devices.
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                deviceTypeScore = 1.0f; // Integrated GPUs are generally less powerful.
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                deviceTypeScore = 2.0f; // Discrete GPUs are typically more powerful.
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                deviceTypeScore = 1.5f; // Virtual GPUs can vary in performance.
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                deviceTypeScore = 0.5f; // CPUs are not suitable for graphics tasks.
        default:
            break;
        }

        score += deviceTypeScore;
        return score;
    }
}