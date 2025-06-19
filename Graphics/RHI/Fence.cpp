#include "Graphics/pch.h"

#include "Graphics/RHI/Fence.h"

#include "Graphics/RHI/Device.h"

namespace cgs::graphics::rhi
{
    Fence::~Fence() noexcept
    {
        if (mFence != VK_NULL_HANDLE)
        {
            vkDestroyFence(mDevice.GetVkDevice(), mFence, nullptr);
            mFence = VK_NULL_HANDLE;
        }
    }   
} // namespace cgs::graphics::rhi
