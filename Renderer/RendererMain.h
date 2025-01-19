#pragma once

#include "Core/PchCommon.h"

#if defined(Renderer_EXPORTS)
#define RENDERER_API __declspec(dllexport)
#else	// NOT defined(Renderer_EXPORTS)
#define RENDERER_API __declspec(dllimport)
#endif	// NOT defined(Renderer_EXPORTS)


extern "C" RENDERER_API int RendererMain(const cgs::core::ProjectInfo& applicationInfo) noexcept;