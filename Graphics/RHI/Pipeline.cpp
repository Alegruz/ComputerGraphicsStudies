#include "Graphics/pch.h"

#include "Graphics/RHI/Pipeline.h"

namespace cgs::graphics::rhi
{
    GraphicsPipeline::GraphicsPipeline(const CreateInfo& createInfo) noexcept
        : PipelineBase(createInfo.BaseCreateInfo) // Initialize the base pipeline with the provided create info
        , mAttachment(createInfo.Attachment) // Initialize the weak pointer to the attachment
    {
        // Additional initialization for graphics pipeline can be added here
    }
} // namespace cgs::graphics::rhi