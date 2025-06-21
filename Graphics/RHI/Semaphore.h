#pragma once

namespace cgs::graphics::rhi
{
    class Semaphore final
    {
    public:
        friend class Device; // Allow Device to create Semaphore instances

    public:
        struct CreateInfo final
        {
            const Device& RhiDevice; // Reference to the device this semaphore belongs to
            VkSemaphore Semaphore = VK_NULL_HANDLE; // The Vulkan semaphore handle
        };

    public:
        Semaphore() = delete; // Default constructor is deleted
        explicit Semaphore(const CreateInfo& createInfo) noexcept;
        Semaphore(const Semaphore&) = delete; // Copy constructor is deleted
        Semaphore(Semaphore&&) noexcept = default; // Move constructor
        ~Semaphore() noexcept;
        Semaphore& operator=(const Semaphore&) = delete; // Copy assignment operator is deleted
        Semaphore& operator=(Semaphore&&) noexcept = default; // Move assignment operator

        CGS_INLINE constexpr const Device& GetDevice() const noexcept { return mDevice; } // Accessor for the device this semaphore belongs to
		CGS_INLINE constexpr VkSemaphore GetVkSemaphore() const noexcept { return mSemaphore; } // Accessor for the Vulkan semaphore handle
        
    private:
        const Device& mDevice; // Reference to the device this semaphore belongs to
        VkSemaphore mSemaphore; // The Vulkan semaphore handle
    };
} // namespace cgs::graphics::rhi
