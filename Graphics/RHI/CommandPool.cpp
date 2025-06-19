#include "Graphics/pch.h"

#include "Graphics/RHI/CommandPool.h"

#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/Device.h"
#include "Graphics/RHI/SwapChain.h"

namespace cgs::graphics::rhi
{
    CommandPool::CommandPool(const CreateInfo& createInfo) noexcept
        : mDevice(createInfo.RhiDevice)
        , mCommandPool(createInfo.CommandPool)
        , mCommandBuffers()
    {
        assert(mCommandPool != VK_NULL_HANDLE);
        AllocateCommandBuffer(); // Allocate the first command buffer upon creation
    }

    CommandPool::~CommandPool() noexcept
    {
        mCommandBuffers.clear();

        if (mCommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(mDevice.GetVkDevice(), mCommandPool, nullptr);
            mCommandPool = VK_NULL_HANDLE;
        }
    }

    void CommandPool::AllocateCommandBuffer() noexcept
    {
        assert(mCommandPool != VK_NULL_HANDLE);

        const SwapChain& swapChain = mDevice.GetSwapChain();
        const uint32_t frameBufferCount = swapChain.GetBackBufferCount();

        for (uint32_t i = 0; i < frameBufferCount; ++i)
        {
            // Allocate command buffers for each frame buffer
            CommandBuffer::CreateInfo commandBufferCreateInfo =
            {
                .RhiCommandPool = *this,
                .Index = static_cast<uint32_t>(mCommandBuffers.size()), // Use the current size as the index
                .FrameBufferIndex = i, // Set the frame buffer index
            };

            VkCommandBufferAllocateInfo allocateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = mCommandPool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, // Primary command buffer
                .commandBufferCount = 1 // Allocate one command buffer
            };
            
            VkResult vr = vkAllocateCommandBuffers(mDevice.GetVkDevice(), &allocateInfo, &commandBufferCreateInfo.CommandBuffer);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to allocate command buffer: %s", VkResultToString(vr));
                continue; // Skip this iteration if allocation fails
            }

            if (commandBufferCreateInfo.CommandBuffer != VK_NULL_HANDLE)
            {
                mCommandBuffers.emplace_back(std::make_unique<CommandBuffer>(commandBufferCreateInfo));
            }
            else
            {
                CGS_LOG_ERROR("Failed to allocate command buffer for command pool.");
            }
        }
    }

    void CommandPool::Reset() noexcept
    {
        if (mCommandPool != VK_NULL_HANDLE)
        {
            VkResult vr = VK_SUCCESS;
            vr = vkResetCommandPool(mDevice.GetVkDevice(), mCommandPool, 0);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to reset command pool: %s", VkResultToString(vr));
            }
        }
    }

    void CommandPool::Trim() const noexcept
    {
        if (mCommandPool != VK_NULL_HANDLE)
        {
            vkTrimCommandPool(mDevice.GetVkDevice(), mCommandPool, 0);
        }
    }
} // namespace cgs::graphics::rhi
