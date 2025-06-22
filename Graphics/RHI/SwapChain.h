#pragma once

namespace cgs::graphics::rhi
{
    class Attachment;

    class SwapChain final
    {
    public:
        friend class Device; // Allow Device to create SwapChain instances
        friend class Instance; // Allow Instance to create SwapChain instances

    public:
        struct CreateInfo final
        {
            Device& RhiDevice; // Reference to the device this swap chain is created from
            VkSwapchainKHR SwapChain = VK_NULL_HANDLE; // Handle to the swap chain
            VkSurfaceKHR Surface = VK_NULL_HANDLE; // Handle to the surface associated with the swap chain, if applicable
            VkExtent2D Extent = { .width = 1920, .height = 1080, }; // Default extent for the swap chain, can be adjusted later
        };

    public:
        SwapChain() = delete; // Default constructor is deleted
        explicit SwapChain(const CreateInfo& createInfo) noexcept;
        SwapChain(const SwapChain&) = delete; // Copy constructor is deleted
        SwapChain(SwapChain&&) noexcept = default; // Move constructor
        ~SwapChain() noexcept;

        SwapChain& operator=(const SwapChain&) = delete; // Copy assignment operator is deleted
        SwapChain& operator=(SwapChain&&) noexcept = default; // Move assignment operator

        uint32_t AcquireNextImage() const noexcept; // Acquire the next image from the swap chain
        void SetBackBufferAttachment(std::shared_ptr<Attachment>& backBufferAttachment) noexcept;

        CGS_INLINE constexpr const Device& GetDevice() const noexcept { return mDevice; } // Accessor for the device this swap chain is created from
		CGS_INLINE constexpr VkSwapchainKHR GetVkSwapChain() const noexcept { return mSwapChain; } // Accessor for the Vulkan swap chain handle
        uint32_t GetBackBufferCount() const noexcept;
        CGS_INLINE constexpr VkSurfaceKHR GetVkSurface() const noexcept { return mSurface; } // Accessor for the Vulkan surface handle, if applicable
        CGS_INLINE constexpr uint32_t GetWidth() const noexcept { return mWidth; }
        CGS_INLINE constexpr uint32_t GetHeight() const noexcept { return mHeight; } // Accessors for the width and height of the swap chain
    
    private:
        Device& mDevice; // Reference to the device this swap chain is created from
        VkSwapchainKHR mSwapChain; // Handle to the swap chain
        VkSurfaceKHR mSurface; // Handle to the surface associated with the swap chain, if applicable
        uint32_t mWidth;
        uint32_t mHeight; // Width and height of the swap chain
        std::shared_ptr<Attachment> mBackBufferAttachment; // Attachment for the back buffer, if applicable
    };
} // namespace cgs::graphics::rhi
