#pragma once

namespace cgs::graphics
{
	namespace rhi
	{
		class CommandBuffer;
		class Instance;
	}

	class IRendererImplementation
	{
	public:
		struct CreateInfo final
		{
			rhi::Instance& Instance; // Reference to the RHI instance
			std::string Name; // Name of the renderer implementation
		};

	public:
		explicit IRendererImplementation(const CreateInfo& createInfo) noexcept;
		virtual ~IRendererImplementation() noexcept = default;
		virtual void Render(rhi::CommandBuffer&) noexcept = 0; // Render method to be implemented by derived classes

		CGS_INLINE const std::string& GetName() const noexcept { return mName; } // Get the name of the renderer implementation

	protected:
		rhi::Instance& mInstance; // Reference to the RHI instance
		std::string mName; // Name of the renderer implementation
	};

	class EmptyRendererImplementation final : public IRendererImplementation
	{
	public:
		struct CreateInfo final
		{
			IRendererImplementation::CreateInfo BaseCreateInfo; // Base create info for the renderer implementation
		};

	public:
		explicit EmptyRendererImplementation(const CreateInfo& createInfo) noexcept;
		~EmptyRendererImplementation() noexcept = default;
		void Render(rhi::CommandBuffer&) noexcept override;
	};

	template<typename T>
	concept RendererImplementationType = std::derived_from<T, IRendererImplementation>;

	class Renderer final
	{
	public:
		struct CreateInfo final
		{
			cgs::core::Config&&		Config; // Configuration for the renderer
			cgs::core::ProjectInfo	ApplicationInfo;
			void*					WindowHandle = nullptr; // Handle to the window for the renderer
		};

	public:
		Renderer() = delete;
		explicit Renderer(const CreateInfo& createInfo) noexcept;
		~Renderer() noexcept;

		template<RendererImplementationType T>
		void AddRenderer(T::CreateInfo& createInfo) noexcept;

		void Render() noexcept;

	private:
		cgs::core::Config mConfig; // Configuration for the renderer
		std::unique_ptr<rhi::Instance> mInstance;
		void* mWindowHandle = nullptr; // Handle to the window for the renderer

		std::unordered_map<std::string, std::unique_ptr<IRendererImplementation>> mRendererImplementations; // Renderer implementations
		std::vector<std::string> mRenderingOrder;
		uint32_t mCurrentFrameIndex; // Current frame index for rendering
	};

	CGS_INLINE IRendererImplementation::IRendererImplementation(const CreateInfo& createInfo) noexcept
		: mInstance(createInfo.Instance)
		, mName(createInfo.Name)
	{
	}

	CGS_INLINE EmptyRendererImplementation::EmptyRendererImplementation(const CreateInfo& createInfo) noexcept
		: IRendererImplementation(createInfo.BaseCreateInfo)
	{
	}

	template<RendererImplementationType T>
	void Renderer::AddRenderer(T::CreateInfo& createInfo) noexcept
	{
		static_assert(std::is_base_of_v<IRendererImplementation, T>, "Renderer implementation must derive from IRendererImplementation.");

		auto rendererImpl = std::make_unique<T>(createInfo);
		mRendererImplementations.emplace(rendererImpl->GetName(), std::move(rendererImpl));
	}
}
