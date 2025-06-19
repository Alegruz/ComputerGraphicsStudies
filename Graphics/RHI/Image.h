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
        };

    public:
        Image() = delete; // Default constructor is deleted
        explicit Image(const CreateInfo& createInfo) noexcept;

        Image(const Image&) = delete; // Copy constructor is deleted
        Image(Image&&) noexcept = default; // Move constructor
        ~Image() noexcept;

        Image& operator=(const Image&) = delete; // Copy assignment operator is deleted
        Image& operator=(Image&&) noexcept = default; // Move assignment operator

        CGS_INLINE constexpr VkImage GetVkImage() const noexcept { return mImage; } // Accessor for the Vulkan image handle
        CGS_INLINE constexpr bool IsBackBuffer() const noexcept { return mDeviceMemory == VK_NULL_HANDLE; }

    private:
        VkImage mImage; // Handle to the Vulkan image
        VkImageView mImageView; // Optional image view for the image, if applicable
    };
} // namespace cgs::graphics::rhi
