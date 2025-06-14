#include "Graphics/Renderer.h"

#include "Graphics/RHI/Instance.h"

namespace cgs::graphics
{
	Renderer::Renderer(const RendererCreateInfo& createInfo) noexcept
		: mConfig(createInfo.ConfigCreateInfo)
		, mInstance()
	{
		const std::filesystem::path& configFilePath = mConfig.GetConfigFilePath();
		CGS_LOG_INFO("Renderer created with configuration from: %s", configFilePath.string().c_str());

		// Retrieve project information from the configuration
		std::string projectName;
		bool result = mConfig.GetSetting("Name", projectName);
		if (!result)
		{
			CGS_LOG_ERROR("Failed to retrieve project name from configuration. Using default name 'DefaultProject'.");
			projectName = "DefaultProject";
		}
		mConfig.SetSetting("Name", projectName);
		CGS_LOG_INFO("Project Name: %s", projectName.c_str());
		uint32_t projectVersionVariant;
		result = mConfig.GetSetting("VersionVariant", projectVersionVariant);
		if (!result)
		{
			CGS_LOG_ERROR("Failed to retrieve project version variant from configuration. Using default value 0.");
			projectVersionVariant = 0;
		}
		mConfig.SetSetting("VersionVariant", projectVersionVariant);
		uint32_t projectVersionMajor;
		result = mConfig.GetSetting("VersionMajor", projectVersionMajor);
		if (!result)
		{
			CGS_LOG_ERROR("Failed to retrieve project version major from configuration. Using default value 0.");
			projectVersionMajor = 0;
		}
		mConfig.SetSetting("VersionMajor", projectVersionMajor);
		uint32_t projectVersionMinor;
		result = mConfig.GetSetting("VersionMinor", projectVersionMinor);
		if (!result)
		{
			CGS_LOG_ERROR("Failed to retrieve project version minor from configuration. Using default value 0.");
			projectVersionMinor = 0;
		}
		mConfig.SetSetting("VersionMinor", projectVersionMinor);
		uint32_t projectVersionPatch;
		result = mConfig.GetSetting("VersionPatch", projectVersionPatch);
		if (!result)
		{
			CGS_LOG_ERROR("Failed to retrieve project version patch from configuration. Using default value 1.");
			projectVersionPatch = 1;
		}
		mConfig.SetSetting("VersionPatch", projectVersionPatch);

		rhi::InstanceCreateInfo instanceCreateInfo =
		{
			.ApplicationInfo = createInfo.ApplicationInfo,
			.EngineInfo =
			{
				.Name = projectName.c_str(),
				.Version = MAKE_API_VERSION(projectVersionVariant, projectVersionMajor, projectVersionMinor, projectVersionPatch),
			},
		};

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