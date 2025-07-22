#include "Engine/pch.h"

#include "Engine/Engine.h"

#include "Core/Window.h"

#include "Graphics/pch.h"
#include "Graphics/RendererManager.h"

namespace cgs
{
    Engine::Engine(const CreateInfo& createInfo) noexcept
        : mConfig(std::move(createInfo.EngineConfig))
        , mProjectInfo()
        , mRenderer(nullptr)
        , mWindow(createInfo.Window) // Store the window handle for the renderer
		, mbIsRunning(false) // Initialize the running state to false
    {
        initialize(std::move(createInfo.RendererConfig)); // Call the initialization method
    }

    Engine::~Engine() noexcept
    {
        mRenderer.reset(); // Release the renderer
        CGS_LOG_INFO("Engine destroyed.");
    }

    bool Engine::HandleSystemEvents() noexcept
    {
#if defined(CGS_WIN32)
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				mbIsRunning = false; // Stop the engine if a quit message is received
				return false; // Return false to indicate the engine should stop
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
#endif  // defined(CGS_WIN32)
        return true;
    }

    bool Engine::Run() noexcept
	{
		mWindow.Show(); // Show the window for the renderer

        mbIsRunning = true;
		while (mbIsRunning == true)
		{
            HandleSystemEvents(); // Handle system events to check for quit messages
			if (mbIsRunning == false)
			{
				break; // Exit the loop if the engine is not running
			}
			mRenderer->Render(); // Call the render method of the renderer
		}

		return true; // Return true to indicate successful execution
	}

    void Engine::initialize(core::Config&& rendererConfig) noexcept
    {
        [[maybe_unused]] const std::filesystem::path& configFilePath = mConfig.GetConfigFilePath();
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

        CGS_LOG_INFO("Creating RendererManager...");

        // Create the renderer with the provided project information
        graphics::RendererManager::CreateInfo rendererCreateInfo =
        {
            .EngineConfig = mConfig, // Pass the engine configuration to the renderer
            .RendererConfig = std::move(rendererConfig),
            .ApplicationInfo = mProjectInfo,
            .WindowHandle = mWindow.GetWindow(), // Pass the window handle to the renderer
        };

        mRenderer = std::make_unique<graphics::RendererManager>(rendererCreateInfo);
            
        CGS_LOG_INFO("RendererManager created successfully.");
        CGS_LOG_INFO("Engine initialized successfully.");
    }
} // namespace cgs