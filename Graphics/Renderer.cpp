#include "Graphics/pch.h"

#include "Graphics/Renderer.h"

#include "Graphics/RHI/BackBuffer.h"
#include "Graphics/RHI/Device.h"
#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/CommandPool.h"
#include "Graphics/RHI/Image.h"
#include "Graphics/RHI/Instance.h"
#include "Graphics/RHI/PhysicalDevice.h"
#include "Graphics/RHI/PhysicalDeviceGroup.h"
#include "Graphics/RHI/Queue.h"
#include "Graphics/RHI/QueueFamily.h"
#include "Graphics/RHI/SwapChain.h"

namespace cgs::graphics
{
	Renderer::Renderer(const CreateInfo& createInfo) noexcept
		: mConfig(std::move(createInfo.Config))
		, mInstance()
		, mWindowHandle(createInfo.WindowHandle) // Store the window handle for the renderer
		, mRendererImplementations() // Initialize the renderer implementations vector
		, mRenderingOrder() // Initialize the rendering order vector
		, mCurrentFrameIndex(0) // Initialize the current frame index to 0
	{
		[[maybe_unused]] const std::filesystem::path& configFilePath = mConfig.GetConfigFilePath();
		CGS_LOG_INFO("Renderer created with configuration from: %s", configFilePath.string().c_str());

		// Retrieve project information from the configuration
		rhi::Instance::CreateInfo instanceCreateInfo =
		{
			.Config = mConfig,
			.ApplicationInfo = createInfo.ApplicationInfo,
			.WindowHandle = mWindowHandle, // Pass the window handle to the instance create info
		};

		mConfig.CreateProjectInfo(instanceCreateInfo.EngineInfo);

		CGS_LOG_INFO("Renderer initialized with project: %s (Version: %u)", 
			instanceCreateInfo.ApplicationInfo.Name.c_str(),
			instanceCreateInfo.ApplicationInfo.Version);
		CGS_LOG_INFO("Creating RHI Instance...");
		// Create the RHI instance with the provided application and engine information
		mInstance = std::make_unique<rhi::Instance>(instanceCreateInfo);
		CGS_LOG_INFO("RHI Instance created successfully.");
		CGS_LOG_INFO("Renderer initialized successfully.");

		const std::string emptyRendererName = "EmptyRenderer";
		EmptyRendererImplementation::CreateInfo emptyRendererCreateInfo =
		{
			.BaseCreateInfo = {.Instance = *mInstance, .Name = emptyRendererName } // Create info for the empty renderer implementation
		};
		AddRenderer<EmptyRendererImplementation>(emptyRendererCreateInfo); // Add a default empty renderer implementation
		mRenderingOrder.push_back(emptyRendererName); // Add the empty renderer to the rendering order
	}

	Renderer::~Renderer() noexcept
	{
		mInstance.reset(); // Automatically cleans up the RHI instance
		CGS_LOG_INFO("Renderer destroyed.");
	}

	void Renderer::Render() noexcept
	{
		rhi::PhysicalDevice& physicalDevice = mInstance->GetMainPhysicalDeviceGroup().GetMainPhysicalDevice();
		rhi::Device& device = physicalDevice.GetLogicalDevice();
		const rhi::QueueFamily& queueFamily = physicalDevice.GetMainQueueFamily();
		const rhi::Queue& queue = queueFamily.GetMainQueue();
		rhi::CommandPool& commandPool = device.GetMainCommandPool();
		rhi::CommandBuffer& commandBuffer = commandPool.GetCommandBuffer(mCurrentFrameIndex);

		{
			rhi::CommandBufferScope commandBufferScope(commandBuffer);

			for (auto& rendererName : mRenderingOrder)
			{
				auto it = mRendererImplementations.find(rendererName);
				if (it != mRendererImplementations.end())
				{
					it->second->Render(commandBuffer); // Call the Render method of each renderer implementation
				}
				else
				{
					CGS_LOG_ERROR("Renderer implementation '%s' not found in the rendering order.", rendererName.c_str());
				}
			}
		}

		queue.Submit(commandBuffer);
		queue.Present(commandBuffer);

		mCurrentFrameIndex = (mCurrentFrameIndex + 1) % device.GetSwapChain().GetBackBufferCount(); // Increment the frame index for the next frame
	}

	void EmptyRendererImplementation::Render(rhi::CommandBuffer& commandBuffer) noexcept
	{
		const rhi::PhysicalDevice& physicalDevice = mInstance.GetMainPhysicalDeviceGroup().GetMainPhysicalDevice();
		const rhi::Device& device = physicalDevice.GetLogicalDevice();
		const rhi::QueueFamily& queueFamily = physicalDevice.GetMainQueueFamily();
		const rhi::BackBuffer& backBuffer = device.GetSwapChain().GetBackBuffer(commandBuffer.GetFrameBufferIndex());

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
			.image = backBuffer.GetColorAttachment().GetVkImage(), // Image to transition
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
			.imageView = backBuffer.GetColorAttachment().GetVkImageView(), // Image view for the color attachment
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // Layout for the color attachment
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // Clear the attachment at the start
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE, // Store the result
			.clearValue = { 0.0f, 0.0f, 0.2f, 1.0f } // Clear value for the color attachment
		};

		const uint32_t width = backBuffer.GetColorAttachment().GetWidth();
		const uint32_t height = backBuffer.GetColorAttachment().GetHeight();

		VkRenderingInfo renderingInfo =
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.pNext = nullptr,
			.flags = 0, // No special flags
			.renderArea = { .offset = { .x = 0, .y = 0 }, .extent = { .width = width, .height = height } }, // Render area based on the back buffer size
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
			.image = backBuffer.GetColorAttachment().GetVkImage(), // Image to transition
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
}
