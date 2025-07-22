#include "Graphics/pch.h"

#include "Graphics/RendererManager.h"

#include "Graphics/Renderable.h"
#include "Graphics/RenderGraph.h"
#include "Graphics/RHI/Attachment.h"
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
#include "Graphics/RHI/VertexInput.h"

namespace cgs::graphics
{
	RendererManager::RendererManager(const CreateInfo& createInfo) noexcept
		: mEngineConfig(createInfo.EngineConfig) // Store the engine configuration reference
		, mConfig(std::move(createInfo.RendererConfig))
		, mInstance()
		, mProcessHandle(createInfo.ProcessHandle) // Store the process handle for the renderer
		, mWindowHandle(createInfo.WindowHandle) // Store the window handle for the renderer
		, mRenderers() // Initialize the renderer implementations vector
		, mRenderGraph() // Initialize the rendering order vector
		, mCurrentFrameIndex(0) // Initialize the current frame index to 0
		, mRenderPipelinesPath() // Initialize the render pipelines path
		, mAttachments() // Initialize the attachments map
		, mVertexLayouts() // Initialize the vertex inputs map
		, mRenderables() // Initialize the renderables map
	{
		[[maybe_unused]] const std::filesystem::path& configFilePath = mConfig.GetConfigFilePath();
		CGS_LOG_INFO("RendererManager created with configuration from: %s", configFilePath.string().c_str());

		// Retrieve project information from the configuration
		rhi::Instance::CreateInfo instanceCreateInfo =
		{
			.Config = mConfig,
			.ApplicationInfo = createInfo.ApplicationInfo,
			.ProcessHandle = mProcessHandle, // Pass the process handle to the instance create info
			.WindowHandle = mWindowHandle, // Pass the window handle to the instance create info
		};

		mConfig.CreateProjectInfo(instanceCreateInfo.EngineInfo);

		CGS_LOG_INFO("RendererManager initialized with project: %s (Version: %u)", 
			instanceCreateInfo.ApplicationInfo.Name.c_str(),
			instanceCreateInfo.ApplicationInfo.Version);
		CGS_LOG_INFO("Creating RHI Instance...");
		// Create the RHI instance with the provided application and engine information
		mInstance = std::make_unique<rhi::Instance>(instanceCreateInfo);
		CGS_LOG_INFO("RHI Instance created successfully.");
		CGS_LOG_INFO("RendererManager initialized successfully.");

		std::string renderPipelinesPathString;
		bool bResult = mConfig.GetSetting(CONFIG_RENDER_PIPELINES_PATH, renderPipelinesPathString);
		if (!bResult || renderPipelinesPathString.empty())
		{
			CGS_LOG_ERROR("Render pipelines path not found in configuration. Using default path.");
			renderPipelinesPathString = "Assets/RenderPipelines"; // Default path if not specified
		}
		mRenderPipelinesPath = std::filesystem::path(renderPipelinesPathString);

		loadAttachments(); // Load attachments from the specified path
		loadVertexLayouts(); // Load vertex inputs from the specified path
		loadRenderables(); // Load renderables from the specified path
		loadRenderers(); // Load all renderer implementations from the specified path
		loadRenderGraph(); // Load the render graph from the specified file
	}

	RendererManager::~RendererManager() noexcept
	{
		mRenderGraph.reset(); // Automatically cleans up the render graph
		mRenderers.clear();
		mAttachments.clear();
		mVertexLayouts.clear();
		mInstance.reset(); // Automatically cleans up the RHI instance
		CGS_LOG_INFO("RendererManager destroyed.");
	}

	void RendererManager::Render() noexcept
	{
		rhi::PhysicalDevice& physicalDevice = mInstance->GetMainPhysicalDeviceGroup().GetMainPhysicalDevice();
		rhi::Device& device = physicalDevice.GetLogicalDevice();
		const rhi::QueueFamily& queueFamily = physicalDevice.GetMainQueueFamily();
		const rhi::Queue& queue = queueFamily.GetMainQueue();
		rhi::CommandPool& commandPool = device.GetMainCommandPool();
		rhi::CommandBuffer& commandBuffer = commandPool.GetCommandBuffer(mCurrentFrameIndex);

		{
			rhi::CommandBufferScope commandBufferScope(commandBuffer);
			
			const std::shared_ptr<RenderGraphNode>& headNode = mRenderGraph->GetHeadNode();
			std::queue<std::shared_ptr<RenderGraphNode>> nodeQueue;
			if (headNode) 
			{
				nodeQueue.push(headNode);
			}

			while (!nodeQueue.empty())
			{
				auto currentNode = nodeQueue.front();
				nodeQueue.pop();

				if (currentNode->GetType() == RenderGraphNode::Type::RENDER)
				{
					const std::string& rendererName = currentNode->GetRendererName();
					auto it = mRenderers.find(rendererName);
					if (it != mRenderers.end())
					{
						it->second->Render(commandBuffer); // Call the Render method of each renderer implementation
					}
					else
					{
						CGS_LOG_ERROR("RendererManager implementation '%s' not found in the rendering order.", rendererName.c_str());
					}
				}

				for (const auto& child : currentNode->GetChildren())
				{
					if (child) nodeQueue.push(child);
				}
			}
		}

		queue.Submit(commandBuffer);
		queue.Present(commandBuffer);

		mCurrentFrameIndex = (mCurrentFrameIndex + 1) % device.GetSwapChain().GetBackBufferCount(); // Increment the frame index for the next frame
	}

	void RendererManager::loadAttachments() noexcept
	{
		rhi::SwapChain& swapChain = mInstance->GetMainPhysicalDeviceGroup().GetMainPhysicalDevice().GetLogicalDevice().GetSwapChain();

		const std::filesystem::path attachmentsPath = mRenderPipelinesPath / "Attachments.xml";
		if (!std::filesystem::exists(attachmentsPath))
		{
			CGS_LOG_ERROR("Attachments file '%s' does not exist. Please check the configuration.", attachmentsPath.string().c_str());
			return; // Exit if the attachments file does not exist
		}

		pugi::xml_document doc;
		const pugi::xml_parse_result result = doc.load_file(attachmentsPath.string().c_str());
		if (!result)
		{
			CGS_LOG_ERROR("Failed to load attachments file '%s': %s", attachmentsPath.string().c_str(), result.description());
			return; // Exit if the attachments file could not be loaded
		}

		const pugi::xml_node root = doc.child("Attachments");
		if (!root)
		{
			CGS_LOG_ERROR("Attachments file '%s' is missing the root 'Attachments' node.", attachmentsPath.string().c_str());
			return; // Exit if the root node is missing
		}

		for (const pugi::xml_node& attachmentNode : root.children("Attachment"))
		{
			const std::string name = attachmentNode.attribute("Name").as_string();
			if (name.empty())
			{
				CGS_LOG_ERROR("Attachment name is empty in the file: %s", attachmentsPath.string().c_str());
				continue; // Skip if the attachment name is empty
			}

			auto attachmentPtr = std::make_shared<rhi::Attachment>(rhi::Attachment::CreateInfo{ .RhiInstance = *mInstance, .Name = name, .Node = attachmentNode });
			if (attachmentPtr)
			{
				if (attachmentPtr->IsBackBuffer() == true)
				{
					swapChain.SetBackBufferAttachment(attachmentPtr); // Set the back buffer attachment in the swap chain if it is a back buffer
				}
				mAttachments[name] = std::move(attachmentPtr); // Store the attachment in the map
			}
			else
			{
				CGS_LOG_ERROR("Failed to create attachment '%s' from node in file: %s", name.c_str(), attachmentsPath.string().c_str());
			}
		}
	}

	void RendererManager::loadRenderables() noexcept
	{
		std::string renderablesPathString;
		bool bResult = mEngineConfig.GetSetting(CONFIG_RENDERABLES_PATH, renderablesPathString); // Retrieve the path to the renderers from the configuration
		if (!bResult || renderablesPathString.empty())
		{
			CGS_LOG_ERROR("Renderers path not found in configuration. Using default path.");
			renderablesPathString = "Assets/Renderables"; // Default path if not specified
		}

		std::filesystem::path renderablesPath(renderablesPathString);
		if (!std::filesystem::exists(renderablesPath))
		{
			CGS_LOG_ERROR("Renderables path '%s' does not exist. Please check the configuration.", renderablesPath.string().c_str());
			return; // Exit if the renderers path does not exist
		}

		std::vector<std::filesystem::path> modelPaths = Renderable::GetModelPaths(renderablesPath); // Get all model paths from the specified directory
		
		std::string renderableNameString;
		bResult = mConfig.GetSetting(CONFIG_SELECTED_RENDERABLE, renderableNameString); // Retrieve the selected renderable name from the configuration
		if (!bResult || renderableNameString.empty())
		{
			CGS_LOG_ERROR("Selected renderable name not found in configuration. Using first model as default.");
			if (!modelPaths.empty())
			{
				renderableNameString = modelPaths.front().filename().string(); // Use the first model as default if not specified
			}
			else
			{
				CGS_LOG_ERROR("No models found in the specified renderables path: %s", renderablesPath.string().c_str());
				return; // Exit if no models are found
			}
		}

		for (const auto& modelPath : modelPaths)
		{
			if (modelPath.filename().string().find(renderableNameString) != std::string::npos) // Check if the model path contains the selected renderable name
			{
				Renderable::CreateInfo createInfo = 
				{ 
					.ModelPath = modelPath,
					.VertexLayouts = mVertexLayouts, // Pass the vertex layouts map to the renderable
				};
				std::unique_ptr<Renderable> renderable = Renderable::CreateOrNull(createInfo);
				if (renderable)
				{
					const std::string& renderableName = renderable->GetName();
					mRenderables[renderableName] = std::move(renderable); // Store the renderable in the map
					CGS_LOG_INFO("Renderable '%s' loaded successfully from path: %s", renderableName.c_str(), modelPath.string().c_str());
				}
				else
				{
					CGS_LOG_ERROR("Failed to create renderable from model path: %s", modelPath.string().c_str());
				}
			}
		}
	}

	void RendererManager::loadRenderers() noexcept
	{
		std::string renderersPathString;
		bool bResult = mConfig.GetSetting(CONFIG_RENDERERS_PATH, renderersPathString); // Retrieve the path to the renderers from the configuration
		if (!bResult || renderersPathString.empty())
		{
			CGS_LOG_ERROR("Renderers path not found in configuration. Using default path.");
			renderersPathString = "Assets/Renderers"; // Default path if not specified
		}

		std::filesystem::path renderersPath(renderersPathString);
		if (!std::filesystem::exists(renderersPath))
		{
			CGS_LOG_ERROR("Renderers path '%s' does not exist. Please check the configuration.", renderersPath.string().c_str());
			return; // Exit if the renderers path does not exist
		}

		for (const auto& entry : std::filesystem::directory_iterator(renderersPath))
		{
			if (entry.is_regular_file())
			{
				Renderer::CreateInfo createInfo =
				{
					.Instance = *mInstance,
					.RendererFilePath = entry.path()
				};
				std::unique_ptr<Renderer> renderer = Renderer::CreateOrNull(createInfo);
				if (renderer)
				{
					const std::string& rendererName = renderer->GetName();
					mRenderers[rendererName] = std::move(renderer); // Store the renderer implementation in the map
				}
			}
		}
	}

	void RendererManager::loadRenderGraph() noexcept
	{
		std::string renderGraphFilePathString;
		const bool bResult = mConfig.GetSetting(CONFIG_RENDER_GRAPH_FILE_PATH, renderGraphFilePathString); // Retrieve the render graph file path from the configuration
		if (!bResult || renderGraphFilePathString.empty())
		{
			CGS_LOG_ERROR("Render graph file path not found in configuration. Using default path.");
			renderGraphFilePathString = "Assets/Renderers/RenderGraph.xml"; // Default path if not specified
		}

		std::filesystem::path renderGraphFilePath(renderGraphFilePathString);
		if (!std::filesystem::exists(renderGraphFilePath))
		{
			CGS_LOG_ERROR("Render graph file '%s' does not exist. Please check the configuration.", renderGraphFilePath.string().c_str());
			return; // Exit if the render graph file does not exist
		}
		mRenderGraph = std::make_unique<RenderGraph>(RenderGraph::CreateInfo{ .RenderGraphFilePath = renderGraphFilePath });
	}

	void RendererManager::loadVertexLayouts() noexcept
	{
		const std::filesystem::path vertexLayoutsPath = mRenderPipelinesPath / "VertexLayouts.xml";
		if (!std::filesystem::exists(vertexLayoutsPath))
		{
			CGS_LOG_ERROR("Vertex inputs file '%s' does not exist. Please check the configuration.", vertexLayoutsPath.string().c_str());
			return; // Exit if the vertex inputs file does not exist
		}

		pugi::xml_document doc;
		const pugi::xml_parse_result result = doc.load_file(vertexLayoutsPath.string().c_str());
		if (!result)
		{
			CGS_LOG_ERROR("Failed to load vertex inputs file '%s': %s", vertexLayoutsPath.string().c_str(), result.description());
			return; // Exit if the vertex inputs file could not be loaded
		}

		const pugi::xml_node root = doc.child("VertexLayouts");
		if (!root)
		{
			CGS_LOG_ERROR("Vertex inputs file '%s' is missing the root 'VertexLayouts' node.", vertexLayoutsPath.string().c_str());
			return; // Exit if the root node is missing
		}

		for(const pugi::xml_node& vertexLayout : root.children("VertexLayout"))
		{
			const std::string name = vertexLayout.attribute("Name").as_string();
			if (name.empty())
			{
				CGS_LOG_ERROR("Vertex input name is empty in the file: %s", vertexLayoutsPath.string().c_str());
				continue; // Skip if the vertex input name is empty
			}

			auto vertexLayoutPtr = std::make_unique<rhi::VertexLayout>(rhi::VertexLayout::CreateInfo{ .Name = name, .Elements = vertexLayout });
			mVertexLayouts[name] = std::move(vertexLayoutPtr); // Store the vertex input in the map
		}
	}
}
