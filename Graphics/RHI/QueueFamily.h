#pragma once

namespace cgs::graphics::rhi
{
    class PhysicalDevice;

    class QueueFamily final
    {
    public:
        struct CreateInfo final
        {
            PhysicalDevice& RhiPhysicalDevice; // Reference to the physical device this queue family belongs to
            VkQueueFamilyProperties2 QueueFamilyProperties; // Properties of the queue family
            uint32_t Index = 0; // Index of the queue family in the physical device
        };

    public:
        QueueFamily() = delete;
        explicit QueueFamily(const CreateInfo &createInfo) noexcept;

        QueueFamily(const QueueFamily&) = delete;
        QueueFamily(QueueFamily&&) noexcept = default;
        ~QueueFamily() noexcept = default;

        QueueFamily& operator=(const QueueFamily&) = delete;
        QueueFamily& operator=(QueueFamily&&) noexcept = delete;

    private:
        VkQueueFamilyProperties2 mQueueFamilyProperties; // Properties of the queue family
        uint32_t mIndex; // Index of the queue family in the physical device
    };
} // namespace cgs::graphics::rhi
