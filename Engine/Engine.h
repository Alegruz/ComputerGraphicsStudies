#pragma once

#include "Core/Config.h"

namespace cgs
{
    namespace core
    {
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
            core::Config::CreateInfo ConfigCreateInfo; // Configuration for the engine
        };

    public:
        Engine() = delete;
        explicit Engine(const CreateInfo& createInfo) noexcept;
        explicit Engine(core::Config&& config, core::Config&& rendererConfig) noexcept;

        Engine(const Engine&) = delete;
        Engine(Engine&&) noexcept = default;
        ~Engine() noexcept;

        Engine& operator=(const Engine&) = delete;
        Engine& operator=(Engine&&) noexcept = delete;

        CGS_INLINE constexpr const core::Config& GetConfig() const noexcept { return mConfig; } // Accessor for the engine configuration

    private:
        void initialize(core::Config&& rendererConfig) noexcept; // Initialize the engine

    private:
        core::Config mConfig; // Configuration for the engine
        core::ProjectInfo mProjectInfo; // Project information for the engine

        std::unique_ptr<graphics::Renderer> mRenderer; // Renderer instance for the engine
    };
} // namespace cgs
