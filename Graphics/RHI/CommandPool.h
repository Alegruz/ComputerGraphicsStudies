#pragma once

namespace cgs::graphics::rhi
{
    class CommandBuffer; // Forward declaration of CommandBuffer class
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

        constexpr const std::vector<std::unique_ptr<CommandBuffer>>& GetCommandBuffers() const noexcept;
        
        void AllocateCommandBuffer() noexcept; // Allocate command buffers from this command pool
        void FreeCommandBuffer(const uint32_t commandBufferIndex) noexcept; // Free a specific command buffer by index
        void FreeCommandBuffers() noexcept; // Free all command buffers in this command pool
        void Destroy(CommandBuffer& inoutCommandBuffer) noexcept;
    
    private:
        Device& mDevice; // Reference to the device this command pool belongs to
        VkCommandPool mCommandPool; // The Vulkan command pool handle
        std::vector<std::unique_ptr<CommandBuffer>> mCommandBuffers; // Command buffers created by this command pool
    };

    CGS_INLINE constexpr const std::vector<std::unique_ptr<CommandBuffer>>& CommandPool::GetCommandBuffers() const noexcept
    {
        return mCommandBuffers; // Return the command buffers associated with this command pool
    }
} // namespace cgs::graphics::rhi
