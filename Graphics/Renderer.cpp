#include "Graphics/pch.h"

#include "Graphics/Renderer.h"

#include "Graphics/RHI/Instance.h"

namespace cgs::graphics
{
	Renderer::Renderer(const CreateInfo& createInfo) noexcept
		: mConfig(std::move(createInfo.Config))
		, mInstance()
	{
		const std::filesystem::path& configFilePath = mConfig.GetConfigFilePath();
		CGS_LOG_INFO("Renderer created with configuration from: %s", configFilePath.string().c_str());

		// Retrieve project information from the configuration
		rhi::Instance::CreateInfo instanceCreateInfo =
		{
			.Config = mConfig,
			.ApplicationInfo = createInfo.ApplicationInfo,
		};

		mConfig.CreateProjectInfo(instanceCreateInfo.EngineInfo);

		CGS_LOG_INFO("Renderer initialized with project: %s (Version: %u)", 
			instanceCreateInfo.ApplicationInfo.Name.c_str(),
			instanceCreateInfo.ApplicationInfo.Version);
		CGS_LOG_INFO("Creating RHI Instance...");
		// Create the RHI instance with the provided application and engine information
		mInstance = std::make_unique<rhi::Instance>(instanceCreateInfo);
		CGS_LOG_INFO("RHI Instance created successfully.");
		CGS_LOG_INFO("Renderer initialized successfully.");
	}

	Renderer::~Renderer() noexcept
	{
		mInstance.reset(); // Automatically cleans up the RHI instance
		CGS_LOG_INFO("Renderer destroyed.");
	}
}
