#pragma once

namespace cgs::graphics::rhi
{
    class PhysicalDevice;
    class Queue;

    class QueueFamily final
    {
    public:
        friend class PhysicalDevice; // Allow PhysicalDevice to create QueueFamily instances

    public:
        struct CreateInfo final
        {
            PhysicalDevice& RhiPhysicalDevice; // Reference to the physical device this queue family belongs to
            VkQueueFamilyProperties2 QueueFamilyProperties; // Properties of the queue family
            uint32_t Index = 0; // Index of the queue family in the physical device
        };

    public:
        QueueFamily() = delete;
        explicit QueueFamily(const CreateInfo& createInfo) noexcept;
        QueueFamily(const QueueFamily&) = delete;
        QueueFamily(QueueFamily&&) noexcept = default;
        ~QueueFamily() noexcept;

        QueueFamily& operator=(const QueueFamily&) = delete;
        QueueFamily& operator=(QueueFamily&&) noexcept = delete;
        
        float EvaluateScore() const noexcept;
        void PrintProperties() const noexcept;

        CGS_INLINE constexpr const VkQueueFamilyProperties2& GetQueueFamilyProperties() const noexcept { return mQueueFamilyProperties; }
        CGS_INLINE constexpr uint32_t GetQueueCount() const noexcept { return mQueueFamilyProperties.queueFamilyProperties.queueCount; }
        CGS_INLINE constexpr VkQueueFlags GetQueueFlags() const noexcept { return mQueueFamilyProperties.queueFamilyProperties.queueFlags; }
        CGS_INLINE constexpr uint32_t GetIndex() const noexcept { return mIndex; }
		CGS_INLINE constexpr const std::vector<std::unique_ptr<Queue>>& GetQueues() const noexcept { return mQueues; }
		CGS_INLINE const Queue& GetMainQueue() const noexcept { return *mQueues[mMainQueueIndex]; }

        bool IsPresentSupported() const noexcept;
		bool IsDirectQueue() const noexcept;
		bool IsComputeSupported() const noexcept;
		bool IsTransferSupported() const noexcept;

    private:
        const PhysicalDevice& mPhysicalDevice; // Reference to the physical device this queue family belongs to
        VkQueueFamilyProperties2 mQueueFamilyProperties; // Properties of the queue family
        uint32_t mIndex; // Index of the queue family in the physical device
        std::vector<std::unique_ptr<Queue>> mQueues; // Queues in this queue family, if any
		uint32_t mMainQueueIndex; // Index of the main queue in this queue family, if applicable
    };
} // namespace cgs::graphics::rhi
