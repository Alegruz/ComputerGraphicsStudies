#include "Graphics/pch.h"

#include "Graphics/RHI/Attachment.h"

#include "Graphics/RHI/Device.h"
#include "Graphics/RHI/Image.h"
#include "Graphics/RHI/Instance.h"
#include "Graphics/RHI/PhysicalDevice.h"
#include "Graphics/RHI/PhysicalDeviceGroup.h"
#include "Graphics/RHI/QueueFamily.h"
#include "Graphics/RHI/SwapChain.h"

namespace cgs::graphics::rhi
{
    Attachment::Attachment(const CreateInfo& createInfo) noexcept
        : mInstance(createInfo.RhiInstance) // Initialize the RHI instance reference
        , mName(createInfo.Name) // Initialize the attachment name
        , mColorAttachments()
        , mDepthAttachments()
    {
        Format colorFormat;
        Format depthFormat;

        PhysicalDevice& physicalDevice = createInfo.RhiInstance.GetMainPhysicalDeviceGroup().GetMainPhysicalDevice();
        Device& device = physicalDevice.GetLogicalDevice();
        SwapChain& swapChain = device.GetSwapChain();
        
        const bool bIsBackBuffer = IsBackBuffer(); // Check if the attachment is a back buffer
        const bool bIsFrameBuffered = createInfo.Node.attribute("FrameBuffered").as_bool(false); // Check if the attachment is frame buffered
        uint32_t backBufferCount = swapChain.GetBackBufferCount();
        if(bIsBackBuffer == false && bIsFrameBuffered && backBufferCount <= 0)
        {
            CGS_LOG_ERROR("Attachment '%s' is marked as frame buffered but the swap chain has no back buffers.", mName.c_str());
            return; // Return early if the attachment is frame buffered but no back buffers are available
        }

        uint32_t width = swapChain.GetWidth();
        uint32_t height = swapChain.GetHeight();

        for (const pugi::xml_node& elementNode : createInfo.Node.children())
        {
            const std::string_view elementName = elementNode.name();
            if (elementName == "SizeScale")
            {
                width = static_cast<uint32_t>(static_cast<float>(width) * elementNode.attribute("Width").as_float(1.0f)); // Default to 1.0f if not specified
                height = static_cast<uint32_t>(static_cast<float>(height) * elementNode.attribute("Height").as_float(1.0f)); // Default to 1.0f if not specified
            }
            else if (elementName == "ColorFormat")
            {
				const char* value = elementNode.children().begin()->value(); // Get the value of the first child node
                colorFormat = Format(StringToVkFormat(value));
                if (colorFormat.DefaultFormat == VK_FORMAT_UNDEFINED)
                {
                    CGS_LOG_ERROR("Invalid color format '%s' for attachment '%s'.", value, mName.c_str());
                    colorFormat = Format(VK_FORMAT_B8G8R8A8_UNORM); // Default to a common format if invalid
                };
            }
            else if (elementName == "DepthFormat")
            {
                const char* value = elementNode.children().begin()->value(); // Get the value of the first child node
                depthFormat = Format(StringToVkFormat(value));
                if (depthFormat.DefaultFormat == VK_FORMAT_UNDEFINED)
                {
                    CGS_LOG_ERROR("Invalid depth format '%s' for attachment '%s'.", value, mName.c_str());
                    depthFormat = Format(VK_FORMAT_D32_SFLOAT); // Default to a common depth format if invalid
                }
            }
            else
            {
                CGS_LOG_WARNING("Unknown element '%s' in attachment '%s'.", elementName.data(), mName.c_str());
            }
        }

        VkResult vr = VK_SUCCESS;
        std::vector<VkImage> swapChainImages;
        if (bIsBackBuffer)
        {
            vr = vkGetSwapchainImagesKHR(device.GetVkDevice(), swapChain.GetVkSwapChain(), &backBufferCount, nullptr);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to get swap chain image count: %s", VkResultToString(vr));
                return; // Return early on failure
            }

            swapChainImages.resize(backBufferCount);
            vr = vkGetSwapchainImagesKHR(device.GetVkDevice(), swapChain.GetVkSwapChain(), &backBufferCount, swapChainImages.data());
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to get swap chain images: %s", VkResultToString(vr));
                return;
            }
        }
        else
        {
            // If not a back buffer, we assume a single image for the attachment
            swapChainImages.resize(1);
            swapChainImages[0] = VK_NULL_HANDLE; // Placeholder, will be created later
        }

        uint32_t frameBufferedCount = bIsFrameBuffered ? backBufferCount : 1; // Use back buffer count if frame buffered, otherwise use 1
        mColorAttachments.reserve(frameBufferedCount);
        mDepthAttachments.reserve(frameBufferedCount);
        for (const auto& image : swapChainImages)
        {
            Image::CreateInfo imageCreateInfo =
            {
                .ResourceCreateInfo =
                {
                    .RhiDevice = device,
                },
                .Image = image,
            };

            if(imageCreateInfo.Image == VK_NULL_HANDLE)
            {
                VkImageCreateInfo colorImageCreateInfo =
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0, // No special flags
                    .imageType = VK_IMAGE_TYPE_2D, // 2D image
                    .format = colorFormat.DefaultFormat, // Depth format
                    .extent = { .width = width, .height = height, .depth = 1, }, // Example extent, should match swap chain size
                    .mipLevels = 1,
                    .arrayLayers = 1,
                    .samples = VK_SAMPLE_COUNT_1_BIT, // No multisampling
                    .tiling = VK_IMAGE_TILING_OPTIMAL,
                    .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, // Color attachment and transfer source
                    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                    .queueFamilyIndexCount = 0,
                    .pQueueFamilyIndices = nullptr,
                    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED // Initial layout
                };
                vr = vkCreateImage(device.GetVkDevice(), &colorImageCreateInfo, nullptr, &imageCreateInfo.Image);
                if (vr != VK_SUCCESS)
                {
                    CGS_LOG_ERROR("Failed to create color image: %s", VkResultToString(vr));
                    continue; // Skip this image if creation fails
                }
                
                VkMemoryRequirements colorImageMemoryRequirements = {};
                vkGetImageMemoryRequirements(device.GetVkDevice(), imageCreateInfo.Image, &colorImageMemoryRequirements);

                VkMemoryAllocateInfo colorImageMemoryAllocateInfo =
                {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                    .pNext = nullptr,
                    .allocationSize = colorImageMemoryRequirements.size,
                    .memoryTypeIndex = device.GetPhysicalDevice().GetMemoryTypeIndex(
                        colorImageMemoryRequirements.memoryTypeBits,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT // Device local memory for optimal performance
                    ),
                };

                vr = vkAllocateMemory(device.GetVkDevice(), &colorImageMemoryAllocateInfo, nullptr, &imageCreateInfo.ResourceCreateInfo.DeviceMemory);
                if (vr != VK_SUCCESS)
                {
                    CGS_LOG_ERROR("Failed to allocate memory for depth image: %s", VkResultToString(vr));
                    vkDestroyImage(device.GetVkDevice(), imageCreateInfo.Image, nullptr);
                    continue; // Skip this image if memory allocation fails
                }

                vr = vkBindImageMemory(device.GetVkDevice(), imageCreateInfo.Image, imageCreateInfo.ResourceCreateInfo.DeviceMemory, 0);
                if (vr != VK_SUCCESS)
                {
                    CGS_LOG_ERROR("Failed to bind memory to depth image: %s", VkResultToString(vr));
                    vkFreeMemory(device.GetVkDevice(), imageCreateInfo.ResourceCreateInfo.DeviceMemory, nullptr);
                    vkDestroyImage(device.GetVkDevice(), imageCreateInfo.Image, nullptr);
                    continue; // Skip this image if binding fails
                }
            }

            VkImageViewCreateInfo imageViewCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0, // No special flags
                .image = imageCreateInfo.Image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D, // 2D image view
                .format = colorFormat.DefaultFormat, // Assuming SRGB format for color attachment
                .components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } // Color aspect
            };

            vr = vkCreateImageView(device.GetVkDevice(), &imageViewCreateInfo, nullptr, &imageCreateInfo.ImageView);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to create image view: %s", VkResultToString(vr));
                continue; // Skip this image if view creation fails
            }

            Image::CreateInfo depthImageCreateInfo =
            {
                .ResourceCreateInfo =
                {
                    .RhiDevice = device,
                },
            };

            VkImageCreateInfo depthStencilImageCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0, // No special flags
                .imageType = VK_IMAGE_TYPE_2D, // 2D image
                .format = depthFormat.DefaultFormat, // Depth format
                .extent = { .width = width, .height = height, .depth = 1, }, // Example extent, should match swap chain size
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
            vr = vkCreateImage(device.GetVkDevice(), &depthStencilImageCreateInfo, nullptr, &depthImageCreateInfo.Image);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to create depth image: %s", VkResultToString(vr));
                continue; // Skip this image if creation fails
            }
            
            VkMemoryRequirements depthImageMemoryRequirements = {};
            vkGetImageMemoryRequirements(device.GetVkDevice(), depthImageCreateInfo.Image, &depthImageMemoryRequirements);

            VkMemoryAllocateInfo depthImageMemoryAllocateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = nullptr,
                .allocationSize = depthImageMemoryRequirements.size,
                .memoryTypeIndex = device.GetPhysicalDevice().GetMemoryTypeIndex(
                    depthImageMemoryRequirements.memoryTypeBits,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT // Device local memory for optimal performance
                ),
            };

            vr = vkAllocateMemory(device.GetVkDevice(), &depthImageMemoryAllocateInfo, nullptr, &depthImageCreateInfo.ResourceCreateInfo.DeviceMemory);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to allocate memory for depth image: %s", VkResultToString(vr));
                vkDestroyImage(device.GetVkDevice(), depthImageCreateInfo.Image, nullptr);
                continue; // Skip this image if memory allocation fails
            }

            vr = vkBindImageMemory(device.GetVkDevice(), depthImageCreateInfo.Image, depthImageCreateInfo.ResourceCreateInfo.DeviceMemory, 0);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to bind memory to depth image: %s", VkResultToString(vr));
                vkFreeMemory(device.GetVkDevice(), depthImageCreateInfo.ResourceCreateInfo.DeviceMemory, nullptr);
                vkDestroyImage(device.GetVkDevice(), depthImageCreateInfo.Image, nullptr);
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
            vr = vkCreateImageView(device.GetVkDevice(), &depthImageViewCreateInfo, nullptr, &depthImageCreateInfo.ImageView);
            if (vr != VK_SUCCESS)
            {
                CGS_LOG_ERROR("Failed to create depth image view: %s", VkResultToString(vr));
                vkFreeMemory(device.GetVkDevice(), depthImageCreateInfo.ResourceCreateInfo.DeviceMemory, nullptr);
                vkDestroyImage(device.GetVkDevice(), depthImageCreateInfo.Image, nullptr);
                continue; // Skip this image if view creation fails
            }
            
            mColorAttachments.emplace_back(std::make_unique<Image>(imageCreateInfo));
            mDepthAttachments.emplace_back(std::make_unique<Image>(depthImageCreateInfo));
        }
    }
} // namespace cgs::graphics::rhi