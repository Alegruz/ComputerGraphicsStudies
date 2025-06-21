#pragma once

namespace cgs::graphics::rhi
{
	class Device;
	class Instance;
	class PhysicalDeviceGroup;
	class QueueFamily;

	class PhysicalDevice final
	{
	public:
		friend class PhysicalDeviceGroup;

	public:
		struct CreateInfo final
		{
			Instance&				RhiInstance;
			PhysicalDeviceGroup&	RhiPhysicalDeviceGroup;
			VkPhysicalDevice		PhysicalDevice;
			bool					bCreateLogicalDevice = true; // Whether to create a logical device for this physical device
		};

		struct Properties final
		{
			VkPhysicalDeviceMemoryProperties	MemoryProperties;
			VkPhysicalDeviceVulkan14Properties 	Vulkan14Properties;
			VkPhysicalDeviceVulkan13Properties 	Vulkan13Properties;
			VkPhysicalDeviceVulkan12Properties 	Vulkan12Properties;
			VkPhysicalDeviceVulkan11Properties 	Vulkan11Properties;
			VkPhysicalDeviceProperties2 		PhysicalDeviceProperties;

			CGS_INLINE constexpr Properties() noexcept
				: Vulkan14Properties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES, .pNext = nullptr }
				, Vulkan13Properties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES, .pNext = &Vulkan14Properties }
				, Vulkan12Properties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES, .pNext = &Vulkan13Properties }
				, Vulkan11Properties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES, .pNext = &Vulkan12Properties }
				, PhysicalDeviceProperties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &Vulkan11Properties }
			{
			}
		};

	public:
		PhysicalDevice() = delete;
		explicit PhysicalDevice(const CreateInfo& createInfo) noexcept;
		PhysicalDevice(const PhysicalDevice&) = delete;
		PhysicalDevice(PhysicalDevice&&) noexcept = default;
		~PhysicalDevice() noexcept;

		PhysicalDevice& operator=(const PhysicalDevice&) = delete;
		PhysicalDevice& operator=(PhysicalDevice&&) noexcept = delete;

		void DestroyLogicalDevice(VkDevice &inoutDevice) const noexcept;
		float EvaluateScore() const noexcept;
		uint32_t GetMemoryTypeIndex(const uint32_t typeBits, const VkMemoryPropertyFlags memoryPropertyFlags) const noexcept;
		void PrintProperties() const noexcept;

		constexpr const char* GetName() const noexcept;
		CGS_INLINE constexpr const Properties& GetProperties() const noexcept { return mProperties; }
		constexpr const std::vector<std::unique_ptr<QueueFamily>>& GetQueueFamilies() const noexcept;
		CGS_INLINE const QueueFamily& GetMainQueueFamily() const noexcept { return *mQueueFamilies.front().get(); }
		CGS_INLINE const Device& GetLogicalDevice() const noexcept { return *mLogicalDevice; }
		CGS_INLINE Device& GetLogicalDevice() noexcept { return *mLogicalDevice; }
		CGS_INLINE const Instance& GetInstance() const noexcept { return mInstance; }
		bool IsPresentSupported(const uint32_t queueFamilyIndex) const noexcept;

	private:
		static void printDeviceProperties(const Properties &properties) noexcept;
		static constexpr const char* getVendorIdName(const VkVendorId vendorId) noexcept;
		static constexpr const char* getTypeName(const VkPhysicalDeviceType deviceType) noexcept;
		static constexpr const char* getDriverIdName(const VkDriverId driverId) noexcept;

	private:
		void createQueueFamilies() noexcept;
		void createLogicalDevice() noexcept;

	private:
		Instance&				mInstance;
		PhysicalDeviceGroup&	mPhysicalDeviceGroup; // Reference to the physical device group this physical device belongs to
		VkPhysicalDevice		mPhysicalDevice;
		Properties				mProperties; // Properties of the physical device

		std::vector<std::unique_ptr<QueueFamily>> mQueueFamilies; // Queue families supported by this physical device
		std::unique_ptr<Device>	mLogicalDevice; // Logical device created from this physical device, if any
	};

	CGS_INLINE constexpr const char* PhysicalDevice::GetName() const noexcept
	{
		return mProperties.PhysicalDeviceProperties.properties.deviceName;
	}

	CGS_INLINE constexpr const std::vector<std::unique_ptr<QueueFamily>>& PhysicalDevice::GetQueueFamilies() const noexcept
	{
		return mQueueFamilies;
	}

	CGS_INLINE constexpr const char* PhysicalDevice::getVendorIdName(const VkVendorId vendorId) noexcept
	{
		switch (vendorId)
		{
		case VK_VENDOR_ID_KHRONOS:
			return "Khronos";
		case VK_VENDOR_ID_VIV:
			return "Vivante";
		case VK_VENDOR_ID_VSI:
			return "VeriSilicon";
		case VK_VENDOR_ID_KAZAN:
			return "Kazan";
		case VK_VENDOR_ID_CODEPLAY:
			return "Codeplay";
		case VK_VENDOR_ID_MESA:
			return "Mesa";
		case VK_VENDOR_ID_POCL:
			return "POCL";
		case VK_VENDOR_ID_MOBILEYE:
			return "Mobileye";
		default:
			return "Unknown Vendor ID";
		}
	}

	CGS_INLINE constexpr const char* PhysicalDevice::getTypeName(const VkPhysicalDeviceType deviceType) noexcept
	{
		switch (deviceType)
		{
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			return "Other";
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			return "Integrated GPU";
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			return "Discrete GPU";
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			return "Virtual GPU";
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			return "CPU";
		default:
			CGS_LOG_ERROR("Unknown physical device type: %d", static_cast<int>(deviceType));
			return "Unknown Device Type";
		}
	}

	CGS_INLINE constexpr const char* PhysicalDevice::getDriverIdName(const VkDriverId driverId) noexcept
	{
		switch (driverId)
		{
		case VK_DRIVER_ID_AMD_PROPRIETARY:
			return "AMD Proprietary";
		case VK_DRIVER_ID_AMD_OPEN_SOURCE:
			return "AMD Open Source";
		case VK_DRIVER_ID_MESA_RADV:
			return "Mesa RADV";
		case VK_DRIVER_ID_NVIDIA_PROPRIETARY:
			return "NVIDIA Proprietary";
		case VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS:
			return "Intel Proprietary Windows";
		case VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA:
			return "Intel Open Source Mesa";
		case VK_DRIVER_ID_IMAGINATION_PROPRIETARY:
			return "Imagination Proprietary";
		case VK_DRIVER_ID_QUALCOMM_PROPRIETARY:
			return "Qualcomm Proprietary";
		case VK_DRIVER_ID_ARM_PROPRIETARY:
			return "ARM Proprietary";
		case VK_DRIVER_ID_GOOGLE_SWIFTSHADER:
			return "Google SwiftShader";
		case VK_DRIVER_ID_GGP_PROPRIETARY:
			return "GGP Proprietary";
		case VK_DRIVER_ID_BROADCOM_PROPRIETARY:
			return "Broadcom Proprietary";
		case VK_DRIVER_ID_MESA_LLVMPIPE:
			return "Mesa LLVMpipe";
		case VK_DRIVER_ID_MOLTENVK:
			return "MoltenVK";
		case VK_DRIVER_ID_COREAVI_PROPRIETARY:
			return "CoreAVI Proprietary";
		case VK_DRIVER_ID_JUICE_PROPRIETARY:
			return "Juice Proprietary";
		case VK_DRIVER_ID_VERISILICON_PROPRIETARY:
			return "VeriSilicon Proprietary";
		case VK_DRIVER_ID_MESA_TURNIP:
			return "Mesa Turnip";
		case VK_DRIVER_ID_MESA_V3DV:
			return "Mesa V3DV";
		case VK_DRIVER_ID_MESA_PANVK:
			return "Mesa PanVK";
		case VK_DRIVER_ID_SAMSUNG_PROPRIETARY:
			return "Samsung Proprietary";
		case VK_DRIVER_ID_MESA_VENUS:
			return "Mesa Venus";
		case VK_DRIVER_ID_MESA_DOZEN:
			return "Mesa Dozen";
		case VK_DRIVER_ID_MESA_NVK:
			return "Mesa NVK";
		case VK_DRIVER_ID_IMAGINATION_OPEN_SOURCE_MESA:
			return "Imagination Open Source Mesa";
		case VK_DRIVER_ID_MESA_HONEYKRISP:
			return "Mesa Honeykrisp";
		case VK_DRIVER_ID_VULKAN_SC_EMULATION_ON_VULKAN:
			return "Vulkan SC Emulation on Vulkan";
		default:
		{
			// Handle unknown driver IDs
			CGS_LOG_ERROR("Unknown driver ID: %d", static_cast<int>(driverId));
		}
			return "Unknown Driver ID";
		}
	}
}
