#include "Graphics/pch.h"

#include "Graphics/RHI/CommandBuffer.h"

#include "Graphics/RHI/CommandPool.h"
#include "Graphics/RHI/Device.h"
#include "Graphics/RHI/Fence.h"

namespace cgs::graphics::rhi
{
    CommandBuffer::CommandBuffer(const CreateInfo& createInfo) noexcept
        : mCommandPool(createInfo.RhiCommandPool)
        , mIndex(createInfo.Index)
        , mFrameBufferIndex(createInfo.FrameBufferIndex) // Initialize the frame buffer index
        , mCommandBuffer(createInfo.CommandBuffer)
        , mFence()
    {
        VkFenceCreateInfo fenceCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        const Device& device = mCommandPool.GetDevice();
        VkFence fence = VK_NULL_HANDLE;
        VkResult vr = vkCreateFence(device.GetVkDevice(), &fenceCreateInfo, nullptr, &fence);
        if (vr != VK_SUCCESS)
        {
            CGS_LOG_ERROR("Failed to create fence: %s", VkResultToString(vr));
            return;
        }

        Fence::CreateInfo fenceCreateInfoStruct =
        {
            .RhiDevice = device,
            .Fence = fence // Pass the created
        };
        mFence = std::make_unique<Fence>(fenceCreateInfoStruct);
    }

    CommandBuffer::~CommandBuffer() noexcept
    {
        const Device& device = mCommandPool.GetDevice();
        vkFreeCommandBuffers(device.GetVkDevice(), mCommandPool.GetVkCommandPool(), 1, &mCommandBuffer);
        mCommandBuffer = VK_NULL_HANDLE; // Reset the command buffer handle
    }

    void CommandBuffer::Begin() const noexcept
    {
        assert(mCommandBuffer != VK_NULL_HANDLE);
        
        VkCommandBufferBeginInfo beginInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // Use one-time submit for this command buffer
            .pInheritanceInfo = nullptr // No inheritance info for primary command buffers
        };

        VkResult vr = vkBeginCommandBuffer(mCommandBuffer, &beginInfo);
        if( vr != VK_SUCCESS )
        {
            CGS_LOG_ERROR("Failed to begin command buffer: {}", vr);
        }
    }

    void CommandBuffer::End() const noexcept
    {
        assert(mCommandBuffer != VK_NULL_HANDLE);
        VkResult vr = vkEndCommandBuffer(mCommandBuffer);
        if( vr != VK_SUCCESS )
        {
            CGS_LOG_ERROR("Failed to end command buffer: {}", vr);
        }
    }

    void CommandBuffer::Reset() const noexcept
    {
        assert(mCommandBuffer != VK_NULL_HANDLE);
        VkResult vr = vkResetCommandBuffer(mCommandBuffer, 0);
        if( vr != VK_SUCCESS )
        {
            CGS_LOG_ERROR("Failed to reset command buffer: {}", vr);
        }
    }
} // namespace cgs::graphics::rhi
