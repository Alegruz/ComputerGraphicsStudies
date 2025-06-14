#pragma once

#include "volk/volk.h"

#include "Core/pch.h"

namespace cgs::graphics::rhi
{
    class PhysicalDevice;

	struct InstanceCreateInfo
	{
		cgs::core::ProjectInfo ApplicationInfo; // Information about the application
		cgs::core::ProjectInfo EngineInfo;
	};

	class Instance final
	{
	public:
		static VkBool32 DebugReportCallback(VkDebugReportFlagsEXT flags, [[maybe_unused]] VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, [[maybe_unused]] void* pUserData) noexcept;
		static VkBool32 DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, [[maybe_unused]] void* pUserData) noexcept;
        static constexpr void FilterMessages(bool& bInoutPrintMessage, const char* message) noexcept;
	
	public:
		Instance() = delete;
		explicit Instance(const InstanceCreateInfo& createInfo) noexcept;
		~Instance() noexcept;

	private:
		VkInstance	mInstance;
        std::vector<std::unique_ptr<PhysicalDevice>> mPhysicalDevices;
	};
}