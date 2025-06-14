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
            uint32_t Index; // Index of the physical device group
            VkPhysicalDeviceGroupProperties PhysicalDeviceGroupProperties; // Properties of the physical device group
        };

    public:
        static constexpr const uint32_t DEFAULT_INDEX = 0; // Default index for the physical device group

    public:
        PhysicalDeviceGroup() = delete;
        explicit PhysicalDeviceGroup(CreateInfo& createInfo) noexcept;
        ~PhysicalDeviceGroup() noexcept;

        void PrintProperties() const noexcept;

        CGS_INLINE constexpr uint32_t GetIndex() const noexcept { return mIndex; }

    private:
        void createPhysicalDevices() noexcept;

    private:
        Instance& mInstance; // Reference to the RHI instance
        uint32_t mIndex;
        VkPhysicalDeviceGroupProperties mPhysicalDeviceGroupProperties; // Properties of the physical device group
        std::vector<std::unique_ptr<PhysicalDevice>> mPhysicalDevices; // List of physical devices in the group
    };
} // namespace cgs::graphics::rhi
