#pragma once

namespace cgs::graphics::rhi
{
    class CommandBuffer final
    {
    public:
        friend class Device; // Allow Device to create CommandPool instances

    public:
        struct CreateInfo final
        {
            CommandPool& RhiCommandPool;
            uint32_t Index = 0; // Index of the command buffer in the command pool
            VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
        };

    public:
        CommandBuffer(const CreateInfo& createInfo) noexcept;
        ~CommandBuffer() noexcept;

        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer(CommandBuffer&&) = delete;
        CommandBuffer& operator=(const CommandBuffer&) = delete;
        CommandBuffer& operator=(CommandBuffer&&) = delete;

        void Begin() const noexcept;
        void End() const noexcept;
        void Reset() const noexcept;

    private:
        CommandPool& mCommandPool;
        uint32_t mIndex;
        VkCommandBuffer mCommandBuffer;
    };
} // namespace cgs::graphics::rhi
