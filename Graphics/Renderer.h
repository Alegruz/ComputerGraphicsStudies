#pragma once

#include "Graphics/RHI/Instance.h"

namespace cgs::graphics
{
    struct RendererCreateInfo
    {
        cgs::core::Config::CreateInfo   ConfigCreateInfo; // Configuration for the renderer
        cgs::core::ProjectInfo          ApplicationInfo;
    };
    
	class Renderer
	{
	public:
		Renderer() = delete;
		explicit Renderer(const RendererCreateInfo& createInfo) noexcept;
		~Renderer() noexcept;

	private:
		cgs::core::Config mConfig; // Configuration for the renderer
		std::unique_ptr<rhi::Instance> mInstance;
	};
}