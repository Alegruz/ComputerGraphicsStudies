#include "Graphics/pch.h"

#include "Graphics/RHI/CommandBuffer.h"

#include "Graphics/RHI/CommandPool.h"

namespace cgs::graphics::rhi
{
    CommandBuffer::CommandBuffer(const CreateInfo& createInfo) noexcept
        : mCommandPool(createInfo.RhiCommandPool)
        , mIndex(createInfo.Index)
        , mCommandBuffer(createInfo.CommandBuffer)
    {
    }

    CommandBuffer::~CommandBuffer() noexcept
    {
        mCommandPool.Destroy(*this);
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
