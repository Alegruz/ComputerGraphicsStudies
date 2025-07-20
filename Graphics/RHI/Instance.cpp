#include "Graphics/pch.h"

#include "Graphics/RHI/Instance.h"

#include <iostream>
#include <string>
#include <vector>

#include "Graphics/RHI/PhysicalDeviceGroup.h"
#include "Graphics/RHI/SwapChain.h"

namespace cgs::graphics::rhi
{
	VkBool32 Instance::DebugReportCallback(VkDebugReportFlagsEXT flags, [[maybe_unused]] VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, [[maybe_unused]] void* pUserData) noexcept
	{
		std::string logMsg;
		logMsg.reserve(256);
		logMsg += "object: 0x";
		char buf[17];
		snprintf(buf, sizeof(buf), "%016llX", static_cast<unsigned long long>(object));
		logMsg += buf;
		logMsg += " location: ";
		logMsg += std::to_string(location);
		logMsg += " messageCode: ";
		logMsg += std::to_string(messageCode);
		logMsg += " layerPrefix: ";
		if (pLayerPrefix)
			logMsg += pLayerPrefix;
		logMsg += " message: ";
		if (pMessage)
			logMsg += pMessage;
		logMsg += '\n';

		switch (flags)
		{
		case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
			break;
		case VK_DEBUG_REPORT_WARNING_BIT_EXT:
			CGS_LOG_WARNING(logMsg.c_str());
			break;
		case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
			CGS_LOG_WARNING(logMsg.c_str());
			break;
		case VK_DEBUG_REPORT_ERROR_BIT_EXT:
			CGS_LOG_ERROR(logMsg.c_str());
			break;
		case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
			CGS_LOG_DEBUG(logMsg.c_str());
			break;
		case VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT:
			[[fallthrough]];
		default:
			assert(false);
			break;
		}

		return VK_TRUE;
	}

	VkBool32 Instance::DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, [[maybe_unused]] void* pUserData) noexcept
	{
		bool bPrintMessage = false;
		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			bPrintMessage = true;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			bPrintMessage = true;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
			[[fallthrough]];
		default:
			assert(false);
			break;
		}

        FilterMessages(bPrintMessage, pCallbackData->pMessage);

		switch (messageTypes)
		{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			[[fallthrough]];
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			[[fallthrough]];
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
			[[fallthrough]];
		case VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT:
			[[fallthrough]];
		default:
			assert(false);
			break;
		}

		if (bPrintMessage == true)
		{
			std::string logMsg;
			logMsg.reserve(512);

			// Severity
			switch (messageSeverity)
			{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				logMsg += "[VERBOSE]";
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				logMsg += "[INFO]";
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				logMsg += "[WARNING]";
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				logMsg += "[SEVERITY]";
				break;
			default:
				assert(false);
				break;
			}

			// MessageId and Message
			logMsg += " MessageId: [";
			if (pCallbackData->pMessageIdName)
				logMsg.append(pCallbackData->pMessageIdName);
			logMsg += ", #: ";
			logMsg += std::to_string(pCallbackData->messageIdNumber);
			logMsg += "] Message: ";
			if (pCallbackData->pMessage)
				logMsg.append(pCallbackData->pMessage);
			logMsg += '\n';

			// Queue labels
			for (uint32_t i = 0; i < pCallbackData->queueLabelCount; ++i)
			{
				logMsg += "\tQueueLabel: ";
				if (pCallbackData->pQueueLabels[i].pLabelName)
					logMsg.append(pCallbackData->pQueueLabels[i].pLabelName);
				logMsg += '\n';
			}
			// Command buffer labels
			for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; ++i)
			{
				logMsg += "\tCommandBufferLabel: ";
				if (pCallbackData->pCmdBufLabels[i].pLabelName)
					logMsg.append(pCallbackData->pCmdBufLabels[i].pLabelName);
				logMsg += '\n';
			}
			// Objects
			for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
			{
				const auto& object = pCallbackData->pObjects[i];
				logMsg += "\tObjectType: ";
				logMsg += std::to_string(object.objectType);
				logMsg += " Handle: 0x";
				char buf[17];
				snprintf(buf, sizeof(buf), "%016llX", static_cast<unsigned long long>(object.objectHandle));
				logMsg += buf;
				logMsg += " Name: ";
				if (object.pObjectName)
					logMsg.append(object.pObjectName);
				logMsg += '\n';
			}
			
			switch (messageSeverity)
			{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				CGS_LOG_INFO(logMsg.c_str());
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				CGS_LOG_INFO(logMsg.c_str());
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				CGS_LOG_WARNING(logMsg.c_str());
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				CGS_LOG_ERROR(logMsg.c_str());
				break;
			default:
				assert(false);
				break;
			}
		}

		return VK_TRUE;
	}

	constexpr void Instance::FilterMessages(bool& bInoutPrintMessage, const char* message) noexcept
	{
		if (!message)
		{
			return;
		}

		if (bInoutPrintMessage == true)
		{
			if (strstr(message, "#LLP_LAYER_3") != nullptr)
			{
				if (strstr(message, "GalaxyOverlayVkLayer") != nullptr)
				{
					bInoutPrintMessage = false;
				}
			}

			if (strstr(message, "uses API version") != nullptr 
			&& strstr(message, "which is older than the application specified API version of ") != nullptr 
			&& strstr(message, "May cause issues.") != nullptr)
			{
				bInoutPrintMessage = false;
			}
		}
	}

	Instance::Instance(CreateInfo& createInfo) noexcept
		: mConfig(createInfo.Config)
		, mInstance(VK_NULL_HANDLE)
		, mPhysicalDeviceGroups()
		, mMainPhysicalDeviceGroupIndex(0)
		, mDebugUtilsMessenger(VK_NULL_HANDLE)
		, mProcessHandle(createInfo.ProcessHandle) // Store the process handle for the renderer
		, mWindowHandle(createInfo.WindowHandle)
	{
		[[maybe_unused]] VkResult vr = volkInitialize();
		assert(vr == VK_SUCCESS);
		
		uint32_t apiVersion = 0;
		vr = vkEnumerateInstanceVersion(&apiVersion);
		assert(vr == VK_SUCCESS);
		CGS_LOG_INFO("Enumerated Vulkan Instance Version: %u.%u.%u.%u", VK_API_VERSION_VARIANT(apiVersion), VK_API_VERSION_MAJOR(apiVersion), VK_API_VERSION_MINOR(apiVersion), VK_API_VERSION_PATCH(apiVersion));
		assert(apiVersion >= createInfo.ApiVersion);
		createInfo.ApiVersion = apiVersion;

		createInstance(createInfo);
		createDebugUtilsMessenger(createInfo);
		createPhysicalDeviceGroups(createInfo.bCreateLogicalDevice);
	}

	Instance::~Instance() noexcept
	{
		mPhysicalDeviceGroups.clear();

		// Destroy the debug utils messenger if it was created.
		// vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugUtilsMessenger, nullptr);
		// mDebugUtilsMessenger = VK_NULL_HANDLE;

		if (mDebugUtilsMessenger != VK_NULL_HANDLE)
		{
			vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugUtilsMessenger, nullptr);
			mDebugUtilsMessenger = VK_NULL_HANDLE;
		}

		// Destroy the instance.
		if(mInstance != VK_NULL_HANDLE)
		{
			vkDestroyInstance(mInstance, nullptr);
			mInstance = VK_NULL_HANDLE;
		}
	}

	void Instance::createInstance(CreateInfo& createInfo) noexcept
	{
		[[maybe_unused]] VkResult vr = VK_SUCCESS;

		VkApplicationInfo applicationInfo =
		{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = createInfo.ApplicationInfo.Name.c_str(),
			.applicationVersion = createInfo.ApplicationInfo.Version,
			.pEngineName = createInfo.EngineInfo.Name.c_str(),
			.engineVersion = createInfo.EngineInfo.Version,
			.apiVersion = createInfo.ApiVersion,
		};

		std::vector<VkValidationFeatureEnableEXT> validationFeaturesToEnable =
		{
			VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
			VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
			VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
			VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
			VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
		};

		void *pNext = nullptr;
		// VkValidationFeaturesEXT validationFeatures =
		//{
		//	.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
		//	.pNext = pNext,
		//	.enabledValidationFeatureCount = static_cast<uint32_t>(validationFeaturesToEnable.size()),
		//	.pEnabledValidationFeatures = validationFeaturesToEnable.data(),
		//	.disabledValidationFeatureCount = 0,
		//	.pDisabledValidationFeatures = nullptr,
		// };
		// pNext = &validationFeatures;

		createInfo.DebugUtilsMessengerCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.pNext = pNext,
			.flags = 0,
			.messageSeverity = (VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT),
			.messageType = (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT),
			.pfnUserCallback = DebugUtilsMessengerCallback,
			.pUserData = nullptr,
		};
		pNext = &createInfo.DebugUtilsMessengerCreateInfo;

		// Not using VK_EXT_debug_report because it is deprecated in favor of VK_EXT_debug_utils.
		// Reference: https://github.com/KhronosGroup/Vulkan-Samples/issues/47
		// VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo
		// {
		// 	.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
		// 	.pNext = pNext,
		// 	.flags = (VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT),
		// 	.pfnCallback = DebugReportCallback,
		// 	.pUserData = nullptr,
		// };
		// pNext = &debugReportCallbackCreateInfo;

		std::vector<const char *> extensionNamesToEnable =
		{
			// VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			// VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME,
			// VK_EXT_VALIDATION_FLAGS_EXTENSION_NAME,
#if defined(CGS_WIN32)
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(CGS_UNIX)
			VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME, // Uncomment if using Wayland for Unix
#endif	// defined(CGS_WIN32)
			VK_KHR_SURFACE_EXTENSION_NAME, // Required for all platforms
		};

		VkInstanceCreateInfo instanceCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = pNext,
			.flags = 0,
			.pApplicationInfo = &applicationInfo,
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = nullptr,
			.enabledExtensionCount = static_cast<uint32_t>(extensionNamesToEnable.size()),
			.ppEnabledExtensionNames = extensionNamesToEnable.data(),
		};

		vr = vkCreateInstance(&instanceCreateInfo, nullptr, &mInstance);
		assert(vr == VK_SUCCESS);

		volkLoadInstance(mInstance);
	}

	void Instance::createDebugUtilsMessenger(CreateInfo& createInfo) noexcept
	{
		[[maybe_unused]] VkResult vr = VK_SUCCESS;

		bool bIsDebugLayerEnabled = false;
#if defined(CGS_DEBUG)
		bIsDebugLayerEnabled = true;
#else  // defined(CGS_DEBUG)
		bool bIsDebugLayerEnabledInConfig = false;
		const bool result = mConfig.GetSetting(CONFIG_ENABLE_DEBUG_LAYER, bIsDebugLayerEnabledInConfig);
		if (result == true)
		{
			bIsDebugLayerEnabled = bIsDebugLayerEnabledInConfig;
		}
#endif // defined(CGS_DEBUG)

		if (bIsDebugLayerEnabled)
		{
			createInfo.DebugUtilsMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.DebugUtilsMessengerCreateInfo.pNext = nullptr;
			vr = vkCreateDebugUtilsMessengerEXT(mInstance, &createInfo.DebugUtilsMessengerCreateInfo, nullptr, &mDebugUtilsMessenger);
			assert(vr == VK_SUCCESS && mDebugUtilsMessenger != VK_NULL_HANDLE);
		}
	}

	void Instance::createPhysicalDeviceGroups(const bool bCreateLogicalDevice) noexcept
	{
		[[maybe_unused]] VkResult vr = VK_SUCCESS;

		uint32_t physicalDeviceGroupCount = 0;
		vr = vkEnumeratePhysicalDeviceGroups(mInstance, &physicalDeviceGroupCount, nullptr);
		assert(vr == VK_SUCCESS);
		assert(physicalDeviceGroupCount > 0);
		CGS_LOG_INFO("Enumerated Physical Device Group Count: %u", physicalDeviceGroupCount);
		if (physicalDeviceGroupCount == 0)
		{
			CGS_LOG_ERROR("No physical devices found.");
			return;
		}
		CGS_LOG_INFO("Enumerating Physical Devices...");
		std::vector<VkPhysicalDeviceGroupProperties> physicalDeviceGroupProperties(physicalDeviceGroupCount);
		vr = vkEnumeratePhysicalDeviceGroups(mInstance, &physicalDeviceGroupCount, physicalDeviceGroupProperties.data());
		assert(vr == VK_SUCCESS);

		uint32_t physicalDeviceGroupToUseIndex = std::numeric_limits<uint32_t>::max();
		const bool bPhysicalDeviceGroupIndexFound = mConfig.GetSetting(CONFIG_PHYSICAL_DEVICE_GROUP_INDEX, physicalDeviceGroupToUseIndex);

		for (uint32_t i = 0; i < physicalDeviceGroupCount; ++i)
		{
			const auto& groupProperties = physicalDeviceGroupProperties[i];
			assert(groupProperties.sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES);
			
			if(bPhysicalDeviceGroupIndexFound == true && i != physicalDeviceGroupToUseIndex)
			{
				continue;
			}

			PhysicalDeviceGroup::CreateInfo createInfo =
			{
				.RhiInstance = *this,
				.Index = i,
				.PhysicalDeviceGroupProperties = groupProperties,
				.bCreateLogicalDevice = bCreateLogicalDevice,
			};
			mPhysicalDeviceGroups.emplace_back(std::make_unique<PhysicalDeviceGroup>(createInfo));
			
			if(bPhysicalDeviceGroupIndexFound == true && i == physicalDeviceGroupToUseIndex)
			{
				mMainPhysicalDeviceGroupIndex = i;
				break;
			}
		}
		
	}
}
