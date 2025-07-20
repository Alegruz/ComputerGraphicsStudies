#pragma once

namespace cgs::graphics::rhi
{
    class Resource
    {
    public:
        struct CreateInfo final
        {
            const Device& RhiDevice; // Reference to the device this resource is created from
            VkDeviceMemory DeviceMemory = VK_NULL_HANDLE; // Device memory associated with this resource, if applicable
        };
        
    public:
        Resource() = delete; // Default constructor is deleted
        explicit Resource(const CreateInfo& createInfo) noexcept;

        Resource(const Resource&) = delete; // Copy constructor is deleted
        Resource(Resource&&) noexcept = default; // Move constructor
        virtual ~Resource() noexcept = default;

        Resource& operator=(const Resource&) = delete; // Copy assignment operator is deleted
        Resource& operator=(Resource&&) noexcept = delete; // Move assignment operator

        CGS_INLINE constexpr const Device& GetDevice() const noexcept { return mDevice; } // Accessor for the device this resource is created from
        CGS_INLINE constexpr VkDeviceMemory GetVkDeviceMemory() const noexcept { return mDeviceMemory; } // Accessor for the device memory associated with this resource, if applicable

    protected:
        const Device& mDevice; // Reference to the device this resource is created from
        VkDeviceMemory mDeviceMemory; // Device memory associated with this resource, if applicable
    };
} // namespace cgs::graphics::rhi
