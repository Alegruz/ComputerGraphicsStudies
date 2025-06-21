#include "Graphics/pch.h"

#include "Graphics/RHI/CommandBuffer.h"

#include "Graphics/RHI/BackBuffer.h"
#include "Graphics/RHI/CommandPool.h"
#include "Graphics/RHI/Device.h"
#include "Graphics/RHI/Fence.h"
#include "Graphics/RHI/Image.h"
#include "Graphics/RHI/Semaphore.h"
#include "Graphics/RHI/SwapChain.h"

namespace cgs::graphics::rhi
{
    CommandBuffer::CommandBuffer(const CreateInfo& createInfo) noexcept
        : mCommandPool(createInfo.RhiCommandPool)
        , mIndex(createInfo.Index)
        , mFrameBufferIndex(createInfo.FrameBufferIndex) // Initialize the frame buffer index
		, mBackBufferIndex(createInfo.FrameBufferIndex) // Initialize the back buffer index
        , mCommandBuffer(createInfo.CommandBuffer)
        , mFence()
		, mPresentCompletionSemaphore()
		, mRenderCompletionSemaphore()
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

        Semaphore::CreateInfo presentSemaphoreCreateInfo =
        {
            .RhiDevice = device,
        };

        Semaphore::CreateInfo renderSemaphoreCreateInfo =
        {
            .RhiDevice = device,
        };

        VkSemaphoreCreateInfo semaphoreCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0 // No special flags
        };

        vr = vkCreateSemaphore(device.GetVkDevice(), &semaphoreCreateInfo, nullptr, &presentSemaphoreCreateInfo.Semaphore);
        if (vr != VK_SUCCESS)
        {
            CGS_LOG_ERROR("Failed to create semaphore: %s", VkResultToString(vr));
            return;
        }

        vr = vkCreateSemaphore(device.GetVkDevice(), &semaphoreCreateInfo, nullptr, &renderSemaphoreCreateInfo.Semaphore);
        if (vr != VK_SUCCESS)
        {
            CGS_LOG_ERROR("Failed to create semaphore: %s", VkResultToString(vr));
            return;
        }

		mPresentCompletionSemaphore = std::make_unique<Semaphore>(presentSemaphoreCreateInfo);
		mRenderCompletionSemaphore = std::make_unique<Semaphore>(renderSemaphoreCreateInfo);
    }

    CommandBuffer::~CommandBuffer() noexcept
    {
        mPresentCompletionSemaphore.reset();
        mRenderCompletionSemaphore.reset();

        const Device& device = mCommandPool.GetDevice();
        vkFreeCommandBuffers(device.GetVkDevice(), mCommandPool.GetVkCommandPool(), 1, &mCommandBuffer);
        mCommandBuffer = VK_NULL_HANDLE; // Reset the command buffer handle
    }

    void CommandBuffer::Begin() noexcept
    {
        assert(mCommandBuffer != VK_NULL_HANDLE);

        Wait();

        const SwapChain& swapChain = mCommandPool.GetDevice().GetSwapChain();

        VkResult vr = VK_SUCCESS;
        vr = vkAcquireNextImageKHR(
            mCommandPool.GetDevice().GetVkDevice(),
            swapChain.GetVkSwapChain(),
            UINT64_MAX, // Wait indefinitely
            mPresentCompletionSemaphore->GetVkSemaphore(),
            VK_NULL_HANDLE, // No fence
            &mBackBufferIndex // Store the acquired back buffer index
        );
		if (vr != VK_SUCCESS && vr != VK_SUBOPTIMAL_KHR)
		{
			CGS_LOG_ERROR("Failed to acquire next image: %s", VkResultToString(vr));
			return;
		}

		vkResetCommandBuffer(mCommandBuffer, 0); // Reset the command buffer before beginning
        
        VkCommandBufferBeginInfo beginInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // Use one-time submit for this command buffer
            .pInheritanceInfo = nullptr // No inheritance info for primary command buffers
        };

        vr = vkBeginCommandBuffer(mCommandBuffer, &beginInfo);
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

    void CommandBuffer::Wait() const noexcept
    {
		assert(mFence != nullptr);
		VkResult vr = vkWaitForFences(mCommandPool.GetDevice().GetVkDevice(), 1, &mFence->GetVkFence(), VK_TRUE, UINT64_MAX);
		if (vr != VK_SUCCESS)
		{
			CGS_LOG_ERROR("Failed to wait for fence: %s", VkResultToString(vr));
		}

		vr = vkResetFences(mCommandPool.GetDevice().GetVkDevice(), 1, &mFence->GetVkFence());
        if (vr != VK_SUCCESS)
        {
            CGS_LOG_ERROR("Failed to reset fence: %s", VkResultToString(vr));
        }
    }
} // namespace cgs::graphics::rhi
