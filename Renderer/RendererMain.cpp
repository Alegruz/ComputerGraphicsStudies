#include "Renderer/RendererMain.h"

#include "volk/volk.h"

import Core.Math.Matrix;
import Core.Math.Vector;
import Renderer;

int RendererMain(const cgs::core::ProjectInfo& applicationInfo) noexcept
{
	const cgs::core::Vector2 vec2(10.0f, 2.0f);
	const cgs::core::Vector3 vec3(2.5f, 5.0f, 7.5f);

	cgs::core::Matrix mat;

	cgs::renderer::RendererCreateInfo rendererCreateInfo =
	{
		.ApplicationInfo = applicationInfo,
		.EngineInfo =
		{
			.Name = "Renderer",
			.Version = MAKE_API_VERSION(0, 0, 0, 1),
		},
	};

	cgs::renderer::Renderer renderer(rendererCreateInfo);

	return static_cast<int>((vec2.X * vec2.Y) + (vec3.X * vec3.Y * vec3.Z)) * static_cast<int>(mat.At(0, 0));
}