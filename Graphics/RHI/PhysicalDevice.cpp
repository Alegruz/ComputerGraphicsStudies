#include "Graphics/pch.h"

#include "Graphics/RHI/PhysicalDevice.h"

#include "Graphics/RHI/Device.h"
#include "Graphics/RHI/Instance.h"
#include "Graphics/RHI/PhysicalDeviceGroup.h"
#include "Graphics/RHI/Queue.h"
#include "Graphics/RHI/QueueFamily.h"

namespace cgs::graphics::rhi
{
    PhysicalDevice::PhysicalDevice(const CreateInfo& createInfo) noexcept
        : mInstance(createInfo.RhiInstance)
        , mPhysicalDeviceGroup(createInfo.RhiPhysicalDeviceGroup)
        , mPhysicalDevice(createInfo.PhysicalDevice)
        , mProperties()
        , mLogicalDevice(nullptr)
    {
        assert(mPhysicalDevice != VK_NULL_HANDLE);
        vkGetPhysicalDeviceProperties2(mPhysicalDevice, const_cast<VkPhysicalDeviceProperties2*>(&mProperties.PhysicalDeviceProperties));

        PrintProperties();

        CGS_LOG_INFO("Creating queue families for the physical device %s...", GetName());
        createQueueFamilies();
        CGS_LOG_INFO("Physical device %s has %zu queue families.", GetName(), mQueueFamilies.size());
        CGS_LOG_INFO("Creating logical device for the physical device %s...", GetName());
        createLogicalDevice();
    }

    PhysicalDevice::~PhysicalDevice() noexcept
    {
        if (mLogicalDevice != nullptr)
        {
            mLogicalDevice.reset();
        }

        mPhysicalDevice = VK_NULL_HANDLE;
    }

    void PhysicalDevice::DestroyLogicalDevice(VkDevice& inoutDevice) const noexcept
    {
        if (inoutDevice != VK_NULL_HANDLE)
        {
            vkDestroyDevice(inoutDevice, nullptr);
            inoutDevice = VK_NULL_HANDLE;
            CGS_LOG_INFO("Logical device destroyed successfully.");
        }
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
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                deviceTypeScore = 1.0f; // Integrated GPUs are generally less powerful.
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                deviceTypeScore = 2.0f; // Discrete GPUs are typically more powerful.
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                deviceTypeScore = 1.5f; // Virtual GPUs can vary in performance.
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                deviceTypeScore = 0.5f; // CPUs are not suitable for graphics tasks.
                break;
        default:
            CGS_LOG_ERROR("Unknown physical device type: %d", static_cast<int>(properties.deviceType));
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

    void PhysicalDevice::createQueueFamilies() noexcept
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties2(mPhysicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties2> queueFamilyPropertiesList(queueFamilyCount, { .sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2 });
        vkGetPhysicalDeviceQueueFamilyProperties2(mPhysicalDevice, &queueFamilyCount, queueFamilyPropertiesList.data());
        CGS_LOG_INFO("Physical device %s has %u queue families.", GetName(), queueFamilyCount);

        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            QueueFamily::CreateInfo queueFamilyCreateInfo =
            {
                .RhiPhysicalDevice = *this,
                .QueueFamilyProperties = queueFamilyPropertiesList[i],
            };
            mQueueFamilies.emplace_back(std::make_unique<QueueFamily>(queueFamilyCreateInfo));
        }
    }

    void PhysicalDevice::createLogicalDevice() noexcept
    {
        VkResult vr = VK_SUCCESS;

        uint32_t mainPhysicalDeviceGroupIndex = 0;
        const bool result = mInstance.GetConfig().GetSetting("MainPhysicalDeviceGroupIndex", mainPhysicalDeviceGroupIndex);
        if (!result)
        {
            mainPhysicalDeviceGroupIndex = PhysicalDeviceGroup::DEFAULT_INDEX; // Default to the first physical device group if not set
        }
        
        if (mainPhysicalDeviceGroupIndex != mPhysicalDeviceGroup.GetIndex())
        {
            return;
        }

        // Prepare queue create infos
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        for (const auto& queueFamily : mQueueFamilies)
        {
            const std::vector<float> queuePriorities(queueFamily->GetQueueCount(), queueFamily->EvaluateScore());
            VkDeviceQueueCreateInfo queueCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .queueFamilyIndex = queueFamily->GetIndex(),
                .queueCount = queueFamily->GetQueueCount(),
                .pQueuePriorities = queuePriorities.data(),
            };
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkDeviceCreateInfo deviceCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledLayerCount = 0, // Deprecated, should not be used
            .ppEnabledLayerNames = nullptr, // Deprecated, should not be used
            .enabledExtensionCount = 0, // Should be filled with enabled extensions
            .ppEnabledExtensionNames = nullptr, // Should be filled with enabled extension names
            .pEnabledFeatures = nullptr // Should be filled with enabled features
        };

        Device::CreateInfo logicalDeviceCreateInfo =
        {
            .RhiPhysicalDevice = *this,
            .Device = VK_NULL_HANDLE // This will be filled by vkCreateDevice
        };
        vr = vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &logicalDeviceCreateInfo.Device);
        if (vr != VK_SUCCESS)
        {
            CGS_LOG_ERROR("Failed to create logical device for physical device %s", GetName());
            return;
        }

        volkLoadDevice(logicalDeviceCreateInfo.Device);
        CGS_LOG_INFO("Logical device created successfully for physical device %s.", GetName());
        mLogicalDevice = std::make_unique<Device>(logicalDeviceCreateInfo);

        for (auto& queueFamily : mQueueFamilies)
        {
            for (uint32_t i = 0; i < queueFamily->GetQueueCount(); ++i)
            {
                Queue::CreateInfo queueCreateInfo =
                {
                    .RhiDevice = *mLogicalDevice.get(),
                    .RhiQueueFamily = *queueFamily.get(),
                    .Queue = VK_NULL_HANDLE // This will be filled by vkGetDeviceQueue
                };

                const VkDeviceQueueInfo2 queueFamilyCreateInfo =
                {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
                    .pNext = nullptr,
                    .flags = 0,
                    .queueFamilyIndex = queueFamily->GetIndex(),
                    .queueIndex = i
                };

                vkGetDeviceQueue2(logicalDeviceCreateInfo.Device, &queueFamilyCreateInfo, &queueCreateInfo.Queue);
                queueFamily->mQueues.emplace_back(std::make_unique<Queue>(queueCreateInfo));
            }
        }
    }
}
