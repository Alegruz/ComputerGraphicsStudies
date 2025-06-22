#include "Graphics/pch.h"

#include "Graphics/RenderGraph.h"

namespace cgs::graphics
{
    RenderGraph::RenderGraph(const CreateInfo& createInfo) noexcept
        : mRenderGraphFilePath(createInfo.RenderGraphFilePath) // Store the render graph file path
        , mHeadNode(nullptr) // Initialize the head node to nullptr
        , mTailNode(nullptr) // Initialize the tail node to nullptr
        , mNodes() // Initialize the nodes vector
    {
        // Load the render graph from the specified XML file
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(mRenderGraphFilePath.string().c_str());
        if (!result)
        {
            CGS_LOG_ERROR("Failed to load render graph file '%s': %s", mRenderGraphFilePath.string().c_str(), result.description());
            return; // Exit if the file could not be loaded
        }

        // Parse the XML document and create render graph nodes
        pugi::xml_node renderGraphNode = doc.child("RenderGraph");
        for(pugi::xml_node node : renderGraphNode.children())
        {
            const std::string nodeType = node.name();
            const std::string rendererName = node.attribute("Renderer").as_string();

            std::shared_ptr<RenderGraphNode> renderNode;
            if (nodeType == "Render")
            {
                renderNode = std::make_shared<RenderGraphNode>(RenderGraphNode::CreateInfo{ .NodeType = RenderGraphNode::Type::RENDER, .RendererName = rendererName });
            }
            else
            {
                CGS_LOG_WARNING("Unknown render graph node type '%s' in file '%s'", nodeType.c_str(), mRenderGraphFilePath.string().c_str());
                continue; // Skip unknown node types
            }
            
            // mHeadNode;
            // mTailNode;
            if (!mHeadNode)
            {
                mHeadNode = renderNode; // Set the first node as the head
                mTailNode = renderNode; // Also set it as the tail
            }
            else
            {
                RenderGraphNode::LinkNodes(mTailNode, renderNode); // Link the new node to the tail
                mTailNode = renderNode; // Update the tail to the new node
            }
        }
    }
} // namespace cgs::graphics
