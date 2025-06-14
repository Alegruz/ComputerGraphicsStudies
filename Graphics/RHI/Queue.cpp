#include "Graphics/pch.h"
#include "Graphics/RHI/Queue.h"

namespace cgs::graphics::rhi
{
    Queue::Queue(const CreateInfo& createInfo) noexcept
        : mDevice(createInfo.RhiDevice)
        , mQueueFamily(createInfo.RhiQueueFamily)
        , mQueue(createInfo.Queue)
    {
        assert(mQueue != VK_NULL_HANDLE);
    }

    void Queue::Reset() noexcept
    {
        // Resetting a queue is not a common operation in Vulkan.
        // This function can be used to reset any internal state if needed.
        // Currently, it does nothing as Vulkan queues do not have a reset operation.
    }
    
    void Queue::Trim() noexcept
    {
        // Trimming a queue is not a common operation in Vulkan.
        // This function can be used to release any internal resources if needed.
        // Currently, it does nothing as Vulkan queues do not have a trim operation.
    }
} // namespace cgs::graphics::rhi
