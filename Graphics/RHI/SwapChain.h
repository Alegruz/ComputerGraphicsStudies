#pragma once

namespace cgs::graphics::rhi
{
    class BackBuffer;

    class SwapChain final
    {
    public:
        friend class Device; // Allow Device to create SwapChain instances
        friend class Instance; // Allow Instance to create SwapChain instances

    public:
        struct CreateInfo final
        {
            const Device& RhiDevice; // Reference to the device this swap chain is created from
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

        CGS_INLINE constexpr const Device& GetDevice() const noexcept { return mDevice; } // Accessor for the device this swap chain is created from
        CGS_INLINE constexpr uint32_t GetBackBufferCount() const noexcept { return static_cast<uint32_t>(mBackBuffers.size()); } // Get the number of back buffers in the swap chain
    
    private:
        const Device& mDevice; // Reference to the device this swap chain is created from
        VkSwapchainKHR mSwapChain; // Handle to the swap chain
        VkSurfaceKHR mSurface; // Handle to the surface associated with the swap chain, if applicable
        std::vector<std::unique_ptr<BackBuffer>> mBackBuffers; // Images in the swap chain
    };
} // namespace cgs::graphics::rhi
