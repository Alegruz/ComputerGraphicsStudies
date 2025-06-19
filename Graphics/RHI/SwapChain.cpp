#include "Graphics/pch.h"

#include "Graphics/RHI/SwapChain.h"

#include "Graphics/RHI/BackBuffer.h"
#include "Graphics/RHI/Device.h"
#include "Graphics/RHI/Image.h"
#include "Graphics/RHI/Instance.h"
#include "Graphics/RHI/PhysicalDevice.h"
#include "Graphics/RHI/Semaphore.h"

namespace cgs::graphics::rhi
{
    SwapChain::SwapChain(const CreateInfo& createInfo) noexcept
        : mDevice(createInfo.RhiDevice)
        , mSwapChain(createInfo.SwapChain)
        , mSurface(createInfo.Surface)
    {
        assert(mSwapChain != VK_NULL_HANDLE);

        uint32_t imageCount = 0;
        VkResult vr = vkGetSwapchainImagesKHR(mDevice.GetVkDevice(), mSwapChain, &imageCount, nullptr);
        if (vr != VK_SUCCESS)
        {
            CGS_LOG_ERROR("Failed to get swap chain image count: %s", VkResultToString(vr));
            return; // Return early on failure
        }

        std::vector<VkImage> swapChainImages(imageCount);
        vr = vkGetSwapchainImagesKHR(mDevice.GetVkDevice(), mSwapChain, &imageCount, swapChainImages.data());
        if (vr != VK_SUCCESS)
        {
            CGS_LOG_ERROR("Failed to get swap chain images: %s", VkResultToString(vr));
            return;
        }

        mBackBuffers.reserve(swapChainImages.size());
        for (const auto& image : swapChainImages)
        {
            Image::CreateInfo imageCreateInfo =
            {
                .ResourceCreateInfo =
                {
                    .RhiDevice = mDevice,
                },
                .Image = image,
            };

            VkImageViewCreateInfo imageViewCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0, // No special flags
                .image = imageCreateInfo.Image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D, // 2D image view
                .format = VK_FORMAT_B8G8R8A8_SRGB, // Assuming SRGB format for color attachment
                .components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } // Color aspect
            };

            vr = vkCreateImageView(mDevice.GetVkDevice(), &imageViewCreateInfo, nullptr, &imageCreateInfo.ImageView);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to create image view: %s", VkResultToString(vr));
                continue; // Skip this image if view creation fails
            }

            Image::CreateInfo depthImageCreateInfo =
            {
                .ResourceCreateInfo =
                {
                    .RhiDevice = mDevice,
                },
            };

            VkImageCreateInfo depthStencilImageCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0, // No special flags
                .imageType = VK_IMAGE_TYPE_2D, // 2D image
                .format = VK_FORMAT_D24_UNORM_S8_UINT, // Depth format
                .extent = { .width = createInfo.Extent.width, .height = createInfo.Extent.height, .depth = 1, }, // Example extent, should match swap chain size
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT, // No multisampling
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, // Depth attachment and transfer source
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED // Initial layout
            };
            vr = vkCreateImage(mDevice.GetVkDevice(), &depthStencilImageCreateInfo, nullptr, &depthImageCreateInfo.Image);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to create depth image: %s", VkResultToString(vr));
                continue; // Skip this image if creation fails
            }
            
            VkMemoryRequirements depthImageMemoryRequirements = {};
            vkGetImageMemoryRequirements(mDevice.GetVkDevice(), depthImageCreateInfo.Image, &depthImageMemoryRequirements);

            VkMemoryAllocateInfo depthImageMemoryAllocateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = nullptr,
                .allocationSize = depthImageMemoryRequirements.size,
                .memoryTypeIndex = mDevice.GetPhysicalDevice().GetMemoryTypeIndex(
                    depthImageMemoryRequirements.memoryTypeBits,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT // Device local memory for optimal performance
                ),
            };

            vr = vkAllocateMemory(mDevice.GetVkDevice(), &depthImageMemoryAllocateInfo, nullptr, &depthImageCreateInfo.ResourceCreateInfo.DeviceMemory);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to allocate memory for depth image: %s", VkResultToString(vr));
                vkDestroyImage(mDevice.GetVkDevice(), depthImageCreateInfo.Image, nullptr);
                continue; // Skip this image if memory allocation fails
            }

            vr = vkBindImageMemory(mDevice.GetVkDevice(), depthImageCreateInfo.Image, depthImageCreateInfo.ResourceCreateInfo.DeviceMemory, 0);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to bind memory to depth image: %s", VkResultToString(vr));
                vkFreeMemory(mDevice.GetVkDevice(), depthImageCreateInfo.ResourceCreateInfo.DeviceMemory, nullptr);
                vkDestroyImage(mDevice.GetVkDevice(), depthImageCreateInfo.Image, nullptr);
                continue; // Skip this image if binding fails
            }

            VkImageViewCreateInfo depthImageViewCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0, // No special flags
                .image = depthImageCreateInfo.Image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D, // 2D image view
                .format = depthStencilImageCreateInfo.format, // Depth format
                .components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
                .subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 } // Depth and stencil aspects
            };
            vr = vkCreateImageView(mDevice.GetVkDevice(), &depthImageViewCreateInfo, nullptr, &depthImageCreateInfo.ImageView);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to create depth image view: %s", VkResultToString(vr));
                vkFreeMemory(mDevice.GetVkDevice(), depthImageCreateInfo.ResourceCreateInfo.DeviceMemory, nullptr);
                vkDestroyImage(mDevice.GetVkDevice(), depthImageCreateInfo.Image, nullptr);
                continue; // Skip this image if view creation fails
            }

            Semaphore::CreateInfo presentSemaphoreCreateInfo =
            {
                .RhiDevice = mDevice,
            };

            Semaphore::CreateInfo renderSemaphoreCreateInfo =
            {
                .RhiDevice = mDevice,
            };

            VkSemaphoreCreateInfo semaphoreCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0 // No special flags
            };

            vr = vkCreateSemaphore(mDevice.GetVkDevice(), &semaphoreCreateInfo, nullptr, &presentSemaphoreCreateInfo.Semaphore);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to create semaphore: %s", VkResultToString(vr));
                continue; // Skip this image if semaphore creation fails
            }

            vr = vkCreateSemaphore(mDevice.GetVkDevice(), &semaphoreCreateInfo, nullptr, &renderSemaphoreCreateInfo.Semaphore);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to create semaphore: %s", VkResultToString(vr));
                continue; // Skip this image if semaphore creation fails
            }

            BackBuffer::CreateInfo backBufferCreateInfo =
            {
                .ColorAttachment = std::make_unique<Image>(imageCreateInfo), // Create an Image for each back buffer
                .DepthAttachment = std::make_unique<Image>(depthImageCreateInfo), // Create a depth Image for each back buffer
                .PresentCompletionSemaphore = std::make_unique<Semaphore>(presentSemaphoreCreateInfo), // Create a semaphore for present completion
                .RenderCompletionSemaphore = std::make_unique<Semaphore>(renderSemaphoreCreateInfo), // Create a semaphore for render completion
            };
            mBackBuffers.emplace_back(std::make_unique<BackBuffer>(backBufferCreateInfo)); // Create a BackBuffer for each image
        }
    }

    SwapChain::~SwapChain() noexcept
    {
        if (mSwapChain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(mDevice.GetVkDevice(), mSwapChain, nullptr);
            mSwapChain = VK_NULL_HANDLE;
        }
        
        const Instance& instance = mDevice.GetPhysicalDevice().GetInstance();
		if(mSurface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(instance.GetVkInstance(), mSurface, nullptr);
			mSurface = VK_NULL_HANDLE;
		}
		else
		{
			CGS_LOG_WARNING("Attempted to destroy a null surface.");
		}
    }

    uint32_t SwapChain::AcquireNextImage() const noexcept
    {
        assert(mSwapChain != VK_NULL_HANDLE);

        uint32_t imageIndex = 0;
        const VkAcquireNextImageInfoKHR acquireInfo =
        {
            .sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
            .pNext = nullptr,
            .swapchain = mSwapChain,
            .timeout = UINT64_MAX, // Wait indefinitely for the next image
            .semaphore = VK_NULL_HANDLE, // No semaphore is used here
            .fence = VK_NULL_HANDLE // No fence is used here
        };
        VkResult vr = vkAcquireNextImage2KHR(mDevice.GetVkDevice(), &acquireInfo, &imageIndex);
        if (vr != VK_SUCCESS && vr != VK_SUBOPTIMAL_KHR)
        {
            CGS_LOG_ERROR("Failed to acquire next image from swap chain: %s", VkResultToString(vr));
            return UINT32_MAX; // Return an invalid index on failure
        }

        return imageIndex; // Return the acquired image index
    }
} // namespace cgs::graphics::rhi