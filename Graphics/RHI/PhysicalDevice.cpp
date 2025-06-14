#include "Graphics/RHI/PhysicalDevice.h"

#include "Graphics/RHI/Device.h"
#include "Graphics/RHI/Instance.h"

namespace cgs::graphics::rhi
{
    PhysicalDevice::PhysicalDevice(const CreateInfo& createInfo) noexcept
        : mInstance(createInfo.RhiInstance)
        , mPhysicalDevice(createInfo.PhysicalDevice)
        , mProperties()
        , mLogicalDevice(nullptr)
    {
        assert(mPhysicalDevice != VK_NULL_HANDLE);
        vkGetPhysicalDeviceProperties2(mPhysicalDevice, const_cast<VkPhysicalDeviceProperties2*>(&mProperties.PhysicalDeviceProperties));

        PrintProperties();

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties2(mPhysicalDevice, &queueFamilyCount, nullptr);
        mQueueFamilyPropertiesList.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties2(mPhysicalDevice, &queueFamilyCount, mQueueFamilyPropertiesList.data());
        CGS_LOG_INFO("Physical device %s has %u queue families.", GetName(), queueFamilyCount);
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
        printDeviceProperties(mProperties);
    }

    void PhysicalDevice::printDeviceProperties(const Properties& properties) noexcept
    {
        CGS_LOG_INFO("Physical Device Properties:"
                    "\n\tAPI Version: %u.%u.%u.%u"
                    "\n\tDriver Version: %u"
                    "\n\tVendor ID: %s"
                    "\n\tDevice ID: %u"
                    "\n\tDevice Type: %s"
                    "\n\tDevice Name: %s"
                    "\n\tDriver ID: %s"
                    "\n\tDriver Name: %s"
                    "\n\tDriver Info: %s"
                    , VK_API_VERSION_VARIANT(properties.PhysicalDeviceProperties.properties.apiVersion)
                    , VK_API_VERSION_MAJOR(properties.PhysicalDeviceProperties.properties.apiVersion)
                    , VK_API_VERSION_MINOR(properties.PhysicalDeviceProperties.properties.apiVersion)
                    , VK_API_VERSION_PATCH(properties.PhysicalDeviceProperties.properties.apiVersion)
                    , properties.PhysicalDeviceProperties.properties.driverVersion
                    , getVendorIdName(static_cast<VkVendorId>(properties.PhysicalDeviceProperties.properties.vendorID))
                    , properties.PhysicalDeviceProperties.properties.deviceID
                    , getTypeName(properties.PhysicalDeviceProperties.properties.deviceType)
                    , properties.PhysicalDeviceProperties.properties.deviceName
                    , getDriverIdName(properties.Vulkan12Properties.driverID)
                    , properties.Vulkan12Properties.driverName
                    , properties.Vulkan12Properties.driverInfo
                );
    }
}