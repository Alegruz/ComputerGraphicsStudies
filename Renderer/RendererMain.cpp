#include "Renderer/RendererMain.h"

import Core.Math.Matrix;
import Core.Math.Vector;

int RendererMain() noexcept
{
	const cgs::core::Vector2 vec2(10.0f, 2.0f);
	const cgs::core::Vector3 vec3(2.5f, 5.0f, 7.5f);

	cgs::core::Matrix mat;

	return static_cast<int>((vec2.X * vec2.Y) + (vec3.X * vec3.Y * vec3.Z)) * static_cast<int>(mat.At(0, 0));
}