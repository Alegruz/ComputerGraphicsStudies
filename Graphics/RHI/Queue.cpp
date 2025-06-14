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
} // namespace cgs::graphics::rhi
