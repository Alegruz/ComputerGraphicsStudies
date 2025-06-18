#pragma once

namespace cgs::graphics
{
	namespace rhi
	{
		class Instance;
	}

	class Renderer final
	{
	public:
		struct CreateInfo final
		{
			cgs::core::Config&&		Config; // Configuration for the renderer
			cgs::core::ProjectInfo	ApplicationInfo;
		};

	public:
		Renderer() = delete;
		explicit Renderer(const CreateInfo& createInfo) noexcept;
		~Renderer() noexcept;

	private:
		cgs::core::Config mConfig; // Configuration for the renderer
		std::unique_ptr<rhi::Instance> mInstance;
	};
}
