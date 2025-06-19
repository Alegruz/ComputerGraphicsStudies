#include "Graphics/pch.h"

#include "Graphics/RHI/BackBuffer.h"

#include "Graphics/RHI/Image.h"
#include "Graphics/RHI/Semaphore.h"

namespace cgs::graphics::rhi
{
    BackBuffer::BackBuffer(CreateInfo& createInfo) noexcept
        : mColorAttachment(std::move(createInfo.ColorAttachment))
        , mDepthAttachment(std::move(createInfo.DepthAttachment))
        , mPresentCompletionSemaphore(std::move(createInfo.PresentCompletionSemaphore))
        , mRenderCompletionSemaphore(std::move(createInfo.RenderCompletionSemaphore))
    {
        assert(mColorAttachment != nullptr);
        assert(mDepthAttachment != nullptr); // Depth attachment
        assert(mPresentCompletionSemaphore != nullptr);
        assert(mRenderCompletionSemaphore != nullptr);
    }

    BackBuffer::~BackBuffer() noexcept
    {
        // The unique_ptr will automatically clean up the resources
        mColorAttachment.reset();
        mDepthAttachment.reset();
        mPresentCompletionSemaphore.reset();
        mRenderCompletionSemaphore.reset();
    }
} // namespace cgs::graphics::rhi
