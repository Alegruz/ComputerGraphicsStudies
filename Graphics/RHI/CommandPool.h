#pragma once

namespace cgs::graphics::rhi
{
    class Device; // Forward declaration of Device class

    class CommandPool final
    {
    public:
        friend class Device; // Allow Device to create CommandPool instances

    public:
        struct CreateInfo final
        {
            Device& RhiDevice; // Reference to the device this command pool belongs to
            VkCommandPool CommandPool = VK_NULL_HANDLE; // The Vulkan command pool handle
        };

    public:
        CommandPool() = delete; // Default constructor is deleted
        explicit CommandPool(const CreateInfo &createInfo) noexcept;

        CommandPool(const CommandPool&) = delete; // Copy constructor is deleted
        CommandPool(CommandPool&&) noexcept = default; // Move constructor
        ~CommandPool() noexcept; // Destructor

        CommandPool& operator=(const CommandPool&) = delete; // Copy assignment operator is deleted
        CommandPool& operator=(CommandPool&&) noexcept = delete; // Move assignment operator is deleted
    
    private:
        Device& mDevice; // Reference to the device this command pool belongs to
        VkCommandPool mCommandPool; // The Vulkan command pool handle
    };
} // namespace cgs::graphics::rhi
