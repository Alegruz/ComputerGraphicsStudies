#include "Graphics/pch.h"

#include "Graphics/RHI/Image.h"

#include "Graphics/RHI/Device.h"

namespace cgs::graphics::rhi
{
    Image::Image(const CreateInfo& createInfo) noexcept
        : Resource(createInfo.ResourceCreateInfo)
        , mImage(createInfo.Image)
        , mImageView(createInfo.ImageView) // Initialize image view to null
		, mWidth(createInfo.Width)
		, mHeight(createInfo.Height)
    {
        assert(mImage != VK_NULL_HANDLE);
    }

    Image::~Image() noexcept
    {
        if (mImage != VK_NULL_HANDLE && IsBackBuffer() == false)
        {
            // If the image is not a back buffer, destroy it
            vkDestroyImage(mDevice.GetVkDevice(), mImage, nullptr);
            mImage = VK_NULL_HANDLE;
        }
    }
} // namespace cgs::graphics::rhi