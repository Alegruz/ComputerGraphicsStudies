#pragma once

namespace cgs::graphics::rhi
{
    class CommandPool;
    class PhysicalDevice;

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

        void Destroy(CommandPool& inoutCommandPool) const noexcept;
        void Reset(CommandPool& inoutCommandPool) const noexcept;
        void Trim(CommandPool& inoutCommandPool) const noexcept;

    private:
        void createCommandPools() noexcept;

    private:
        PhysicalDevice& mPhysicalDevice; // Reference to the physical device this logical device is created from
        VkDevice mDevice;

        std::vector<std::unique_ptr<CommandPool>> mCommandPools; // Command pools created by this device
    };
}