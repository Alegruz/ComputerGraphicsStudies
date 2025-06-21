#pragma once

namespace cgs::graphics::rhi
{
    class Fence final
    {
    public:
        friend class Device; // Allow Device to create Fence instances
        friend class Queue; // Allow Device to create Fence instances

    public:
        struct CreateInfo final
        {
            const Device& RhiDevice; // Reference to the device this fence belongs to
            VkFence Fence = VK_NULL_HANDLE; // The Vulkan fence handle
        };

    public:
        Fence() = delete; // Default constructor is deleted
        explicit constexpr Fence(const CreateInfo& createInfo) noexcept;

        Fence(const Fence&) = delete; // Copy constructor is deleted
        Fence(Fence&&) noexcept = default; // Move constructor
        ~Fence() noexcept;

        Fence& operator=(const Fence&) = delete; // Copy assignment operator is deleted
        Fence& operator=(Fence&&) noexcept = delete; // Move assignment operator is deleted

        CGS_INLINE constexpr const VkFence& GetVkFence() const noexcept { return mFence; } // Accessor for the Vulkan fence handle
    
    private:
        const Device& mDevice; // Reference to the device this fence belongs to
        VkFence mFence; // The Vulkan fence handle
    };

    CGS_INLINE constexpr Fence::Fence(const CreateInfo& createInfo) noexcept
        : mDevice(createInfo.RhiDevice)
        , mFence(createInfo.Fence)
    {
        assert(mFence != VK_NULL_HANDLE); // Ensure the fence handle is valid
    }
} // namespace cgs::graphics::rhi
