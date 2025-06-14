#pragma once

#include "volk/volk.h"

#include "Core/pch.h"

namespace cgs::graphics::rhi
{
	class Instance;

	class PhysicalDevice final
	{
	public:
		friend class Instance;

	public:
		struct CreateInfo final
		{
			Instance&				RhiInstance;
			VkPhysicalDevice		PhysicalDevice;
		};

	public:
		PhysicalDevice() = delete;
		explicit PhysicalDevice(const CreateInfo& createInfo) noexcept;
		PhysicalDevice(const PhysicalDevice&) = delete;
		PhysicalDevice(PhysicalDevice&&) noexcept = default;
		~PhysicalDevice() noexcept;

		PhysicalDevice& operator=(const PhysicalDevice&) = delete;
		PhysicalDevice& operator=(PhysicalDevice&&) noexcept = delete;

		float EvaluateScore() const noexcept;

	private:
		Instance&			mInstance;
		VkPhysicalDevice	mPhysicalDevice;
	};
}