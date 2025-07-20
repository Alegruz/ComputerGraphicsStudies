#pragma once

namespace cgs::graphics
{
	namespace rhi
	{
		class CommandBuffer;
		class Instance;
		class PipelineBase;
	}

	enum class eRenderCommand : uint8_t
	{
		DRAW,
		COUNT,
	};

	class IRenderCommand
	{
	public:
		CGS_INLINE explicit IRenderCommand(const rhi::Instance& instance, const eRenderCommand type) noexcept
			: mInstance(instance) // Initialize the RHI instance reference
			, mType(type) // Initialize the render command type
		{
		}
		CGS_INLINE virtual ~IRenderCommand() noexcept = default; // Virtual destructor for proper cleanup

		CGS_INLINE virtual void Execute(rhi::CommandBuffer& commandBuffer) noexcept = 0; // Pure virtual function to execute the render command

		CGS_INLINE eRenderCommand GetType() const noexcept { return mType; } // Get the type of the render command

	protected:
		const rhi::Instance& mInstance; // Reference to the RHI instance for accessing resources
		eRenderCommand mType; // Type of the render command
	};

	template<eRenderCommand RENDER_COMMAND>
	class RenderCommand final : public IRenderCommand
	{
	public:
		CGS_INLINE explicit RenderCommand(const rhi::Instance& instance) noexcept
			: IRenderCommand(instance, RENDER_COMMAND) // Initialize the base class with the render command type
		{
			static_assert(RENDER_COMMAND != RENDER_COMMAND, "RenderCommand must be specialized for a specific eRenderCommand type.");
		}
		CGS_INLINE ~RenderCommand() noexcept override = default; // Override the destructor for proper cleanup
		void Execute([[maybe_unused]] rhi::CommandBuffer& commandBuffer) noexcept override {}
	};

	// Specialization for DRAW command
	template<>
	class RenderCommand<eRenderCommand::DRAW> final : public IRenderCommand
	{
	public:
		CGS_INLINE explicit RenderCommand(const rhi::Instance& instance, rhi::PipelineBase& pipeline) noexcept
			: IRenderCommand(instance, eRenderCommand::DRAW) // Initialize the base class with DRAW type
			, mPipeline(pipeline) // Initialize the pipeline reference
		{
		}
		CGS_INLINE ~RenderCommand() noexcept override = default; // Override the destructor for proper cleanup
		void Execute(rhi::CommandBuffer& commandBuffer) noexcept override;
		CGS_INLINE rhi::PipelineBase& GetPipeline() noexcept { return mPipeline; } // Get the pipeline associated with this render command

	private:
		rhi::PipelineBase& mPipeline;
	};

	class Renderer final
	{
	public:
		struct CreateInfo final
		{
			rhi::Instance& Instance; // Reference to the RHI instance
			std::filesystem::path RendererFilePath;
		};
	
	public:
		static std::unique_ptr<Renderer> CreateOrNull(CreateInfo& createInfo) noexcept;

	public:
		explicit Renderer(rhi::Instance& mInstance) noexcept;
		~Renderer() noexcept;

		CGS_INLINE const std::string& GetName() const noexcept { return mName; } // Get the name of the renderer implementation

		void Render(rhi::CommandBuffer& commandBuffer) noexcept;

	protected:
		rhi::Instance& mInstance; // Reference to the RHI instance
		std::string mName; // Name of the renderer implementation
		std::unordered_map<std::string, std::unique_ptr<rhi::PipelineBase>> mPipelines; // Map of pipelines by name
		std::vector<std::unique_ptr<IRenderCommand>> mRenderCommands; // List of render commands
	};
}
