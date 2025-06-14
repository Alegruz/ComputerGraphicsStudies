#include "Graphics/pch.h"

#include "Graphics/RHI/Device.h"

#include "Graphics/RHI/CommandPool.h"
#include "Graphics/RHI/PhysicalDevice.h"
#include "Graphics/RHI/QueueFamily.h"

namespace cgs::graphics::rhi
{
    Device::Device(const CreateInfo& createInfo) noexcept
        : mPhysicalDevice(createInfo.RhiPhysicalDevice)
        , mDevice(createInfo.Device)
    {
        assert(mDevice != VK_NULL_HANDLE);
    }

    Device::~Device() noexcept
    {
        mPhysicalDevice.DestroyLogicalDevice(mDevice);
    }

    void Device::Destroy(CommandPool& inoutCommandPool) const noexcept
    {
        if (inoutCommandPool.mCommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(mDevice, inoutCommandPool.mCommandPool, nullptr);
            inoutCommandPool.mCommandPool = VK_NULL_HANDLE;
        }
    }

    void Device::Reset(CommandPool& inoutCommandPool) const noexcept
    {
        if (inoutCommandPool.mCommandPool != VK_NULL_HANDLE)
        {
            VkResult vr = VK_SUCCESS;
            vr = vkResetCommandPool(mDevice, inoutCommandPool.mCommandPool, 0);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to reset command pool: %s", VkResultToString(vr));
            }
        }
    }

    void Device::Trim(CommandPool& inoutCommandPool) const noexcept
    {
        if (inoutCommandPool.mCommandPool != VK_NULL_HANDLE)
        {
            vkTrimCommandPool(mDevice, inoutCommandPool.mCommandPool, 0);
        }
    }

    void Device::createCommandPools() noexcept
    {
        VkResult vr = VK_SUCCESS;

        for (const auto& queueFamily : mPhysicalDevice.GetQueueFamilies())
        {
            CommandPool::CreateInfo commandPoolCreateInfo =
            {
                .RhiDevice = *this,
                .CommandPool = VK_NULL_HANDLE // Will be created later
            };

            VkCommandPoolCreateInfo commandPoolInfo =
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // Allows command buffers to be reset
                .queueFamilyIndex = queueFamily->GetIndex() // Use the index of the queue family
            };
            vr = vkCreateCommandPool(mDevice, &commandPoolInfo, nullptr, &commandPoolCreateInfo.CommandPool);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to create command pool for queue family %u: %s", queueFamily->GetIndex(), VkResultToString(vr));
                continue; // Skip this queue family if command pool creation fails
            }
            CGS_LOG_INFO("Created command pool for queue family %u: %p", queueFamily->GetIndex(), commandPoolCreateInfo.CommandPool);
            // Create the command pool and add it to the list

            auto commandPool = std::make_unique<CommandPool>(commandPoolCreateInfo);
            mCommandPools.push_back(std::move(commandPool));
        }
    }
} 