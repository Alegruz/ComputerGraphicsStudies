#include "Graphics/pch.h"

#include "Graphics/RHI/Queue.h"

#include "Graphics/RHI/Device.h"
#include "Graphics/RHI/Fence.h"

namespace cgs::graphics::rhi
{
    Queue::Queue(const CreateInfo& createInfo) noexcept
        : mDevice(createInfo.RhiDevice)
        , mQueueFamily(createInfo.RhiQueueFamily)
        , mQueue(createInfo.Queue)
        , mSubmissionFence(mDevice.CreateFence())
    {
        assert(mQueue != VK_NULL_HANDLE);
    }
    
    void Queue::Submit() const noexcept
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
        VkResult vr = vkQueueSubmit2(mQueue, 1, &submitInfo, mSubmissionFence->mFence);
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
        if (mSubmissionFence)
        {
            mSubmissionFence.reset();
        }
        mQueue = VK_NULL_HANDLE;
    }
} // namespace cgs::graphics::rhi
