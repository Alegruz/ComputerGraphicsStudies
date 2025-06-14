#include "Graphics/pch.h" 

#include "Graphics/RHI/PhysicalDevice.h"
#include "Graphics/RHI/QueueFamily.h"

namespace cgs::graphics::rhi
{
    QueueFamily::QueueFamily(const CreateInfo& createInfo) noexcept
        : mQueueFamilyProperties(createInfo.QueueFamilyProperties)
        , mIndex(createInfo.Index)
    {
        assert(mQueueFamilyProperties.sType == VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2);

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
} // namespace cgs::graphics::rhi
