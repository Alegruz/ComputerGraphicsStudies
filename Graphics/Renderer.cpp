#include "Graphics/pch.h"

#include "Graphics/Renderer.h"

#include "Graphics/RHI/Attachment.h"
#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/Device.h"
#include "Graphics/RHI/Image.h"
#include "Graphics/RHI/Instance.h"
#include "Graphics/RHI/Pipeline.h"
#include "Graphics/RHI/PhysicalDevice.h"
#include "Graphics/RHI/PhysicalDeviceGroup.h"
#include "Graphics/RHI/QueueFamily.h"
#include "Graphics/RHI/SwapChain.h"

namespace cgs::graphics
{
	void RenderCommand<eRenderCommand::DRAW>::Execute(rhi::CommandBuffer& commandBuffer) noexcept
	{
		const rhi::PhysicalDevice& physicalDevice = mInstance.GetMainPhysicalDeviceGroup().GetMainPhysicalDevice();
		const rhi::QueueFamily& queueFamily = physicalDevice.GetMainQueueFamily();

		const uint32_t currentFrameBufferIndex = commandBuffer.GetFrameBufferIndex();

		const rhi::GraphicsPipeline& graphicsPipeline = static_cast<const rhi::GraphicsPipeline&>(mPipeline);
		const rhi::Attachment& attachment = graphicsPipeline.GetAttachment();
		const rhi::Image& colorAttachment = attachment.GetColorAttachment(currentFrameBufferIndex);
		[[maybe_unused]] const rhi::Image& depthAttachment = attachment.GetDepthAttachment(currentFrameBufferIndex);

		VkImageMemoryBarrier imageMemoryBarrier =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = 0, // No source access mask
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // Destination access mask for color attachment
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, // Old layout is undefined
			.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // New layout is optimal for color attachment
			.srcQueueFamilyIndex = queueFamily.GetIndex(), // Source queue family index
			.dstQueueFamilyIndex = queueFamily.GetIndex(), // Destination queue family index
			.image = colorAttachment.GetVkImage(), // Image to transition
			.subresourceRange =
			{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, // Aspect mask for color attachment
				.baseMipLevel = 0, // Base mip level
				.levelCount = 1, // Level count is 1
				.baseArrayLayer = 0, // Base array layer
				.layerCount = 1 // Layer count is 1
			}
		};

		vkCmdPipelineBarrier(
			commandBuffer.GetVkCommandBuffer(),
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // Destination stage is color attachment output
			0, // No dependency flags
			0, nullptr, // No memory barriers
			0, nullptr, // No buffer barriers
			1, &imageMemoryBarrier // Single image memory barrier
		);

		VkRenderingAttachmentInfo renderingAttachmentInfo =
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.pNext = nullptr,
			.imageView = colorAttachment.GetVkImageView(), // Image view for the color attachment
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // Layout for the color attachment
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // Clear the attachment at the start
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE, // Store the result
			.clearValue = { 0.0f, 0.0f, 0.2f, 1.0f } // Clear value for the color attachment
		};

		const uint32_t width = colorAttachment.GetWidth();
		const uint32_t height = colorAttachment.GetHeight();

		VkRenderingInfo renderingInfo =
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.pNext = nullptr,
			.flags = 0, // No special flags
			.renderArea = {.offset = {.x = 0, .y = 0 }, .extent = {.width = width, .height = height } }, // Render area based on the back buffer size
			.layerCount = 1, // Single layer for 2D rendering
			.viewMask = 0, // No view mask for single view rendering
			.colorAttachmentCount = 1, // Single color attachment
			.pColorAttachments = &renderingAttachmentInfo, // Pointer to the color attachment info
			//.pDepthAttachment = {}, // No depth attachment for this example
			//.pStencilAttachment = {} // No stencil attachment for this example
		};

		vkCmdBeginRendering(commandBuffer.GetVkCommandBuffer(), &renderingInfo);

		VkViewport viewport =
		{
			.x = 0.0f,
			.y = 0.0f,
			.width = static_cast<float>(width),
			.height = static_cast<float>(height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		vkCmdSetViewport(
			commandBuffer.GetVkCommandBuffer(),
			0, // First viewport index
			1, // Single viewport
			&viewport // Pointer to the viewport
		);

		VkRect2D scissor =
		{
			.offset = { 0, 0 }, // Scissor offset
			.extent = { width, height } // Scissor extent based on the back buffer size
		};
		vkCmdSetScissor(
			commandBuffer.GetVkCommandBuffer(),
			0, // First scissor index
			1, // Single scissor
			&scissor // Pointer to the scissor rectangle
		);

		vkCmdEndRendering(commandBuffer.GetVkCommandBuffer());

		imageMemoryBarrier =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // Source access mask for color attachment write
			.dstAccessMask = 0, // No destination access mask
			.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // Old layout is optimal for color attachment
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // New layout is present source
			.srcQueueFamilyIndex = queueFamily.GetIndex(), // Source queue family index
			.dstQueueFamilyIndex = queueFamily.GetIndex(), // Destination queue family index
			.image = colorAttachment.GetVkImage(), // Image to transition
			.subresourceRange =
			{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, // Aspect mask for color attachment
				.baseMipLevel = 0, // Base mip level
				.levelCount = 1, // Level count is 1
				.baseArrayLayer = 0, // Base array layer
				.layerCount = 1 // Layer count is 1
			}
		};

		vkCmdPipelineBarrier(
			commandBuffer.GetVkCommandBuffer(),
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_2_NONE, // Destination stage is color attachment output
			0, // No dependency flags
			0, nullptr, // No memory barriers
			0, nullptr, // No buffer barriers
			1, &imageMemoryBarrier // Single image memory barrier
		);
	}

	std::unique_ptr<Renderer> Renderer::CreateOrNull(CreateInfo& createInfo) noexcept
	{
		pugi::xml_document doc;
		const pugi::xml_parse_result result = doc.load_file(createInfo.RendererFilePath.string().c_str());
		if (!result)
		{
			return nullptr;
		}

		const pugi::xml_node root = doc.child("Renderer");
		if (!root)
		{
			return nullptr;
		}

		std::unique_ptr<Renderer> renderer = std::make_unique<Renderer>(createInfo.Instance);
		renderer->mName = root.attribute("Name").as_string();
		if (renderer->mName.empty())
		{
			CGS_LOG_ERROR("Renderer name is empty in the file: %s", createInfo.RendererFilePath.string().c_str());
			return nullptr;
		}

		const pugi::xml_node pipelinesNode = root.child("Pipelines");
		for (const pugi::xml_node pipelineNode : pipelinesNode.children())
		{
			renderer->mPipelines.emplace(
				std::string(pipelineNode.name()),
				nullptr
			);
		}

		const pugi::xml_node renderNode = root.child("Render");
		for (const pugi::xml_node renderCommandNode : renderNode.children())
		{
			const std::string commandType = renderCommandNode.name();
			if (commandType == "Draw")
			{
				const std::string pipelineName = renderCommandNode.attribute("Pipeline").as_string();
				auto it = renderer->mPipelines.find(pipelineName);
				if (it == renderer->mPipelines.end())
				{
					CGS_LOG_ERROR("Pipeline '%s' not found in renderer: %s", pipelineName.c_str(), renderer->mName.c_str());
					continue;
				}
				if (it->second == nullptr)
				{
					CGS_LOG_ERROR("Pipeline '%s' is not initialized", pipelineName.c_str());
					continue;
				}
				renderer->mRenderCommands.push_back(
					std::make_unique<RenderCommand<eRenderCommand::DRAW>>(createInfo.Instance, *it->second)
				);
			}
			else
			{
				CGS_LOG_ERROR("Unknown render command type: %s in renderer: %s", commandType.c_str(), renderer->mName.c_str());
			}
		}

		return renderer;
	}
	
	Renderer::Renderer(rhi::Instance& instance) noexcept
		: mInstance(instance)
	{
	}
	
	Renderer::~Renderer() noexcept
	{
		mPipelines.clear();
		mRenderCommands.clear();
	}

	void Renderer::Render(rhi::CommandBuffer& commandBuffer) noexcept
	{
		for (auto& renderCommand : mRenderCommands)
		{
			renderCommand->Execute(commandBuffer);
		}
	}
}
