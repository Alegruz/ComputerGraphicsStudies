#include "Graphics/Renderer.h"

#include "Graphics/Common.h"

namespace cgs::graphics
{
	Renderer::Renderer(const RendererCreateInfo& createInfo) noexcept
		: mInstance(createInfo.InstanceCreateInfo)
	{
	}
}