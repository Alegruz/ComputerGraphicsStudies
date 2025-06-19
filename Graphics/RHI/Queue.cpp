#include "Graphics/pch.h"

#include "Graphics/RHI/Queue.h"

#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/Fence.h"
#include "Graphics/RHI/Device.h"

namespace cgs::graphics::rhi
{
    Queue::Queue(const CreateInfo& createInfo) noexcept
        : mDevice(createInfo.RhiDevice)
        , mQueueFamily(createInfo.RhiQueueFamily)
        , mQueue(createInfo.Queue)
    {
        assert(mQueue != VK_NULL_HANDLE);

        VkFenceCreateInfo fenceCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
    }
    
    void Queue::Submit(CommandBuffer& commandBuffer) const noexcept
    {
        const VkSubmitInfo2 submitInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext = nullptr,
            .flags = 0, // No special flags
            .waitSemaphoreInfoCount = 0, // No wait semaphores
            .pWaitSemaphoreInfos = nullptr, // No wait semaphores
            .commandBufferInfoCount = 0, // No command buffers
            .pCommandBufferInfos = nullptr, // No command buffers
            .signalSemaphoreInfoCount = 0, // No signal semaphores
            .pSignalSemaphoreInfos = nullptr // No signal semaphores
        };
        Fence& fence = commandBuffer.GetFence();
        VkResult vr = vkQueueSubmit2(mQueue, 1, &submitInfo, fence.GetVkFence());
        if (vr != VK_SUCCESS)
        {
            CGS_LOG_ERROR("Failed to submit queue: %s", VkResultToString(vr));
        }
        else
        {
            CGS_LOG_INFO("Queue submitted successfully.");
        }
    }

    Queue::~Queue() noexcept
    {
        mQueue = VK_NULL_HANDLE;
    }

    void Queue::Present() const noexcept
    {
        const VkPresentInfoKHR presentInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = 0, // No wait semaphores
            .pWaitSemaphores = nullptr, // No wait semaphores
            .swapchainCount = 0, // No swap chains
            .pSwapchains = nullptr, // No swap chains
            .pImageIndices = nullptr, // No image indices
            .pResults = nullptr // No results
        };
        VkResult vr = vkQueuePresentKHR(mQueue, &presentInfo);
        if (vr != VK_SUCCESS)
        {
            CGS_LOG_ERROR("Failed to present queue: %s", VkResultToString(vr));
        }
    }
} // namespace cgs::graphics::rhi
