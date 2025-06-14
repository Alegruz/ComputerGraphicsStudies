#include "Graphics/pch.h"

#include "Graphics/RHI/CommandPool.h"

#include "Graphics/RHI/Device.h"

namespace cgs::graphics::rhi
{
    CommandPool::CommandPool(const CreateInfo& createInfo) noexcept
        : mDevice(createInfo.RhiDevice)
        , mCommandPool(createInfo.CommandPool)
    {
        assert(mCommandPool != VK_NULL_HANDLE);
    }

    CommandPool::~CommandPool() noexcept
    {
        mDevice.Destroy(*this);
    }
} // namespace cgs::graphics::rhi
