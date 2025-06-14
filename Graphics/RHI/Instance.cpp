#include "Graphics/RHI/Instance.h"

#include <iostream>
#include <string>
#include <vector>

#include "volk/volk.h"

#include "Core/pch.h"

#include "Graphics/Common.h"

#include "Graphics/RHI/PhysicalDevice.h"

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
		std::cout << logMsg;

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
    
	Instance::Instance(const InstanceCreateInfo& createInfo) noexcept
		: mInstance(VK_NULL_HANDLE)
	{
		VkResult vr = volkInitialize();
		assert(vr == VK_SUCCESS);

		uint32_t apiVersion = 0;
		vr = vkEnumerateInstanceVersion(&apiVersion);
		assert(vr == VK_SUCCESS);
		std::cout << "Vulkan Instance Version: " << VK_API_VERSION_VARIANT(apiVersion) << '.' << VK_API_VERSION_MAJOR(apiVersion) << '.' << VK_API_VERSION_MINOR(apiVersion) << '.' << VK_API_VERSION_PATCH(apiVersion) << '\n';

		VkApplicationInfo applicationInfo =
		{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = createInfo.ApplicationInfo.Name.c_str(),
			.applicationVersion = createInfo.ApplicationInfo.Version,
			.pEngineName = createInfo.EngineInfo.Name.c_str(),
			.engineVersion = createInfo.EngineInfo.Version,
			.apiVersion = apiVersion,
		};

		std::vector<VkValidationFeatureEnableEXT> validationFeaturesToEnable =
		{
			VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
			VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
			VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
			VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
			VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
		};

		void* pNext = nullptr;
		//VkValidationFeaturesEXT validationFeatures =
		//{
		//	.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
		//	.pNext = pNext,
		//	.enabledValidationFeatureCount = static_cast<uint32_t>(validationFeaturesToEnable.size()),
		//	.pEnabledValidationFeatures = validationFeaturesToEnable.data(),
		//	.disabledValidationFeatureCount = 0,
		//	.pDisabledValidationFeatures = nullptr,
		//};
		//pNext = &validationFeatures;

		VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo
		{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.pNext = pNext,
			.flags = 0,
			.messageSeverity = (VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT),
			.messageType = (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT),
			.pfnUserCallback = DebugUtilsMessengerCallback,
			.pUserData = nullptr,
		};
		pNext = &debugUtilsMessengerCreateInfo;

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

		std::vector<const char*> extensionNamesToEnable =
		{
			// VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			//VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME,
			//VK_EXT_VALIDATION_FLAGS_EXTENSION_NAME,
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

		uint32_t physicalDeviceCount = 0;
		vr = vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, nullptr);
		assert(vr == VK_SUCCESS && physicalDeviceCount > 0);

		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
		vr = vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, physicalDevices.data());
		assert(vr == VK_SUCCESS && physicalDeviceCount > 0);

		struct PhysicalDeviceComparator final
		{
			CGS_INLINE bool operator()(const std::unique_ptr<PhysicalDevice>& lhs, const std::unique_ptr<PhysicalDevice>& rhs) const noexcept
			{
				return ( lhs == nullptr || rhs == nullptr ) || ( lhs->EvaluateScore() < rhs->EvaluateScore() );
			}
		};

		std::priority_queue<std::unique_ptr<PhysicalDevice>, std::vector<std::unique_ptr<PhysicalDevice>>, PhysicalDeviceComparator> physicalDevicesToCreate;
		for (uint32_t i = 0; i < physicalDeviceCount; ++i)
		{
			PhysicalDevice::CreateInfo physicalDeviceCreateInfo =
			{
				.RhiInstance = *this,
				.PhysicalDevice = physicalDevices[i],
			};
			std::unique_ptr<PhysicalDevice> physicalDevice = std::make_unique<PhysicalDevice>(physicalDeviceCreateInfo);
			assert(physicalDevice->mPhysicalDevice != VK_NULL_HANDLE);
			physicalDevicesToCreate.push(std::move(physicalDevice));
		}
		
		mPhysicalDevices.reserve(physicalDeviceCount);
		while (!physicalDevicesToCreate.empty())
		{
			auto device = std::move(const_cast<std::unique_ptr<PhysicalDevice>&>(physicalDevicesToCreate.top()));
			mPhysicalDevices.push_back(std::move(device));
			physicalDevicesToCreate.pop();
		}
	}

	Instance::~Instance() noexcept
	{
		mPhysicalDevices.clear();

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