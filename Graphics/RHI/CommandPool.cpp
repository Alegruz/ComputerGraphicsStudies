#include "Graphics/pch.h"

#include "Graphics/RHI/CommandPool.h"

#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/Device.h"

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
        mDevice.Destroy(*this);
    }

    void CommandPool::AllocateCommandBuffer() noexcept
    {
        assert(mCommandPool != VK_NULL_HANDLE);

        CommandBuffer::CreateInfo commandBufferCreateInfo =
        {
            .RhiCommandPool = *this,
            .Index = static_cast<uint32_t>(mCommandBuffers.size()), // Use the current size as the index
            .CommandBuffer = mDevice.Allocate(*this) // Allocate a new command buffer from the device
        };

        if (commandBufferCreateInfo.CommandBuffer != VK_NULL_HANDLE)
        {
            mCommandBuffers.emplace_back(std::make_unique<CommandBuffer>(commandBufferCreateInfo));
        }
        else
        {
            CGS_LOG_ERROR("Failed to allocate command buffer for command pool.");
        }
    }

    void CommandPool::FreeCommandBuffer(const uint32_t commandBufferIndex) noexcept
    {
        mDevice.FreeCommandBuffer(*this, *mCommandBuffers[commandBufferIndex]);
    }

    void CommandPool::FreeCommandBuffers() noexcept
    {
        for(auto& commandBuffer : mCommandBuffers)
        {
            mDevice.FreeCommandBuffer(*this, *commandBuffer);
        }
    }
    
    void CommandPool::Destroy(CommandBuffer& inoutCommandBuffer) noexcept
    {
        mDevice.Destroy(*this, inoutCommandBuffer);
    }
} // namespace cgs::graphics::rhi
