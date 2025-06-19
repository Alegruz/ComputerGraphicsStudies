#pragma once

namespace cgs::graphics::rhi
{
    class Image;
    class Semaphore;

    class BackBuffer final
    {
    public:
        struct CreateInfo final
        {
            std::unique_ptr<Image> ColorAttachment; // The color attachment image for the back buffer
            std::unique_ptr<Image> DepthAttachment; // The depth attachment image for the back buffer, if applicable
            std::unique_ptr<Semaphore> PresentCompletionSemaphore;
            std::unique_ptr<Semaphore> RenderCompletionSemaphore;
        };
    
    public:
        BackBuffer() = delete; // Default constructor is deleted
        explicit BackBuffer(CreateInfo& createInfo) noexcept;
        BackBuffer(const BackBuffer&) = delete; // Copy constructor is deleted
        BackBuffer(BackBuffer&&) noexcept = default; // Move constructor
        ~BackBuffer() noexcept;

        BackBuffer& operator=(const BackBuffer&) = delete; // Copy assignment operator is deleted
        BackBuffer& operator=(BackBuffer&&) noexcept = default; // Move assignment operator

    private:
        std::unique_ptr<Image> mColorAttachment; // Color attachment image for the back buffer
        std::unique_ptr<Image> mDepthAttachment; // Depth attachment image for the back buffer
        std::unique_ptr<Semaphore> mPresentCompletionSemaphore; // Semaphore for present completion
        std::unique_ptr<Semaphore> mRenderCompletionSemaphore; // Semaphore for render
    };
} // namespace cgs::graphics::rhi
