#include "Graphics/RHI/PhysicalDevice.h"

#include "Graphics/RHI/Device.h"
#include "Graphics/RHI/Instance.h"

namespace cgs::graphics::rhi
{
    PhysicalDevice::PhysicalDevice(const CreateInfo& createInfo) noexcept
        : mInstance(createInfo.RhiInstance)
        , mPhysicalDevice(createInfo.PhysicalDevice)
    {
        assert(mPhysicalDevice != VK_NULL_HANDLE);

        PrintProperties();
    }

    PhysicalDevice::~PhysicalDevice() noexcept
    {
        if (mLogicalDevice != nullptr)
        {
            vkDestroyDevice(mLogicalDevice->mDevice, nullptr);
            mLogicalDevice->mDevice = VK_NULL_HANDLE;
            mLogicalDevice.reset();
        }

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

    void PhysicalDevice::PrintProperties() const noexcept
    {
        VkPhysicalDeviceProperties2 properties;
        vkGetPhysicalDeviceProperties2(mPhysicalDevice, &properties);
        printDeviceProperties(properties);
    }

    void PhysicalDevice::printDeviceProperties(const VkPhysicalDeviceProperties2& properties) noexcept
    {
        CGS_LOG_INFO("Physical Device Properties:");
        CGS_LOG_INFO("\tAPI Version: %u.%u.%u.%u", VK_API_VERSION_VARIANT(properties.properties.apiVersion),
                     VK_API_VERSION_MAJOR(properties.properties.apiVersion),
                     VK_API_VERSION_MINOR(properties.properties.apiVersion),
                     VK_API_VERSION_PATCH(properties.properties.apiVersion));
        CGS_LOG_INFO("\tDriver Version: %u", properties.properties.driverVersion);
        CGS_LOG_INFO("\tVendor ID: %u", properties.properties.vendorID);
        CGS_LOG_INFO("\tDevice ID: %u", properties.properties.deviceID);
        CGS_LOG_INFO("\tDevice Type: %u", static_cast<int>(properties.properties.deviceType));
        CGS_LOG_INFO("\tDevice Name: %s", properties.properties.deviceName);
    }
}