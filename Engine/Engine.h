#pragma once

#include "Core/Config.h"

namespace cgs
{
    namespace core
    {
        class Window;
    } // namespace core
    
    namespace graphics
    {
        class Renderer; // Forward declaration of Renderer class
    } // namespace graphics

    class Engine final
    {
    public:
        struct CreateInfo final
        {
            core::Config&& EngineConfig;
            core::Config&& RendererConfig;
			core::Window& Window; // Reference to the window for the renderer
        };

    public:
        Engine() = delete;
        explicit Engine(const CreateInfo& createInfo) noexcept;

        Engine(const Engine&) = delete;
        Engine(Engine&&) noexcept = default;
        ~Engine() noexcept;

        Engine& operator=(const Engine&) = delete;
        Engine& operator=(Engine&&) noexcept = delete;

		bool HandleSystemEvents() noexcept; // Handle system events, such as window events or input events
		bool Run() noexcept; // Run the engine, typically starts the main loop
		CGS_INLINE constexpr void Stop() noexcept { mbIsRunning = false; } // Stop the engine, sets the running flag to false

        CGS_INLINE constexpr const core::Config& GetConfig() const noexcept { return mConfig; } // Accessor for the engine configuration

    private:
        void initialize(core::Config&& rendererConfig) noexcept; // Initialize the engine

    private:
        core::Config mConfig; // Configuration for the engine
        core::ProjectInfo mProjectInfo; // Project information for the engine

        std::unique_ptr<graphics::Renderer> mRenderer; // Renderer instance for the engine
        core::Window& mWindow;
		bool mbIsRunning; // Flag to indicate if the engine is running
    };
} // namespace cgs
