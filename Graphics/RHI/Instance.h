#pragma once

namespace cgs::graphics::rhi
{
    class PhysicalDeviceGroup;

	class Instance final
	{
	public:
		struct CreateInfo final
		{
			friend class Instance;

			cgs::core::Config& Config; // Reference to the configuration object
			cgs::core::ProjectInfo ApplicationInfo; // Information about the application
			cgs::core::ProjectInfo EngineInfo;
			uint32_t ApiVersion = VK_API_VERSION_1_3; // Vulkan API version to use, default is 1.3
			VkDebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfo = 
			{ 
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT, 
				.pNext = nullptr,
				.flags = 0,
				.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
								   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
								   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
				.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
							   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
				.pfnUserCallback = DebugUtilsMessengerCallback,
				.pUserData = nullptr
			};
		};

	public:
		static VkBool32 DebugReportCallback(VkDebugReportFlagsEXT flags, [[maybe_unused]] VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, [[maybe_unused]] void* pUserData) noexcept;
		static VkBool32 DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, [[maybe_unused]] void* pUserData) noexcept;
        static constexpr void FilterMessages(bool& bInoutPrintMessage, const char* message) noexcept;
	
	public:
		Instance() = delete;
		explicit Instance(CreateInfo& createInfo) noexcept;
		~Instance() noexcept;

		CGS_INLINE constexpr const cgs::core::Config& GetConfig() const noexcept { return mConfig; } // Accessor for the configuration object
		CGS_INLINE constexpr const std::vector<std::unique_ptr<PhysicalDeviceGroup>>& GetPhysicalDeviceGroups() const noexcept { return mPhysicalDeviceGroups; } // Accessor for the physical device groups

	private:
		void createInstance(CreateInfo& createInfo) noexcept;
		void createDebugUtilsMessenger(CreateInfo& createInfo) noexcept;
		void createPhysicalDeviceGroups() noexcept;

	private:
		[[maybe_unused]] cgs::core::Config& mConfig;
		VkInstance mInstance;
		std::vector<std::unique_ptr<PhysicalDeviceGroup>> mPhysicalDeviceGroups;
		VkDebugUtilsMessengerEXT mDebugUtilsMessenger; // Debug messenger for Vulkan validation layers
	};
}
