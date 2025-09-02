#pragma once

#include "Common/Renderer.h"

#if defined(CGS_GRAPHICS_API_D3D12)
namespace cgs
{
    struct RendererCreateInfo final
    {
        uint32 Width;
        uint32 Height;
        HWND Window;
    };

    bool
    InitializeRenderer(const RendererCreateInfo& createInfo) noexcept;
}   // namespace cgs
#endif  // defined(CGS_GRAPHICS_API_D3D12)