#pragma once

namespace cgs::graphics::rhi
{
    class CommandBuffer; // Forward declaration of CommandBuffer class
    class Device;
    class QueueFamily;

    class Queue final
    {
    public:
        struct CreateInfo final
        {
            Device& RhiDevice; // Reference to the physical device this queue belongs to
            QueueFamily& RhiQueueFamily; // Reference to the queue family this queue belongs to
            VkQueue Queue; // The Vulkan queue handle
        };

    public:
        Queue() = delete;
        explicit Queue(const CreateInfo &createInfo) noexcept;

        Queue(const Queue&) = delete;
        Queue(Queue&&) noexcept = default;
        ~Queue() noexcept;

        Queue& operator=(const Queue&) = delete;
        Queue& operator=(Queue&&) noexcept = delete;

        void Present() const noexcept; // Present the queue, typically used for swap chain images
        void Submit(CommandBuffer& commandBuffer) const noexcept;

    private:
        [[maybe_unused]] Device& mDevice; // Reference to the device this queue belongs to
        [[maybe_unused]] QueueFamily& mQueueFamily; // Reference to the queue family this queue belongs to
        VkQueue mQueue; // The Vulkan queue handle
    };
} // namespace cgs::graphics::rhi
