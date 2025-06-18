#include "Engine/pch.h"

#include "Engine/Engine.h"

#include "Graphics/Renderer.h"

namespace cgs
{
    Engine::Engine(const CreateInfo& createInfo) noexcept
        : mConfig(createInfo.ConfigCreateInfo)
        , mProjectInfo()
    {
        initialize(); // Call the initialization method
    }

    Engine::Engine(core::Config&& config) noexcept
        : mConfig(std::move(config))
        , mProjectInfo()
    {
        initialize(); // Call the initialization method
    }

    Engine::~Engine() noexcept
    {
        mRenderer.reset(); // Release the renderer
        CGS_LOG_INFO("Engine destroyed.");
    }

    void Engine::initialize() noexcept
    {
        const std::filesystem::path& configFilePath = mConfig.GetConfigFilePath();
        CGS_LOG_INFO("Engine created with configuration from: %s", configFilePath.string().c_str());

        mConfig.CreateProjectInfo(mProjectInfo); // Create project information from the configuration

        std::string minimumLogLevel;
        bool result = mConfig.GetSetting("MinimumLogLevel", minimumLogLevel);
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
} // namespace cgs