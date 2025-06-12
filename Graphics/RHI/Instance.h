#pragma once

#include "volk/volk.h"

#include "Core/pch.h"

namespace cgs::graphics::rhi
{
    struct InstanceCreateInfo;

	class Instance final
	{
	public:
		static VkBool32 DebugReportCallback(VkDebugReportFlagsEXT flags, [[maybe_unused]] VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, [[maybe_unused]] void* pUserData) noexcept;
		static VkBool32 DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, [[maybe_unused]] void* pUserData) noexcept;

	public:
		Instance() = delete;
		explicit Instance(const InstanceCreateInfo& createInfo) noexcept;
		constexpr ~Instance() noexcept;

	private:
		VkInstance	mInstance;
	};

	CGS_INLINE constexpr Instance::~Instance() noexcept
	{
		// Destroy the debug utils messenger if it was created.
		// vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugUtilsMessenger, nullptr);
		// mDebugUtilsMessenger = VK_NULL_HANDLE;

		// Destroy the instance.
		if(mInstance != VK_NULL_HANDLE)
		{
			vkDestroyInstance(mInstance, nullptr);
			mInstance = VK_NULL_HANDLE;
		}
	}
}