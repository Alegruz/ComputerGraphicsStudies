module;

#include <iostream>
#include <string>
#include <vector>

#include "volk/volk.h"

#include "Core/PchCommon.h"

export module Renderer;

namespace cgs
{
	namespace renderer
	{
		export struct RendererCreateInfo
		{
			cgs::core::ProjectInfo	ApplicationInfo;
			cgs::core::ProjectInfo	EngineInfo;
		};

		export class Renderer
		{
		public:
			static VkBool32 DebugReportCallback(VkDebugReportFlagsEXT flags, [[maybe_unused]] VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, [[maybe_unused]] void* pUserData) noexcept
			{
				bool bBreak = false;
				bool bPrintMessage = false;

				switch (flags)
				{
				case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
					break;
				case VK_DEBUG_REPORT_WARNING_BIT_EXT:
					bBreak = true;
					bPrintMessage = true;
					break;
				case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
					bPrintMessage = true;
					break;
				case VK_DEBUG_REPORT_ERROR_BIT_EXT:
					bBreak = true;
					bPrintMessage = true;
					break;
				case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
					bPrintMessage = true;
					break;
				case VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT:
					[[fallthrough]];
				default:
					assert(false);
					break;
				}

				if (bPrintMessage == true)
				{
					std::cout << std::hex << "object: " << object << " location: " << location << " messageCode: " << messageCode << " layerPrefix: " << pLayerPrefix << " message: " << pMessage << '\n';
				}
				
				if (bBreak == true)
				{
					DEBUG_BREAK();
				}

				return VK_TRUE;
			}

			static VkBool32 DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, [[maybe_unused]] void* pUserData) noexcept
			{
				bool bBreak = false;
				bool bPrintMessage = false;
				switch (messageSeverity)
				{
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
					break;
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
					break;
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
					bBreak = true;
					bPrintMessage = true;
					break;
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
					bBreak = true;
					bPrintMessage = true;
					break;
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
					[[fallthrough]];
				default:
					assert(false);
					break;
				}

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
					switch (messageSeverity)
					{
					case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
						std::cout << "[VERBOSE]";
						break;
					case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
						std::cout << "[INFO]";
						break;
					case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
						std::cout << "[WARNING]";
						break;
					case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
						std::cout << "[SEVERITY]";
						break;
					case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
						[[fallthrough]];
					default:
						assert(false);
						break;
					}

					if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
					{
						//std::cout << "[GENERAL]";
					}
					if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
					{
						//std::cout << "[VALIDATION]";
						bPrintMessage = true;
					}
					if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
					{
						//std::cout << "[PERFORMANCE]";
						bPrintMessage = true;
					}
					if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT)
					{
						assert(false);
					}

					std::cout << "MessageId: [" << pCallbackData->pMessageIdName << ", #: " << pCallbackData->messageIdNumber << "] Message:" << pCallbackData->pMessage << '\n';
					for (uint32_t i = 0; i < pCallbackData->queueLabelCount; ++i)
					{
						const VkDebugUtilsLabelEXT& queueLabel = pCallbackData->pQueueLabels[i];
						std::cout << "\tQueueLabel: " << queueLabel.pLabelName << '\n';
					}
					for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; ++i)
					{
						const VkDebugUtilsLabelEXT& commandBufferLabel = pCallbackData->pCmdBufLabels[i];
						std::cout << "\tCommandBufferLabel: " << commandBufferLabel.pLabelName << '\n';
					}
					for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
					{
						const VkDebugUtilsObjectNameInfoEXT& object = pCallbackData->pObjects[i];
						std::cout << "\tObjectType: " << object.objectType << std::hex << " Handle: " << object.objectHandle << " Name: " << (object.pObjectName ? object.pObjectName : "") << '\n';
					}
				}

				if (bBreak == true)
				{
					DEBUG_BREAK();
				}

				return VK_TRUE;
			}

		public:
			Renderer() = delete;
			CGS_INLINE explicit Renderer(const RendererCreateInfo& createInfo) noexcept
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

				//VkValidationFeaturesEXT validationFeatures =
				//{
				//	.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
				//	.pNext = nullptr,
				//	.enabledValidationFeatureCount = static_cast<uint32_t>(validationFeaturesToEnable.size()),
				//	.pEnabledValidationFeatures = validationFeaturesToEnable.data(),
				//	.disabledValidationFeatureCount = 0,
				//	.pDisabledValidationFeatures = nullptr,
				//};

				VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo
				{
					.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
					//.pNext = &validationFeatures,
					.pNext = nullptr,
					.flags = 0,
					.messageSeverity = (VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT),
					.messageType = (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT),
					.pfnUserCallback = DebugUtilsMessengerCallback,
					.pUserData = nullptr,
				};

				VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo
				{
					.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
					.pNext = &debugUtilsMessengerCreateInfo,
					.flags = (VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT),
					.pfnCallback = DebugReportCallback,
					.pUserData = nullptr,
				};

				std::vector<const char*> extensionNamesToEnable =
				{
					VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
					VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
					//VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME,
					//VK_EXT_VALIDATION_FLAGS_EXTENSION_NAME,
				};

				VkInstanceCreateInfo instanceCreateInfo =
				{
					.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
					.pNext = &debugReportCallbackCreateInfo,
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

			CGS_INLINE constexpr ~Renderer() noexcept
			{
				vkDestroyInstance(mInstance, nullptr);
				mInstance = VK_NULL_HANDLE;
			}

		private:
			VkInstance	mInstance;
		};
	}
}