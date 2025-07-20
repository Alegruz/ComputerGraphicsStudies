#include "Graphics/pch.h"

#include "Core/Window.h"

#include "Graphics/RHI/Device.h"

#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/CommandPool.h"
#include "Graphics/RHI/Fence.h"
#include "Graphics/RHI/Image.h"
#include "Graphics/RHI/Instance.h"
#include "Graphics/RHI/PhysicalDevice.h"
#include "Graphics/RHI/QueueFamily.h"
#include "Graphics/RHI/Semaphore.h"
#include "Graphics/RHI/SwapChain.h"

namespace cgs::graphics::rhi
{
    Device::Device(const CreateInfo& createInfo) noexcept
        : mPhysicalDevice(createInfo.RhiPhysicalDevice)
        , mDevice(createInfo.Device)
    {
        assert(mDevice != VK_NULL_HANDLE);
        createSwapChain(); // Create the swap chain if needed
        createCommandPools(); // Create command pools for the device
    }

    Device::~Device() noexcept
    {
        // Destroy any command pools that were created by this device before
        // tearing down the logical device. This ensures the pools are released
        // with a valid device handle.
        for (auto& commandPool : mCommandPools)
        {
            commandPool.reset(); // Reset the command pool, which will destroy its command buffers
        }

        mCommandPools.clear();
        mSwapChain.reset(); // Reset the swap chain, which will destroy its resources
        mPhysicalDevice.DestroyLogicalDevice(mDevice);
    }

    void Device::createCommandPools() noexcept
    {
        VkResult vr = VK_SUCCESS;

        for (const auto& queueFamily : mPhysicalDevice.GetQueueFamilies())
        {
            CommandPool::CreateInfo commandPoolCreateInfo =
            {
                .RhiDevice = *this,
                .CommandPool = VK_NULL_HANDLE // Will be created later
            };

            VkCommandPoolCreateInfo commandPoolInfo =
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // Allows command buffers to be reset
                .queueFamilyIndex = queueFamily->GetIndex() // Use the index of the queue family
            };
            vr = vkCreateCommandPool(mDevice, &commandPoolInfo, nullptr, &commandPoolCreateInfo.CommandPool);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to create command pool for queue family %u: %s", queueFamily->GetIndex(), VkResultToString(vr));
                continue; // Skip this queue family if command pool creation fails
            }
            CGS_LOG_INFO("Created command pool for queue family %u: %p", queueFamily->GetIndex(), commandPoolCreateInfo.CommandPool);
            // Create the command pool and add it to the list

            auto commandPool = std::make_unique<CommandPool>(commandPoolCreateInfo);
            mCommandPools.push_back(std::move(commandPool));
        }
    }

    void Device::createSwapChain() noexcept
    {
        VkResult vr = VK_SUCCESS;

        const Instance& instance = mPhysicalDevice.GetInstance();

        uint32_t width = 0;
        uint32_t height = 0;
        bool result = instance.GetConfig().GetSetting<uint32_t>(CONFIG_WIDTH, width);
        if (!result || width == 0)
        {
            CGS_LOG_WARNING("Invalid window width: %u. Using default value of 800.", width);
            width = 1920; // Default to 1920 if not set or invalid
        }

        result = instance.GetConfig().GetSetting<uint32_t>(CONFIG_HEIGHT, height);
        if (!result || height == 0)
        {
            CGS_LOG_WARNING("Invalid window height: %u. Using default value of 600.", height);
            height = 1080; // Default to 1080 if not set or invalid
        }

        SwapChain::CreateInfo swapChainCreateInfo =
        {
            .RhiDevice = *this, // Reference to the device this swap chain is created from
            .SwapChain = VK_NULL_HANDLE, // Will be created later
            .Extent = { .width = width, .height = height }, // Default extent for the swap chain, can be adjusted later
        };

        void* processHandle = instance.GetProcessHandle();
        if (processHandle == nullptr)
        {
            CGS_LOG_ERROR("Process handle is null, cannot create surface.");
            return;
        }
        void* windowHandle = instance.GetWindowHandle();
		if (windowHandle == nullptr)
		{
			CGS_LOG_ERROR("Window handle is null, cannot create surface.");
			return;
		}

#if defined(CGS_WIN32)
		const VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.hinstance = reinterpret_cast<HINSTANCE>(processHandle), // Use the current module handle
			.hwnd = reinterpret_cast<HWND>(windowHandle), // Cast the window handle to HWND
		};

		vr = vkCreateWin32SurfaceKHR(instance.GetVkInstance(), &win32SurfaceCreateInfo, nullptr, &swapChainCreateInfo.Surface);
		if (vr != VK_SUCCESS)
		{
			CGS_LOG_ERROR("Failed to create Vulkan surface: %d", vr);
			return;
		}
#elif defined(CGS_UNIX)
        const VkWaylandSurfaceCreateInfoKHR waylandSurfaceCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .display = static_cast<wl_display*>(processHandle), // Cast the window handle to wl_display
            .surface = static_cast<wl_surface*>(windowHandle)   // Cast the process handle to wl_surface
        };
        vr = vkCreateWaylandSurfaceKHR(instance.GetVkInstance(), &waylandSurfaceCreateInfo, nullptr, &swapChainCreateInfo.Surface);
        if (vr != VK_SUCCESS)
        {
            CGS_LOG_ERROR("Failed to create Vulkan Wayland surface: %d", vr);
            return;
        }
#endif	// defined(CGS_WIN32)

        uint32_t frameBufferCount = 0;
        result = instance.GetConfig().GetSetting(CONFIG_FRAME_BUFFER_COUNT, frameBufferCount);
        if (!result || frameBufferCount == 0)
        {
            CGS_LOG_WARNING("Invalid frame buffer count: %u. Using default value of 2.", frameBufferCount);
            frameBufferCount = 2; // Default to 2 if not set or invalid
        }

        VkSwapchainCreateInfoKHR vkSwapChainCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0, // No special flags
            .surface = swapChainCreateInfo.Surface,
            .minImageCount = frameBufferCount,
            .imageFormat = VK_FORMAT_B8G8R8A8_UNORM, // Standard RGBA format
            .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, // Standard sRGB color space
            .imageExtent = VkExtent2D
            {
                .width = width, // Width of the swap chain images
                .height = height // Height of the swap chain images
            },
            .imageArrayLayers = 1, // Single layer for 2D images
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, // Usage flags
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE, // Exclusive sharing mode
            .queueFamilyIndexCount = 0, // Not sharing with other queue families
            .pQueueFamilyIndices = nullptr,
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, // No transformation
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, // Opaque composite alpha
            .presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
            .clipped = VK_TRUE, // Clipping enabled
            .oldSwapchain = VK_NULL_HANDLE // No old swapchain
        };

        vr = vkCreateSwapchainKHR(mDevice, &vkSwapChainCreateInfo, nullptr, &swapChainCreateInfo.SwapChain);
        if (vr != VK_SUCCESS)
        {
            CGS_LOG_ERROR("Failed to create swap chain: %s", VkResultToString(vr));
            return; // Exit if swap chain creation fails
        }

        mSwapChain = std::make_unique<SwapChain>(swapChainCreateInfo);
    }
}
