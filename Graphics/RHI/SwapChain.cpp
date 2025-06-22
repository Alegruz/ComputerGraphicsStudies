#include "Graphics/pch.h"

#include "Graphics/RHI/SwapChain.h"

#include "Graphics/RHI/Attachment.h"
#include "Graphics/RHI/CommandPool.h"
#include "Graphics/RHI/Device.h"
#include "Graphics/RHI/Image.h"
#include "Graphics/RHI/Instance.h"
#include "Graphics/RHI/PhysicalDevice.h"
#include "Graphics/RHI/Semaphore.h"

namespace cgs::graphics::rhi
{
    SwapChain::SwapChain(const CreateInfo& createInfo) noexcept
        : mDevice(createInfo.RhiDevice)
        , mSwapChain(createInfo.SwapChain)
        , mSurface(createInfo.Surface)
        , mWidth(createInfo.Extent.width)
        , mHeight(createInfo.Extent.height)
        , mBackBufferAttachment()
    {
        assert(mSwapChain != VK_NULL_HANDLE);
    }

    SwapChain::~SwapChain() noexcept
    {
        if (mSwapChain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(mDevice.GetVkDevice(), mSwapChain, nullptr);
            mSwapChain = VK_NULL_HANDLE;
        }
        
        const Instance& instance = mDevice.GetPhysicalDevice().GetInstance();
		if(mSurface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(instance.GetVkInstance(), mSurface, nullptr);
			mSurface = VK_NULL_HANDLE;
		}
		else
		{
			CGS_LOG_WARNING("Attempted to destroy a null surface.");
		}
    }

    uint32_t SwapChain::AcquireNextImage() const noexcept
    {
        assert(mSwapChain != VK_NULL_HANDLE);

        uint32_t imageIndex = 0;
        const VkAcquireNextImageInfoKHR acquireInfo =
        {
            .sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
            .pNext = nullptr,
            .swapchain = mSwapChain,
            .timeout = UINT64_MAX, // Wait indefinitely for the next image
            .semaphore = VK_NULL_HANDLE, // No semaphore is used here
            .fence = VK_NULL_HANDLE // No fence is used here
        };
        VkResult vr = vkAcquireNextImage2KHR(mDevice.GetVkDevice(), &acquireInfo, &imageIndex);
        if (vr != VK_SUCCESS && vr != VK_SUBOPTIMAL_KHR)
        {
            CGS_LOG_ERROR("Failed to acquire next image from swap chain: %s", VkResultToString(vr));
            return UINT32_MAX; // Return an invalid index on failure
        }

        return imageIndex; // Return the acquired image index
    }

	void SwapChain::SetBackBufferAttachment(std::shared_ptr<Attachment>& backBufferAttachment) noexcept
	{
		assert(backBufferAttachment != nullptr);
		mBackBufferAttachment = backBufferAttachment;

        std::vector<std::unique_ptr<CommandPool>>& commandPools = mDevice.GetCommandPools();
		for (auto& commandPool : commandPools)
		{
			if (commandPool)
			{
                commandPool->AllocateCommandBuffer();
			}
		}
	}

    uint32_t SwapChain::GetBackBufferCount() const noexcept
    {
		return mBackBufferAttachment ? mBackBufferAttachment->GetColorAttachmentCount() : 0;
    }
} // namespace cgs::graphics::rhi