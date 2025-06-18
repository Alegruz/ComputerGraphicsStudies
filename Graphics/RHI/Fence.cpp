#include "Graphics/pch.h"

#include "Graphics/RHI/Fence.h"

#include "Graphics/RHI/Device.h"

namespace cgs::graphics::rhi
{
    Fence::~Fence() noexcept
    {
        if (mFence != VK_NULL_HANDLE)
        {
            mDevice.Destroy(*this); // Destroy the fence using the device
        }
    }   
} // namespace cgs::graphics::rhi
