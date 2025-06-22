#pragma once

namespace cgs::graphics::rhi
{
    class Attachment; // Forward declaration of Attachment class

    class PipelineBase
    {
    public:
        struct CreateInfo final
        {
            std::string Name; // Name of the pipeline
            const pugi::xml_node& Node; // XML node containing pipeline configuration
        };

    public:
        CGS_INLINE explicit PipelineBase(const CreateInfo& createInfo) noexcept
            : mName(createInfo.Name) // Initialize the pipeline name
        {
        }

        CGS_INLINE virtual ~PipelineBase() noexcept = default; // Virtual destructor for proper cleanup

        CGS_INLINE const std::string& GetName() const noexcept { return mName; } // Get the name of the pipeline
        
    protected:
        std::string mName; // Name of the pipeline
    };

    class GraphicsPipeline : public PipelineBase
    {
    public:
        struct CreateInfo final
        {
            PipelineBase::CreateInfo BaseCreateInfo; // Base create info for the pipeline
            std::shared_ptr<Attachment>& Attachment; // Weak pointer to the attachment associated with this pipeline
        };

    public:
        explicit GraphicsPipeline(const CreateInfo& createInfo) noexcept;
        GraphicsPipeline(const GraphicsPipeline&) = delete; // Delete copy constructor
        GraphicsPipeline(GraphicsPipeline&&) noexcept = default; // Move constructor
        CGS_INLINE virtual ~GraphicsPipeline() noexcept = default; // Virtual destructor for proper cleanup
        
        GraphicsPipeline& operator=(const GraphicsPipeline&) = delete; // Delete copy assignment operator
        GraphicsPipeline& operator=(GraphicsPipeline&&) noexcept = default; // Move assignment operator

        CGS_INLINE const Attachment& GetAttachment() const noexcept { return *mAttachment.lock(); } // Get the attachment associated with this pipeline
		CGS_INLINE std::shared_ptr<Attachment> GetAttachmentSharedPtr() const noexcept { return mAttachment.lock(); } // Get a shared pointer to the attachment

    private:
        std::weak_ptr<Attachment> mAttachment; // Weak pointer to the attachment associated with this pipeline
    };
} // namespace cgs::graphics::rhi
