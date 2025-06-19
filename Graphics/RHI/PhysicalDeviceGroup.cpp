#include "Graphics/pch.h"

#include "Graphics/RHI/PhysicalDeviceGroup.h"
#include "Graphics/RHI/Instance.h"
#include "Graphics/RHI/PhysicalDevice.h"

namespace cgs::graphics::rhi
{
    PhysicalDeviceGroup::PhysicalDeviceGroup(CreateInfo& createInfo) noexcept
        : mInstance(createInfo.RhiInstance)
        , mIndex(createInfo.Index)
        , mPhysicalDeviceGroupProperties(createInfo.PhysicalDeviceGroupProperties)
        , mPhysicalDevices()
    {
        assert(mPhysicalDeviceGroupProperties.sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES);
        PrintProperties();
        CGS_LOG_INFO("Creating physical devices for the physical device group...");
        assert(mPhysicalDeviceGroupProperties.physicalDeviceCount > 0);
        assert(mPhysicalDeviceGroupProperties.physicalDeviceCount <= VK_MAX_DEVICE_GROUP_SIZE);
        assert(mPhysicalDeviceGroupProperties.subsetAllocation == VK_FALSE || mPhysicalDeviceGroupProperties.subsetAllocation == VK_TRUE);
        createPhysicalDevices(createInfo.bCreateLogicalDevice);
    }

    PhysicalDeviceGroup::~PhysicalDeviceGroup() noexcept
    {
        mPhysicalDevices.clear();
    }

    void PhysicalDeviceGroup::PrintProperties() const noexcept
    {
        CGS_LOG_INFO("Physical Device Group Properties:"
                    "\n\tPhysical Device Count: %u"
                    "\n\tSubset Allocation: %s"
                    , mPhysicalDeviceGroupProperties.physicalDeviceCount
                    , mPhysicalDeviceGroupProperties.subsetAllocation ? "true" : "false"
                );

        for (uint32_t i = 0; i < mPhysicalDeviceGroupProperties.physicalDeviceCount; ++i)
        {
            CGS_LOG_INFO("\tPhysical Device %u: %p", i, mPhysicalDeviceGroupProperties.physicalDevices[i]);
        }
    }

    void PhysicalDeviceGroup::createPhysicalDevices(const bool bCreateLogicalDevice) noexcept
    {
        const uint32_t physicalDeviceCount = mPhysicalDeviceGroupProperties.physicalDeviceCount;
        CGS_LOG_INFO("Physical device group contains %u physical devices.", physicalDeviceCount);

        std::string deviceName;
        mInstance.GetConfig().GetSetting(CONFIG_PHYSICAL_DEVICE, deviceName);
        if (!deviceName.empty())
        {
            for (uint32_t i = 0; i < physicalDeviceCount; ++i)
            {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(mPhysicalDeviceGroupProperties.physicalDevices[i], &properties);
                std::string currentDeviceName(properties.deviceName);
                if (deviceName == currentDeviceName)
                {
                    PhysicalDevice::CreateInfo physicalDeviceCreateInfo =
                    {
                        .RhiInstance = mInstance,
                        .RhiPhysicalDeviceGroup = *this,
                        .PhysicalDevice = mPhysicalDeviceGroupProperties.physicalDevices[i],
                        .bCreateLogicalDevice = bCreateLogicalDevice,
                    };
                    std::unique_ptr<PhysicalDevice> device = std::make_unique<PhysicalDevice>(physicalDeviceCreateInfo);
                    assert(device->mPhysicalDevice != VK_NULL_HANDLE);
                    mPhysicalDevices.push_back(std::move(device));
                    break;
                }
            }
        }
        else
        {
            struct PhysicalDeviceComparator final
            {
                CGS_INLINE bool operator()(const std::unique_ptr<PhysicalDevice> &lhs, const std::unique_ptr<PhysicalDevice> &rhs) const noexcept
                {
                    return (lhs == nullptr || rhs == nullptr) || (lhs->EvaluateScore() < rhs->EvaluateScore());
                }
            };

            std::priority_queue<std::unique_ptr<PhysicalDevice>, std::vector<std::unique_ptr<PhysicalDevice>>, PhysicalDeviceComparator> physicalDevicesToCreate;
            for (uint32_t i = 0; i < physicalDeviceCount; ++i)
            {
                PhysicalDevice::CreateInfo physicalDeviceCreateInfo =
                {
                    .RhiInstance = mInstance,
                    .RhiPhysicalDeviceGroup = *this,
                    .PhysicalDevice = mPhysicalDeviceGroupProperties.physicalDevices[i],
                    .bCreateLogicalDevice = bCreateLogicalDevice,
                };
                std::unique_ptr<PhysicalDevice> physicalDevice = std::make_unique<PhysicalDevice>(physicalDeviceCreateInfo);
                assert(physicalDevice->mPhysicalDevice != VK_NULL_HANDLE);
                physicalDevicesToCreate.push(std::move(physicalDevice));
            }

            mPhysicalDevices.reserve(physicalDeviceCount);
            while (!physicalDevicesToCreate.empty())
            {
                auto device = std::move(const_cast<std::unique_ptr<PhysicalDevice>&>(physicalDevicesToCreate.top()));
                mPhysicalDevices.push_back(std::move(device));
                physicalDevicesToCreate.pop();
            }
        }
    }
} // namespace cgs::graphics::rhi
