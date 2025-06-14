#include "Graphics/pch.h"

#include "Graphics/RHI/Device.h"

namespace cgs::graphics::rhi
{
    Device::Device(const CreateInfo& createInfo) noexcept
        : mPhysicalDevice(createInfo.RhiPhysicalDevice)
        , mDevice(createInfo.Device)
    {
        assert(mDevice != VK_NULL_HANDLE);
    }

    Device::~Device() noexcept
    {
    }
}