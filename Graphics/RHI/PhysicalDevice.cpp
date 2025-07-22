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
		, mQueueFamilies()
        , mLogicalDevice(nullptr)
    {
        assert(mPhysicalDevice != VK_NULL_HANDLE);
        vkGetPhysicalDeviceProperties2(mPhysicalDevice, const_cast<VkPhysicalDeviceProperties2*>(&mProperties.PhysicalDeviceProperties));
        vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &mProperties.MemoryProperties);

        PrintProperties();

        CGS_LOG_INFO("Creating queue families for the physical device %s...", GetName());
        createQueueFamilies();
        CGS_LOG_INFO("Physical device %s has %zu queue families.", GetName(), mQueueFamilies.size());
        CGS_LOG_INFO("Creating logical device for the physical device %s...", GetName());

        if (createInfo.bCreateLogicalDevice)
        {
            createLogicalDevice();
        }
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
        float deviceTypeScore = 0.0f;
        switch (mProperties.PhysicalDeviceProperties.properties.deviceType)
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
            CGS_LOG_ERROR("Unknown physical device type: %d", static_cast<int>(mProperties.PhysicalDeviceProperties.properties.deviceType));
            break;
        }

        score += deviceTypeScore;
        return score;
    }

    uint32_t PhysicalDevice::GetMemoryTypeIndex(const uint32_t typeBits, const VkMemoryPropertyFlags memoryPropertyFlags) const noexcept
    {
        for (uint32_t i = 0; i < mProperties.MemoryProperties.memoryTypeCount; ++i)
        {
            if ((typeBits & (1 << i)) && (mProperties.MemoryProperties.memoryTypes[i].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags)
            {
                return i;
            }
        }

        CGS_LOG_ERROR("Failed to find suitable memory type.");
        return UINT32_MAX; // Return an invalid index if no suitable memory type is found.
    }

    void PhysicalDevice::PrintProperties() const noexcept
    {
        printDeviceProperties(mProperties);
    }

    bool PhysicalDevice::IsPresentSupported(const uint32_t queueFamilyIndex) const noexcept
    {
#if defined(CGS_WIN32)
        const VkBool32 result = vkGetPhysicalDeviceWin32PresentationSupportKHR(mPhysicalDevice, queueFamilyIndex);
#elif defined(CGS_UNIX)
        const VkBool32 result = vkGetPhysicalDeviceWaylandPresentationSupportKHR(mPhysicalDevice, queueFamilyIndex, nullptr);
#else
        CGS_LOG_ERROR("Presentation support is not implemented for this platform.");
        return false;
#endif
        return result == VK_TRUE;
    }

    void PhysicalDevice::printDeviceProperties([[maybe_unused]] const Properties& properties) noexcept
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

        struct QueueFamilyComparator final
        {
            CGS_INLINE bool operator()(const std::unique_ptr<QueueFamily>& lhs, const std::unique_ptr<QueueFamily>& rhs) const noexcept
            {
                return (lhs == nullptr || rhs == nullptr) || (lhs->EvaluateScore() < rhs->EvaluateScore());
            }
        };

        std::priority_queue<std::unique_ptr<QueueFamily>, std::vector<std::unique_ptr<QueueFamily>>, QueueFamilyComparator> queueFamiliesToCreate;
        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            QueueFamily::CreateInfo queueFamilyCreateInfo =
            {
                .RhiPhysicalDevice = *this,
                .QueueFamilyProperties = queueFamilyPropertiesList[i],
                .Index = i,
            };
            std::unique_ptr<QueueFamily> queueFamily = std::make_unique<QueueFamily>(queueFamilyCreateInfo);
            queueFamiliesToCreate.push(std::move(queueFamily));
        }

        mQueueFamilies.reserve(queueFamilyCount);
        while (!queueFamiliesToCreate.empty())
        {
            auto queueFamily = std::move(const_cast<std::unique_ptr<QueueFamily>&>(queueFamiliesToCreate.top()));
            mQueueFamilies.push_back(std::move(queueFamily));
            queueFamiliesToCreate.pop();
        }
    }

    void PhysicalDevice::createLogicalDevice() noexcept
    {
        VkResult vr = VK_SUCCESS;

        uint32_t mainPhysicalDeviceGroupIndex = 0;
        const bool result = mInstance.GetConfig().GetSetting(CONFIG_PHYSICAL_DEVICE_GROUP_INDEX, mainPhysicalDeviceGroupIndex);
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
        std::vector<std::vector<float>> queuePrioritiesList;
        for (const auto& queueFamily : mQueueFamilies)
        {
            queuePrioritiesList.emplace_back(std::vector<float>(queueFamily->GetQueueCount(), queueFamily->EvaluateScore()));
            VkDeviceQueueCreateInfo queueCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .queueFamilyIndex = queueFamily->GetIndex(),
                .queueCount = queueFamily->GetQueueCount(),
                .pQueuePriorities = queuePrioritiesList.back().data(),
            };
            queueCreateInfos.push_back(queueCreateInfo);
        }

        std::vector<const char *> deviceExtensions = 
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME, // Swapchain extension is commonly used
        };

        void* pNext = nullptr;
        VkPhysicalDeviceVulkan14Features vulkan14Features = 
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
            .pNext = pNext, // Link to Vulkan 1.3 features
        };
        pNext = &vulkan14Features;

        VkPhysicalDeviceVulkan13Features vulkan13Features = 
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = pNext,
            .synchronization2 = VK_TRUE, // Enable synchronization 2
            .dynamicRendering = VK_TRUE, // Enable dynamic rendering
        };
        pNext = &vulkan13Features;

        VkPhysicalDeviceVulkan12Features vulkan12Features = 
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .pNext = pNext,
        };
        pNext = &vulkan12Features;

        VkPhysicalDeviceVulkan11Features vulkan11Features = 
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
            .pNext = pNext,
        };
        pNext = &vulkan11Features;

        VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = 
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = pNext,
        };
        pNext = &physicalDeviceFeatures2;

        VkDeviceCreateInfo deviceCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = pNext,
            .flags = 0,
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledLayerCount = 0, // Deprecated, should not be used
            .ppEnabledLayerNames = nullptr, // Deprecated, should not be used
            .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()), // Should be filled with enabled extensions
            .ppEnabledExtensionNames = deviceExtensions.data(), // Should be filled with enabled extension names
            .pEnabledFeatures = nullptr // Should be filled with enabled features
        };

        Device::CreateInfo logicalDeviceCreateInfo =
        {
            .RhiPhysicalDevice = *this,
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
