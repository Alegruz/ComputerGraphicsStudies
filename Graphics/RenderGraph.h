#pragma once

namespace cgs::graphics
{
    class RenderGraphNode final
    {
    public:
        enum class Type : uint8_t
        {
            RENDER,
        };

        struct CreateInfo final
        {
            Type NodeType; // Type of the render graph node
            std::string RendererName; // Name of the renderer associated with this node
        };

    public:
        static void LinkNodes(std::shared_ptr<RenderGraphNode>& parent, std::shared_ptr<RenderGraphNode>& child) noexcept
        {
            if (parent && child)
            {
                parent->mChildren.push_back(child); // Add child to parent's children
                child->mParent = parent; // Set parent for the child
            }
        }
    
    public:
        CGS_INLINE explicit RenderGraphNode(const CreateInfo& createInfo) noexcept
            : mType(createInfo.NodeType) // Set the type of the render graph node
            , mRendererName(createInfo.RendererName) // Set the renderer name
        {
        }

        CGS_INLINE virtual ~RenderGraphNode() noexcept = default;

        CGS_INLINE Type GetType() const noexcept { return mType; }
        CGS_INLINE const std::string& GetRendererName() const noexcept { return mRendererName; }

        CGS_INLINE const std::weak_ptr<RenderGraphNode>& GetParentOrNull() const noexcept { return mParent; } // Get the parent node
        CGS_INLINE const std::vector<std::shared_ptr<RenderGraphNode>>& GetChildren() const noexcept { return mChildren; } // Get the child nodes
        CGS_INLINE std::vector<std::shared_ptr<RenderGraphNode>>& GetChildren() noexcept { return mChildren; } // Get the child nodes
    
    private:
        std::weak_ptr<RenderGraphNode> mParent; // Parent node in the render graph
        std::vector<std::shared_ptr<RenderGraphNode>> mChildren; // Child nodes
        Type mType; // Type of the render graph node
        std::string mRendererName; // Name of the renderer associated with this node
    };

    class RenderGraph final
    {
    public:
        struct CreateInfo final
        {
            std::filesystem::path RenderGraphFilePath; // Path to the render graph file
        };
    
    public:
        RenderGraph(const CreateInfo& createInfo) noexcept;
        CGS_INLINE RenderGraph(const RenderGraph&) = delete; // Disable copy constructor
        CGS_INLINE RenderGraph(RenderGraph&&) noexcept = default; // Default move constructor
        CGS_INLINE virtual ~RenderGraph() noexcept = default;
        
        CGS_INLINE RenderGraph& operator=(const RenderGraph&) = delete; // Disable copy assignment operator
        CGS_INLINE RenderGraph& operator=(RenderGraph&&) noexcept = default; // Default move assignment operator

        CGS_INLINE const std::filesystem::path& GetRenderGraphFilePath() const noexcept { return mRenderGraphFilePath; } // Get the render graph file path
        CGS_INLINE const std::shared_ptr<RenderGraphNode>& GetHeadNode() const noexcept { return mHeadNode; } // Get the head node of the render graph
        CGS_INLINE const std::shared_ptr<RenderGraphNode>& GetTailNode() const noexcept { return mTailNode; } // Get the tail node of the render graph
        CGS_INLINE const std::vector<std::shared_ptr<RenderGraphNode>>& GetNodes() const noexcept { return mNodes; } // Get all nodes in the render graph
    
    private:
        std::filesystem::path mRenderGraphFilePath; // Path to the render graph file
        std::shared_ptr<RenderGraphNode> mHeadNode; // Head node of the render graph
        std::shared_ptr<RenderGraphNode> mTailNode; // Tail node of the render graph
        std::vector<std::shared_ptr<RenderGraphNode>> mNodes; // All nodes in the render graph
    };
} // namespace cgs::graphics