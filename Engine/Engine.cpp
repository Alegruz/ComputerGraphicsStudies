#include "Engine/pch.h"

#include "Engine/Engine.h"

#include "Graphics/Renderer.h"

namespace cgs
{
    Engine::Engine(const CreateInfo& createInfo) noexcept
        : mConfig(createInfo.ConfigCreateInfo)
        , mProjectInfo()
    {
        const std::filesystem::path& configFilePath = mConfig.GetConfigFilePath();
        CGS_LOG_INFO("Engine created with configuration from: %s", configFilePath.string().c_str());

        std::string projectName;
        bool result = mConfig.GetSetting("Name", projectName);
        if (result == false)
        {
            CGS_LOG_ERROR("Failed to retrieve project name from configuration. Using default name 'DefaultProject'.");
            projectName = "DefaultProject";
            mConfig.SetSetting("Name", projectName);
        }
        mProjectInfo.Name = projectName.c_str();

        uint32_t projectVersionVariant;
        result = mConfig.GetSetting("VersionVariant", projectVersionVariant);
        if (result == false)
        {
            CGS_LOG_ERROR("Failed to retrieve project version variant from configuration. Using default value 0.");
            projectVersionVariant = 0;
            mConfig.SetSetting("VersionVariant", projectVersionVariant);
        }
        uint32_t projectVersionMajor;
        result = mConfig.GetSetting("VersionMajor", projectVersionMajor);
        if (result == false)
        {
            CGS_LOG_ERROR("Failed to retrieve project version major from configuration. Using default value 0.");
            projectVersionMajor = 0;
            mConfig.SetSetting("VersionMajor", projectVersionMajor);
        }
        uint32_t projectVersionMinor;
        result = mConfig.GetSetting("VersionMinor", projectVersionMinor);
        if (result == false)
        {
            CGS_LOG_ERROR("Failed to retrieve project version minor from configuration. Using default value 0.");
            projectVersionMinor = 0;
            mConfig.SetSetting("VersionMinor", projectVersionMinor);
        }
        uint32_t projectVersionPatch;
        result = mConfig.GetSetting("VersionPatch", projectVersionPatch);
        if (result == false)
        {
            CGS_LOG_ERROR("Failed to retrieve project version patch from configuration. Using default value 1.");
            projectVersionPatch = 1;
            mConfig.SetSetting("VersionPatch", projectVersionPatch);
        }
        mProjectInfo.Version = MAKE_API_VERSION(projectVersionVariant, projectVersionMajor, projectVersionMinor, projectVersionPatch);
        CGS_LOG_INFO("Project Info - Name: %s, Version: %u", mProjectInfo.Name.c_str(), mProjectInfo.Version);
        CGS_LOG_INFO("Engine initialized with project: %s (Version: %u)", mProjectInfo.Name.c_str(), mProjectInfo.Version);

        std::string minimumLogLevel;
        result = mConfig.GetSetting("MinimumLogLevel", minimumLogLevel);
        if (result == false)
        {
            minimumLogLevel = "Info"; // Default log level if not specified
            mConfig.SetSetting("MinimumLogLevel", minimumLogLevel);
        }
        cgs::core::Log::GetInstance().SetLogLevel(cgs::core::Log::eLogLevelFromString(minimumLogLevel.c_str()));

        CGS_LOG_INFO("Creating Renderer...");

        // Create the renderer with the provided project information

        std::filesystem::path rendererConfigFilePath;
        result = mConfig.GetSetting("RendererConfigFilePath", rendererConfigFilePath);
        if (result == false)
        {
            CGS_LOG_INFO("Using default path 'Engine/config.ini'.");
            rendererConfigFilePath = "Engine/config.ini"; // Default path if not specified
            mConfig.SetSetting("RendererConfigFilePath", rendererConfigFilePath.string());
        }
        else
        {
            CGS_LOG_INFO("Using renderer configuration file: %s", rendererConfigFilePath.string().c_str());
        }

        graphics::Renderer::CreateInfo rendererCreateInfo =
        {
            .ConfigCreateInfo = core::Config::CreateInfo
            {
                .ConfigFilePath = rendererConfigFilePath
            },
            .ApplicationInfo = mProjectInfo,
        };

        mRenderer = std::make_unique<graphics::Renderer>(rendererCreateInfo);
        CGS_LOG_INFO("Renderer created successfully.");
        CGS_LOG_INFO("Engine initialized successfully.");
    }

    Engine::~Engine() noexcept
    {
        mRenderer.reset(); // Release the renderer
        CGS_LOG_INFO("Engine destroyed.");
    }
} // namespace cgs