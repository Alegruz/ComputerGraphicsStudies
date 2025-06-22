#include "Graphics/pch.h"

#include "Graphics/RHI/VertexLayout.h"

namespace cgs::graphics::rhi
{
    VertexLayout::VertexLayout(const CreateInfo& createInfo) noexcept
        : mName(createInfo.Name) // Initialize the vertex input name
    {
        // Parse the XML node for vertex elements
        for (pugi::xml_node elementNode : createInfo.Elements.children("Element"))
        {
            VertexElement::CreateInfo elementCreateInfo =
            {
                .SemanticName = elementNode.attribute("SemanticName").as_string(),
                .Format = StringToVkFormat(elementNode.attribute("Format").as_string())
            };

            mElements.emplace_back(elementCreateInfo); // Add the vertex element to the list
        }
    }
} // namespace cgs::graphics::rhi