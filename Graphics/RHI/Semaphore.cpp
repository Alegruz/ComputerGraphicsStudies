#include "Graphics/pch.h"

#include "Graphics/RHI/Semaphore.h"

#include "Graphics/RHI/Device.h"

namespace cgs::graphics::rhi
{
    Semaphore::Semaphore(const CreateInfo& createInfo) noexcept
        : mDevice(createInfo.RhiDevice)
        , mSemaphore(createInfo.Semaphore)
    {
        assert(mSemaphore != VK_NULL_HANDLE);
    }

    Semaphore::~Semaphore() noexcept
    {
        if (mSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(mDevice.GetVkDevice(), mSemaphore, nullptr);
            mSemaphore = VK_NULL_HANDLE; // Reset the semaphore handle
        }
    }
}   // namespace cgs::graphics::rhi