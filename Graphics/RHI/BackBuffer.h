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
        };
    
    public:
        BackBuffer() = delete; // Default constructor is deleted
        explicit BackBuffer(CreateInfo& createInfo) noexcept;
        BackBuffer(const BackBuffer&) = delete; // Copy constructor is deleted
        BackBuffer(BackBuffer&&) noexcept = default; // Move constructor
        ~BackBuffer() noexcept;

        BackBuffer& operator=(const BackBuffer&) = delete; // Copy assignment operator is deleted
        BackBuffer& operator=(BackBuffer&&) noexcept = default; // Move assignment operator

		CGS_INLINE const Image& GetColorAttachment() const noexcept { return *mColorAttachment; } // Accessor for the color attachment image
		CGS_INLINE constexpr const std::unique_ptr<Image>& GetDepthAttachmentOrNull() const noexcept { return mDepthAttachment; } // Accessor for the depth attachment image, if applicable

    private:
        std::unique_ptr<Image> mColorAttachment; // Color attachment image for the back buffer
        std::unique_ptr<Image> mDepthAttachment; // Depth attachment image for the back buffer
    };
} // namespace cgs::graphics::rhi
