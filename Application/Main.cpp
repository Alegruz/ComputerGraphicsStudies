#include <iostream>

#include "Core/pch.h"
#include "Graphics/Common.h"

#include "Core/Math/Matrix.hpp"
#include "Core/Math/Vector.hpp"

#include "Graphics/Renderer.h"

int main()
{
	cgs::core::ProjectInfo applicationInfo =
	{
		.Name = "RendererMain",
		.Version = MAKE_API_VERSION(0, 0, 0, 1),
	};

	const cgs::core::math::Vector2 vec2(10.0f, 2.0f);
	const cgs::core::math::Vector3 vec3(2.5f, 5.0f, 7.5f);

	cgs::core::math::Matrix mat;

	cgs::graphics::RendererCreateInfo rendererCreateInfo =
	{
		.InstanceCreateInfo =
		{
			.ApplicationInfo = applicationInfo,
			.EngineInfo =
			{
				.Name = "Renderer",
				.Version = MAKE_API_VERSION(0, 0, 0, 1),
			},
		},
	};

	cgs::graphics::Renderer renderer(rendererCreateInfo);

	const int32_t result = static_cast<int>((vec2.X * vec2.Y) + (vec3.X * vec3.Y * vec3.Z)) * static_cast<int>(mat.At(0, 0));

	std::cout << "result: " << result << std::endl;

	return 0;
}