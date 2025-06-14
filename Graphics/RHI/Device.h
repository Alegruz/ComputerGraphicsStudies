#pragma once

#include "volk/volk.h"

namespace cgs::graphics::rhi
{
    class PhysicalDevice;

    class Device final
    {
    public:
        friend class PhysicalDevice;

    public:
        struct CreateInfo final
        {
            PhysicalDevice& RhiPhysicalDevice; // Reference to the physical device this logical device is created from
            VkDevice        Device; // The Vulkan logical device handle
        };

    public:
        Device() = delete;
        explicit Device(const CreateInfo& createInfo) noexcept;
        Device(const Device&) = delete;
        Device(Device&&) noexcept = default;
        ~Device() noexcept;

        Device& operator=(const Device&) = delete;
        Device& operator=(Device&&) noexcept = delete;

    private:
        PhysicalDevice& mPhysicalDevice; // Reference to the physical device this logical device is created from
        VkDevice mDevice;
    };
}