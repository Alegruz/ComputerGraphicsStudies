#pragma once

namespace cgs::graphics::rhi
{
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
        ~Queue() noexcept = default;

        Queue& operator=(const Queue&) = delete;
        Queue& operator=(Queue&&) noexcept = delete;

        void Reset() noexcept;
        void Trim() noexcept;

    private:
        [[maybe_unused]] Device& mDevice; // Reference to the device this queue belongs to
        [[maybe_unused]] QueueFamily& mQueueFamily; // Reference to the queue family this queue belongs to
        VkQueue mQueue; // The Vulkan queue handle
    };
} // namespace cgs::graphics::rhi
