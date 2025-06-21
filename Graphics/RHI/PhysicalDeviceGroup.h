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
            bool bCreateLogicalDevice = true; // Whether to create logical devices for the physical devices in this group
        };

    public:
        static constexpr const uint32_t DEFAULT_INDEX = 0; // Default index for the physical device group

    public:
        PhysicalDeviceGroup() = delete;
        explicit PhysicalDeviceGroup(CreateInfo& createInfo) noexcept;
        ~PhysicalDeviceGroup() noexcept;

        void PrintProperties() const noexcept;

        CGS_INLINE constexpr uint32_t GetIndex() const noexcept { return mIndex; }
        CGS_INLINE constexpr const std::vector<std::unique_ptr<PhysicalDevice>>& GetPhysicalDevices() const noexcept { return mPhysicalDevices; }
		CGS_INLINE const PhysicalDevice& GetMainPhysicalDevice() const noexcept { return *mPhysicalDevices[mMainPhysicalDeviceIndex]; } // Accessor for the main physical device in the group
        CGS_INLINE PhysicalDevice& GetMainPhysicalDevice() noexcept { return *mPhysicalDevices[mMainPhysicalDeviceIndex]; } // Accessor for the main physical device in the group

    private:
        void createPhysicalDevices(const bool bCreateLogicalDevice) noexcept;

    private:
        Instance& mInstance; // Reference to the RHI instance
        uint32_t mIndex;
        VkPhysicalDeviceGroupProperties mPhysicalDeviceGroupProperties; // Properties of the physical device group
        std::vector<std::unique_ptr<PhysicalDevice>> mPhysicalDevices; // List of physical devices in the group
		uint32_t mMainPhysicalDeviceIndex; // Index of the main physical device in the group, if applicable
    };
} // namespace cgs::graphics::rhi
