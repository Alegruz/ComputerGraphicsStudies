module;

#include <iostream>
#include <string>

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

				VkInstanceCreateInfo instanceCreateInfo =
				{
					.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.pApplicationInfo = &applicationInfo,
					.enabledLayerCount = 0,
					.ppEnabledLayerNames = nullptr,
					.enabledExtensionCount = 0,
					.ppEnabledExtensionNames = nullptr,
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