#pragma once

#include "Graphics/Renderer.h"

namespace cgs::graphics
{
	namespace rhi
	{
		class Attachment;
		class CommandBuffer;
		class Instance;
	}

	class Renderable;
	class RenderGraph;

	class RendererManager final
	{
	public:
		struct CreateInfo final
		{
			const cgs::core::Config& EngineConfig; // Reference to the engine configuration
			cgs::core::Config&&		RendererConfig; // Configuration for the renderer
			cgs::core::ProjectInfo	ApplicationInfo;
			void*					ProcessHandle = nullptr; // Handle to the process
			void*					WindowHandle = nullptr; // Handle to the window for the renderer
		};

	public:
		RendererManager() = delete;
		explicit RendererManager(const CreateInfo& createInfo) noexcept;
		~RendererManager() noexcept;

		void Render() noexcept;

	private:
		void loadAttachments() noexcept;
		void loadRenderables() noexcept;
		void loadRenderers() noexcept;
		void loadRenderGraph() noexcept;
		void loadVertexLayouts() noexcept;

	private:
		const cgs::core::Config& mEngineConfig; // Reference to the engine configuration
		cgs::core::Config mConfig; // Configuration for the renderer
		std::unique_ptr<rhi::Instance> mInstance;
		void* mProcessHandle; // Handle to the process
		void* mWindowHandle; // Handle to the window for the renderer

		std::unordered_map<std::string, std::unique_ptr<Renderer>> mRenderers;
		std::unique_ptr<RenderGraph> mRenderGraph; // Render graph for managing rendering operations
		uint32_t mCurrentFrameIndex; // Current frame index for rendering

		std::filesystem::path mRenderPipelinesPath; // Path to the render pipelines directory
		std::unordered_map<std::string, std::shared_ptr<rhi::Attachment>> mAttachments; // Map of attachments by name
		VertexLayoutsMap mVertexLayouts; // Map of vertex inputs by name
		std::unordered_map<std::string, std::unique_ptr<Renderable>> mRenderables; // Map of renderables by name
	};
}
