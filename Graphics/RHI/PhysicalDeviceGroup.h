#pragma once

#include "volk/volk.h"

namespace cgs::graphics::rhi
{
    class Instance;
    class PhysicalDevice;

    class PhysicalDeviceGroup final
    {
    public:
        struct CreateInfo final
        {
            Instance& RhiInstance; // Reference to the RHI instance
            VkPhysicalDeviceGroupProperties PhysicalDeviceGroupProperties; // Properties of the physical device group
        };

    public:
        PhysicalDeviceGroup() = delete;
        explicit PhysicalDeviceGroup(CreateInfo& createInfo) noexcept;
        ~PhysicalDeviceGroup() noexcept;

        void PrintProperties() const noexcept;

    private:
        void createPhysicalDevices() noexcept;

    private:
        Instance& mInstance; // Reference to the RHI instance
        VkPhysicalDeviceGroupProperties mPhysicalDeviceGroupProperties; // Properties of the physical device group
        std::vector<std::unique_ptr<PhysicalDevice>> mPhysicalDevices; // List of physical devices in the group
    };
} // namespace cgs::graphics::rhi
