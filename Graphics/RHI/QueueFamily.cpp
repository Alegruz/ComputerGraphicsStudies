#include "Graphics/pch.h" 

#include "Graphics/RHI/QueueFamily.h"

#include "Graphics/RHI/PhysicalDevice.h"
#include "Graphics/RHI/Queue.h"

namespace cgs::graphics::rhi
{
    QueueFamily::QueueFamily(const CreateInfo& createInfo) noexcept
        : mPhysicalDevice(createInfo.RhiPhysicalDevice)
        , mQueueFamilyProperties(createInfo.QueueFamilyProperties)
        , mIndex(createInfo.Index)
    {
        assert(mQueueFamilyProperties.sType == VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2);

        PrintProperties();
    }

    QueueFamily::~QueueFamily() noexcept
    {
        // No specific cleanup needed for the queue family itself.
        // The device and queues will handle their own cleanup.
    }

    float QueueFamily::EvaluateScore() const noexcept
    {
        float score = 0.0f;

        if (mQueueFamilyProperties.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            score += 1.0f; // Graphics queue is highly desirable.
        }
        if (mQueueFamilyProperties.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            score += 0.5f; // Compute queue is also desirable.
        }
        if (mQueueFamilyProperties.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            score += 0.2f; // Transfer queue is useful but less critical.
        }

        return score;
    }

    void QueueFamily::PrintProperties() const noexcept
    {
        CGS_LOG_INFO(
            "Queue Family %u Properties:"
            "\n\tQueue Flags:"
            "\n\t\tVK_QUEUE_GRAPHICS_BIT: %s"
            "\n\t\tVK_QUEUE_COMPUTE_BIT: %s"
            "\n\t\tVK_QUEUE_TRANSFER_BIT: %s"
            "\n\t\tVK_QUEUE_SPARSE_BINDING_BIT: %s"
            "\n\t\tVK_PROTECTED_BIT: %s"
            "\n\t\tVK_QUEUE_VIDEO_DECODE_BIT_KHR: %s"
            "\n\t\tVK_QUEUE_VIDEO_ENCODE_BIT_KHR: %s"
            "\n\tQueue Count: %u"
            , mIndex
            , (mQueueFamilyProperties.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) ? "true" : "false"
            , (mQueueFamilyProperties.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) ? "true" : "false"
            , (mQueueFamilyProperties.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) ? "true" : "false"
            , (mQueueFamilyProperties.queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? "true" : "false"
            , (mQueueFamilyProperties.queueFamilyProperties.queueFlags & VK_QUEUE_PROTECTED_BIT) ? "true" : "false"
            , (mQueueFamilyProperties.queueFamilyProperties.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) ? "true" : "false"
            , (mQueueFamilyProperties.queueFamilyProperties.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) ? "true" : "false"
            , mQueueFamilyProperties.queueFamilyProperties.queueCount
        );
    }

    bool QueueFamily::IsPresentSupported() const noexcept
    {
        return mPhysicalDevice.IsPresentSupported(mIndex);
    }
} // namespace cgs::graphics::rhi
