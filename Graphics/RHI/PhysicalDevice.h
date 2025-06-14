#pragma once

#include "volk/volk.h"

#include "Core/pch.h"

namespace cgs::graphics::rhi
{
	class Device;
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
		void PrintProperties() const noexcept;

	private:
		static void printDeviceProperties(const VkPhysicalDeviceProperties2 &properties) noexcept;

	private:
		Instance&				mInstance;
		VkPhysicalDevice		mPhysicalDevice;
		std::unique_ptr<Device>	mLogicalDevice; // Logical device created from this physical device, if any
	};
}