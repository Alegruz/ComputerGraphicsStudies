#pragma once

#include "Core/pch.h"

namespace cgs::graphics
{
    namespace rhi
    {
        struct InstanceCreateInfo
        {
            cgs::core::ProjectInfo	ApplicationInfo;
            cgs::core::ProjectInfo	EngineInfo;
        };
    }

    struct RendererCreateInfo
    {
        cgs::graphics::rhi::InstanceCreateInfo InstanceCreateInfo;
    };
}