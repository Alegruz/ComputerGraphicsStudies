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
        CGS_INLINE SwapChain& GetSwapChain() noexcept { return *mSwapChain; } // Accessor for the swap chain created by this device
        CGS_INLINE constexpr VkDevice GetVkDevice() const noexcept { return mDevice; } // Accessor for the Vulkan logical device handle
		CGS_INLINE constexpr const std::vector<std::unique_ptr<CommandPool>>& GetCommandPools() const noexcept { return mCommandPools; } // Accessor for the command pools created by this device
        CGS_INLINE constexpr std::vector<std::unique_ptr<CommandPool>>& GetCommandPools() noexcept { return mCommandPools; } // Accessor for the command pools created by this device
		CGS_INLINE const CommandPool& GetMainCommandPool() const noexcept { return *mCommandPools[0]; }
		CGS_INLINE CommandPool& GetMainCommandPool() noexcept { return *mCommandPools[0]; } // Accessor for the main command pool created by this device

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