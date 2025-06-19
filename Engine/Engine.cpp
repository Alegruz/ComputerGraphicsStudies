#include "Engine/pch.h"

#include "Engine/Engine.h"

#include "Graphics/pch.h"
#include "Graphics/Renderer.h"

namespace cgs
{
    Engine::Engine(const CreateInfo& createInfo) noexcept
        : mConfig(std::move(createInfo.EngineConfig))
        , mProjectInfo()
        , mRenderer(nullptr)
        , mWindowHandle(createInfo.WindowHandle) // Store the window handle for the renderer
    {
        initialize(std::move(createInfo.RendererConfig)); // Call the initialization method
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
            .WindowHandle = mWindowHandle, // Pass the window handle to the renderer
        };

        mRenderer = std::make_unique<graphics::Renderer>(rendererCreateInfo);
            
        CGS_LOG_INFO("Renderer created successfully.");
        CGS_LOG_INFO("Engine initialized successfully.");
    }
} // namespace cgs