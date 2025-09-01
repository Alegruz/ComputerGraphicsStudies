#pragma once

#include "Common/Renderer.h"

#if defined(CGS_GRAPHICS_API_CPU)
namespace cgs
{
    struct RendererCreateInfo final
    {
        uint32 Width;
        uint32 Height;
    };

    struct CpuRenderWork final
    {
        Texture& OutTexture;
        Texture& OutDepthBuffer;
        RenderWork& Work;
    };

    struct SubRenderWork final
    {
        CpuRenderWork& ParentRenderWork;
        
        const Geometry& CurrentGeometry;
        const Geometry& EmissiveGeometry;

        const CornellBoxVertexShaderOutput V0;
        const CornellBoxVertexShaderOutput V1;
        const CornellBoxVertexShaderOutput V2;

        uint32 MinX = 0;
        uint32 MaxX = 0;
        uint32 MinY = 0;
        uint32 MaxY = 0;

        uint64 WorkIndex = 0;
    };

    struct SubRenderThreadInfo final
    {
        std::shared_ptr<ThreadHandle> CurrentThreadHandle;
        eRenderMethod RenderMethod;

        std::mutex RenderWorksMutex;
        std::queue<SubRenderWork> SubRenderWorks;
        std::atomic<uint64> LastCompleteWorkIndex;
        std::atomic<bool> IsActive;

        CGS_INLINE SubRenderThreadInfo() noexcept
            : CurrentThreadHandle(nullptr)
            , RenderMethod(eRenderMethod::DEFAULT)
            , LastCompleteWorkIndex(std::numeric_limits<uint64>::max())
            , IsActive(true)
        {
        }
        SubRenderThreadInfo(const SubRenderThreadInfo&) = delete;
        CGS_INLINE SubRenderThreadInfo(SubRenderThreadInfo&& other) noexcept
        {
            *this = std::move(other);
        }
        CGS_INLINE ~SubRenderThreadInfo() noexcept = default;

        SubRenderThreadInfo& operator=(const SubRenderThreadInfo&) = delete;
        CGS_INLINE SubRenderThreadInfo& operator=(SubRenderThreadInfo&& other) noexcept
        {
            if (this != &other)
            {
                std::lock_guard<std::mutex> lockGuard(other.RenderWorksMutex);
                CurrentThreadHandle = std::move(other.CurrentThreadHandle);
                RenderMethod = other.RenderMethod;
                SubRenderWorks = std::move(other.SubRenderWorks);
                LastCompleteWorkIndex = other.LastCompleteWorkIndex.load();
                IsActive = other.IsActive.load();
            }
            return *this;
        }
    };

    bool
    InitializeRenderer(const RendererCreateInfo& createInfo) noexcept;

    template<eRasterizationMethod METHOD = eRasterizationMethod::DEFAULT>
    void
    Rasterize(CpuRenderWork& renderWork) noexcept;

    void
    SubRasterize(SubRenderWork& work) noexcept;
}   // namespace cgs
#endif  // defined(CGS_GRAPHICS_API_CPU)