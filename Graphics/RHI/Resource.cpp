#include "Graphics/pch.h"

#include "Graphics/RHI/Resource.h"

namespace cgs::graphics::rhi
{
    Resource::Resource(const CreateInfo& createInfo) noexcept
        : mDevice(createInfo.RhiDevice)
        , mDeviceMemory(createInfo.DeviceMemory)
    {
    }
} // namespace cgs::graphics::rhi