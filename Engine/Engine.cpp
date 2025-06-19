#include "Engine/pch.h"

#include "Engine/Engine.h"

#include "Graphics/pch.h"
#include "Graphics/Renderer.h"

namespace cgs
{
    Engine::Engine(const CreateInfo& createInfo) noexcept
        : mConfig(createInfo.ConfigCreateInfo)
        , mProjectInfo()
    {
        std::filesystem::path rendererConfigFilePath;
        const bool result = mConfig.GetSetting(CONFIG_RENDERER_CONFIG_FILE_PATH, rendererConfigFilePath);
        if (result == false)
        {
            CGS_LOG_INFO("Using default path 'Engine/config.ini'.");
            rendererConfigFilePath = "Engine/config.ini"; // Default path if not specified
            mConfig.SetSetting(CONFIG_RENDERER_CONFIG_FILE_PATH, rendererConfigFilePath.string());
        }
        else
        {
            CGS_LOG_INFO("Using renderer configuration file: %s", rendererConfigFilePath.string().c_str());
        }

        cgs::core::Config rendererConfig = core::Config(core::Config::CreateInfo{.ConfigFilePath = rendererConfigFilePath});
        initialize(std::move(rendererConfig)); // Call the initialization method
    }

    Engine::Engine(core::Config&& config, core::Config&& rendererConfig) noexcept
        : mConfig(std::move(config))
        , mProjectInfo()
    {
        initialize(std::move(rendererConfig)); // Call the initialization method
    }

    Engine::~Engine() noexcept
    {
        mRenderer.reset(); // Release the renderer
        CGS_LOG_INFO("Engine destroyed.");
    }

    void Engine::initialize(core::Config&& rendererConfig) noexcept
    {
        const std::filesystem::path& configFilePath = mConfig.GetConfigFilePath();
        CGS_LOG_INFO("Engine created with configuration from: %s", configFilePath.string().c_str());

        mConfig.CreateProjectInfo(mProjectInfo); // Create project information from the configuration

        std::string minimumLogLevel;
        bool result = mConfig.GetSetting(CONFIG_MINIMUM_LOG_LEVEL, minimumLogLevel);
        if (result == false)
        {
            minimumLogLevel = "Info"; // Default log level if not specified
            mConfig.SetSetting(CONFIG_MINIMUM_LOG_LEVEL, minimumLogLevel);
        }
        cgs::core::Log::GetInstance().SetLogLevel(cgs::core::Log::eLogLevelFromString(minimumLogLevel.c_str()));

        CGS_LOG_INFO("Creating Renderer...");

        // Create the renderer with the provided project information
        graphics::Renderer::CreateInfo rendererCreateInfo =
        {
            .Config = std::move(rendererConfig),
            .ApplicationInfo = mProjectInfo,
        };

        mRenderer = std::make_unique<graphics::Renderer>(rendererCreateInfo);
            
        CGS_LOG_INFO("Renderer created successfully.");
        CGS_LOG_INFO("Engine initialized successfully.");
    }
} // namespace cgs