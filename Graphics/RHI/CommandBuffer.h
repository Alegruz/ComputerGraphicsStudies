#pragma once

namespace cgs::graphics::rhi
{
    class Fence;

    class CommandBuffer final
    {
    public:
        friend class Device; // Allow Device to create CommandPool instances

    public:
        struct CreateInfo final
        {
            CommandPool& RhiCommandPool;
            uint32_t Index = 0; // Index of the command buffer in the command pool
            uint32_t FrameBufferIndex = 0; // Index of the frame buffer this command buffer is associated with
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

        CGS_INLINE constexpr uint32_t GetIndex() const noexcept { return mIndex; }
        CGS_INLINE constexpr uint32_t GetFrameBufferIndex() const noexcept { return mFrameBufferIndex; }
        CGS_INLINE Fence& GetFence() const noexcept { return *mFence; }

    private:
        CommandPool& mCommandPool;
        uint32_t mIndex;
        uint32_t mFrameBufferIndex; // Index of the frame buffer this command buffer is associated with
        VkCommandBuffer mCommandBuffer;
        std::unique_ptr<Fence> mFence; // Optional fence for synchronization
    };
} // namespace cgs::graphics::rhi
