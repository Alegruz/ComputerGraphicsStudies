#pragma once

#include "Graphics/RHI/Instance.h"

namespace cgs::graphics
{
    struct RendererCreateInfo;
    
	class Renderer
	{
	public:
		Renderer() = delete;
		explicit Renderer(const RendererCreateInfo& createInfo) noexcept;
		CGS_INLINE constexpr ~Renderer() noexcept = default;

	private:
		rhi::Instance mInstance;
	};
}