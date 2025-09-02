#pragma once

#include "Common/Renderer.h"

#if defined(CGS_GRAPHICS_API_D3D12)
namespace cgs
{
    class Geometry final
    {
    public:
        CGS_INLINE constexpr 
        Geometry() noexcept: mIsEmissive(false), mName() {}
        CGS_INLINE constexpr
        Geometry(const std::string& name) noexcept: mIsEmissive(false), mName(name) {}
        CGS_INLINE
        ~Geometry() noexcept = default;

        CGS_INLINE constexpr void
        SetIsEmissive(const bool isEmissive) noexcept { mIsEmissive = isEmissive; }
        CGS_INLINE constexpr void
        SetName(const std::string& name) noexcept { mName = name; }

        [[nodiscard]] CGS_INLINE constexpr bool
        IsEmissive() const noexcept { return mIsEmissive; }
        [[nodiscard]] CGS_INLINE constexpr const std::string&
        GetName() const noexcept { return mName; }

    private:
        bool mIsEmissive;
        std::string mName;
    };

    struct RendererCreateInfo final
    {
        uint32 Width;
        uint32 Height;
        HWND Window;
    };

    void
    DestroyRenderer() noexcept;

    bool
    InitializeRenderer(const RendererCreateInfo& createInfo) noexcept;
}   // namespace cgs
#endif  // defined(CGS_GRAPHICS_API_D3D12)