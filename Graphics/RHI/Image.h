#pragma once

#include "Graphics/RHI/Resource.h"

namespace cgs::graphics::rhi
{
    class Image final : public Resource
    {
    public:
        friend class Device; // Allow Device to create Image instances

    public:
        struct CreateInfo final
        {
            Resource::CreateInfo ResourceCreateInfo; // Base resource creation info
            VkImage Image = VK_NULL_HANDLE; // Handle to the Vulkan image
            VkImageView ImageView = VK_NULL_HANDLE; // Optional image view for the image, if applicable
			uint32_t Width = 0; // Width of the image
			uint32_t Height = 0; // Height of the image
            Format Format; // Format of the image
        };

    public:
        Image() = delete; // Default constructor is deleted
        explicit Image(const CreateInfo& createInfo) noexcept;

        Image(const Image&) = delete; // Copy constructor is deleted
        Image(Image&&) noexcept = default; // Move constructor
        ~Image() noexcept;

        Image& operator=(const Image&) = delete; // Copy assignment operator is deleted
        Image& operator=(Image&&) noexcept = delete; // Move assignment operator

        CGS_INLINE constexpr VkImage GetVkImage() const noexcept { return mImage; } // Accessor for the Vulkan image handle
		CGS_INLINE constexpr VkImageView GetVkImageView() const noexcept { return mImageView; } // Accessor for the Vulkan image view handle, if applicable
		CGS_INLINE constexpr uint32_t GetWidth() const noexcept { return mWidth; } // Accessor for the image width
		CGS_INLINE constexpr uint32_t GetHeight() const noexcept { return mHeight; } // Accessor for the image height
        CGS_INLINE constexpr bool IsBackBuffer() const noexcept { return mDeviceMemory == VK_NULL_HANDLE; }

    private:
        VkImage mImage; // Handle to the Vulkan image
        VkImageView mImageView; // Optional image view for the image, if applicable
		uint32_t mWidth; // Width of the image
		uint32_t mHeight; // Height of the image
        Format mFormat; // Format of the image
    };
} // namespace cgs::graphics::rhi
