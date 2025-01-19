#pragma once

#if defined(RENDERER_EXPORTS)
#define RENDERER_API __declspec(dllexport)
#else	// NOT defined(RENDERER_EXPORTS)
#define RENDERER_API __declspec(dllimport)
#endif	// NOT defined(RENDERER_EXPORTS)

extern "C" RENDERER_API int RendererMain() noexcept;