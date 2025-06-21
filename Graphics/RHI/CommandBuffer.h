#pragma once

namespace cgs::graphics::rhi
{
    class Fence;
    class Semaphore;

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

        void Begin() noexcept;
        void End() const noexcept;
        void Reset() const noexcept;
		void Wait() const noexcept; // Wait for the command buffer to complete execution

        CGS_INLINE constexpr uint32_t GetIndex() const noexcept { return mIndex; }
        CGS_INLINE constexpr uint32_t GetFrameBufferIndex() const noexcept { return mFrameBufferIndex; }
		CGS_INLINE constexpr uint32_t GetBackBufferIndex() const noexcept { return mBackBufferIndex; }
		CGS_INLINE constexpr VkCommandBuffer GetVkCommandBuffer() const noexcept { return mCommandBuffer; }
        CGS_INLINE Fence& GetFence() const noexcept { return *mFence; }
		CGS_INLINE const Semaphore& GetPresentCompletionSemaphore() const noexcept { return *mPresentCompletionSemaphore; }
		CGS_INLINE const Semaphore& GetRenderCompletionSemaphore() const noexcept { return *mRenderCompletionSemaphore; }

    private:
        CommandPool& mCommandPool;
        uint32_t mIndex;
        uint32_t mFrameBufferIndex; // Index of the frame buffer this command buffer is associated with
		uint32_t mBackBufferIndex; // Index of the back buffer this command buffer is associated with
        VkCommandBuffer mCommandBuffer;
        std::unique_ptr<Fence> mFence; // Optional fence for synchronization
        std::unique_ptr<Semaphore> mPresentCompletionSemaphore;
        std::unique_ptr<Semaphore> mRenderCompletionSemaphore;
    };

    struct CommandBufferScope final
    {
    public:
        CGS_INLINE explicit CommandBufferScope(CommandBuffer& commandBuffer) noexcept
			: mCommandBuffer(commandBuffer)
		{
			mCommandBuffer.Begin(); // Begin the command buffer when the scope is created
		}
        CGS_INLINE ~CommandBufferScope() noexcept
		{
			mCommandBuffer.End(); // End the command buffer when the scope is destroyed
		}
		CommandBufferScope(const CommandBufferScope&) = delete; // Copy constructor is deleted
		CommandBufferScope(CommandBufferScope&&) noexcept = default; // Move constructor
		CommandBufferScope& operator=(const CommandBufferScope&) = delete; // Copy assignment operator is deleted
		CommandBufferScope& operator=(CommandBufferScope&&) noexcept = delete; // Move assignment operator is deleted

    private:
        CommandBuffer& mCommandBuffer; // Reference to the command buffer being scoped
    };
} // namespace cgs::graphics::rhi
