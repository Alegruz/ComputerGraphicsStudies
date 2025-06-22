#pragma once

namespace cgs::graphics::rhi
{
    class VertexElement final
    {
    public:
        struct CreateInfo final
        {
            std::string SemanticName; // Name of the semantic (e.g., position, normal, color)
            VkFormat Format; // Format of the vertex element (e.g., float3, uint32)
        };
    
    public:
        explicit VertexElement(const CreateInfo& createInfo) noexcept
            : mSemanticName(createInfo.SemanticName), mFormat(createInfo.Format) // Initialize the vertex element with semantic name and format
        {
        }
        
        CGS_INLINE ~VertexElement() noexcept = default; // Default destructor
        
        CGS_INLINE const std::string& GetSemanticName() const noexcept { return mSemanticName; } // Get the semantic name
        CGS_INLINE VkFormat GetFormat() const noexcept { return mFormat; } // Get the format of the vertex element

    private:
        std::string mSemanticName; // Name of the semantic (e.g., position,
        VkFormat mFormat; // Format of the vertex element (e.g., float3, uint32)
    };

    class VertexLayout final
    {
    public:
        struct CreateInfo final
        {
            std::string Name; // Name of the vertex input
            const pugi::xml_node& Elements; // XML node containing vertex elements
        };
    
    public:
        explicit VertexLayout(const CreateInfo& createInfo) noexcept;
        VertexLayout(const VertexLayout&) = delete;
        VertexLayout(VertexLayout&&) = delete;
        CGS_INLINE ~VertexLayout() noexcept = default;
        
        VertexLayout& operator=(const VertexLayout&) = delete;
        VertexLayout& operator=(VertexLayout&&) = delete;

        CGS_INLINE const std::string& GetName() const noexcept { return mName; } // Get the name of the vertex input
        CGS_INLINE const std::vector<VertexElement>& GetElements() const noexcept { return mElements; } // Get the vertex elements
    
    private:
        std::string mName; // Name of the vertex input
        std::vector<VertexElement> mElements; // List of vertex elements
    };
} // namespace cgs::graphics::rhi