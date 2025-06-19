#pragma once

namespace cgs::graphics::rhi
{
    class CommandBuffer;
    class CommandPool;
    class Fence;
    class Image;
    class PhysicalDevice;
    class Semaphore;
    class SwapChain;

    class Device final
    {
    public:
        friend class PhysicalDevice;

    public:
        struct CreateInfo final
        {
            PhysicalDevice& RhiPhysicalDevice; // Reference to the physical device this logical device is created from
            VkDevice        Device; // The Vulkan logical device handle
        };

    public:
        Device() = delete;
        explicit Device(const CreateInfo& createInfo) noexcept;
        Device(const Device&) = delete;
        Device(Device&&) noexcept = default;
        ~Device() noexcept;

        Device& operator=(const Device&) = delete;
        Device& operator=(Device&&) noexcept = delete;

        CGS_INLINE constexpr const PhysicalDevice& GetPhysicalDevice() const noexcept { return mPhysicalDevice; }
        CGS_INLINE const SwapChain& GetSwapChain() const noexcept { return *mSwapChain; }
        CGS_INLINE constexpr VkDevice GetVkDevice() const noexcept { return mDevice; } // Accessor for the Vulkan logical device handle

    private:
        void createCommandPools() noexcept;
        void createSwapChain() noexcept;

    private:
        PhysicalDevice& mPhysicalDevice; // Reference to the physical device this logical device is created from
        VkDevice mDevice;

        std::vector<std::unique_ptr<CommandPool>> mCommandPools; // Command pools created by this device
        std::unique_ptr<SwapChain> mSwapChain; // Swap chain created by this device, if any
    };
}