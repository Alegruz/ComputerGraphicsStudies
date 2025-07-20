#pragma once

namespace cgs::graphics::rhi
{
    class Image; // Forward declaration of Image class

    class Attachment final
    {
    public:
        struct CreateInfo final
        {
            Instance& RhiInstance; // Reference to the RHI instance
            std::string Name; // Name of the attachment
            const pugi::xml_node& Node; // XML node containing attachment properties
        };
    
    public:
        explicit Attachment(const CreateInfo& createInfo) noexcept;
        Attachment(const Attachment&) = delete; // Delete copy constructor
        Attachment(Attachment&&) = delete; // Delete move constructor
        CGS_INLINE ~Attachment() noexcept = default; // Default destructor

        CGS_INLINE Attachment& operator=(const Attachment&) = delete; // Delete copy assignment operator
        Attachment& operator=(Attachment&&) = delete; // Delete move assignment operator

        CGS_INLINE const std::string& GetName() const noexcept { return mName; } // Get the name of the attachment
		CGS_INLINE const std::vector<std::unique_ptr<Image>>& GetColorAttachments() const noexcept { return mColorAttachments; } // Get color attachments
		CGS_INLINE const std::vector<std::unique_ptr<Image>>& GetDepthAttachments() const noexcept { return mDepthAttachments; } // Get depth attachments
        CGS_INLINE const Image& GetColorAttachment(uint32_t index = 0) const noexcept
        { 
            return *mColorAttachments[(index < mColorAttachments.size()) ? index : 0]; 
        } // Get a specific color attachment
        CGS_INLINE const Image& GetDepthAttachment(uint32_t index = 0) const noexcept
        { 
            return *mDepthAttachments[(index < mDepthAttachments.size()) ? index : 0]; 
        } // Get a specific depth attachment
        CGS_INLINE constexpr uint32_t GetColorAttachmentCount() const noexcept { return static_cast<uint32_t>(mColorAttachments.size()); } // Get the number of color attachments
        CGS_INLINE constexpr uint32_t GetDepthAttachmentCount() const noexcept { return static_cast<uint32_t>(mDepthAttachments.size()); } // Get the number of depth attachments
		
        CGS_INLINE constexpr bool IsFrameBuffered() const noexcept { return mColorAttachments.size() > 1 || mDepthAttachments.size() > 1; }
        CGS_INLINE constexpr bool IsBackBuffer() const noexcept { return mName == "BackBuffer"; } // Check if the attachment is a back buffer
    
    private:
        [[maybe_unused]] const Instance& mInstance; // Reference to the RHI instance
        std::string mName; // Name of the attachment

        std::vector<std::unique_ptr<Image>> mColorAttachments; // Color attachment image for the back buffer
        std::vector<std::unique_ptr<Image>> mDepthAttachments; // Depth attachment image for the back buffer
    };
} // namespace cgs::graphics::rhi
