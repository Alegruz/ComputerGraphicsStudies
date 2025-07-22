#pragma once

#include "Core/pch.h"

#if defined(CGS_WIN32)
#include <windows.h>
#elif defined(CGS_LINUX)
#endif  // defined(CGS_WIN32)

#include "volk/volk.h"

// SLANG
#include "slang-com-ptr.h"
#include "slang.h"

#include "pugixml.hpp"

#include "rapidobj/rapidobj.hpp"

namespace cgs
{
    constexpr const char* CONFIG_RENDERER_CONFIG_FILE_PATH("RendererConfigFilePath");
    constexpr const char* CONFIG_ENABLE_DEBUG_LAYER("EnableDebugLayer");
    constexpr const char* CONFIG_PHYSICAL_DEVICE_GROUP_INDEX("PhysicalDeviceGroupIndex");
    constexpr const char* CONFIG_PHYSICAL_DEVICE_INDEX("PhysicalDeviceIndex");
    constexpr const char* CONFIG_PHYSICAL_DEVICE("PhysicalDevice");
    constexpr const char* CONFIG_WIDTH("Width");
    constexpr const char* CONFIG_HEIGHT("Height");
    constexpr const char* CONFIG_FRAME_BUFFER_COUNT("FrameBufferCount");
    constexpr const char* CONFIG_RENDERERS_PATH("RenderersPath");
    constexpr const char* CONFIG_RENDER_GRAPH_FILE_PATH("RenderGraphFilePath");
	constexpr const char* CONFIG_RENDER_PIPELINES_PATH("RenderPipelinesPath");
    constexpr const char* CONFIG_RENDERABLES_PATH("RenderablesPath");
    constexpr const char* CONFIG_SELECTED_RENDERABLE("SelectedRenderable");

    namespace graphics
    {
        namespace rhi
        {
            class CommandPool;
            class Device;
            class Instance;
            class PhysicalDeviceGroup;
            class PhysicalDevice;
            class QueueFamily;
            class VertexLayout;

            CGS_INLINE constexpr const char* VkResultToString(VkResult result)
            {
                switch (result)
                {
                case VK_SUCCESS: return "VK_SUCCESS";
                case VK_NOT_READY: return "VK_NOT_READY";
                case VK_TIMEOUT: return "VK_TIMEOUT";
                case VK_EVENT_SET: return "VK_EVENT_SET";
                case VK_EVENT_RESET: return "VK_EVENT_RESET";
                case VK_INCOMPLETE: return "VK_INCOMPLETE";
                case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
                case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
                case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
                case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
                case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
                case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
                case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
                case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
                case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
                case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
                case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
                case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
                case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
                case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
                case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
                case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
                case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
                case VK_PIPELINE_COMPILE_REQUIRED: return "VK_PIPELINE_COMPILE_REQUIRED";
                case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
                case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
                case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
                case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
                case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
                case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
                case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
                case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR: return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
                case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
                case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
                case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
                case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
                case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
                case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
                case VK_ERROR_NOT_PERMITTED_KHR: return "VK_ERROR_NOT_PERMITTED_KHR";
                case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
                case VK_THREAD_IDLE_KHR: return "VK_THREAD_IDLE_KHR";
                case VK_THREAD_DONE_KHR: return "VK_THREAD_DONE_KHR";
                case VK_OPERATION_DEFERRED_KHR: return "VK_OPERATION_DEFERRED_KHR";
                case VK_OPERATION_NOT_DEFERRED_KHR: return "VK_OPERATION_NOT_DEFERRED_KHR";
                case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
                default: return "VK_RESULT_UNKNOWN";
                }
            }

            CGS_INLINE constexpr VkFormat StringToVkFormat(const std::string& formatStr)
            {
                if (formatStr == "UNDEFINED") return VK_FORMAT_UNDEFINED;
                if (formatStr == "R4G4_UNORM_PACK8") return VK_FORMAT_R4G4_UNORM_PACK8;
                if (formatStr == "R4G4B4A4_UNORM_PACK16") return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
                if (formatStr == "B4G4R4A4_UNORM_PACK16") return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
                if (formatStr == "R5G6B5_UNORM_PACK16") return VK_FORMAT_R5G6B5_UNORM_PACK16;
                if (formatStr == "B5G6R5_UNORM_PACK16") return VK_FORMAT_B5G6R5_UNORM_PACK16;
                if (formatStr == "R5G5B5A1_UNORM_PACK16") return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
                if (formatStr == "B5G5R5A1_UNORM_PACK16") return VK_FORMAT_B5G5R5A1_UNORM_PACK16;
                if (formatStr == "A1R5G5B5_UNORM_PACK16") return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
                if (formatStr == "R8_UNORM") return VK_FORMAT_R8_UNORM;
                if (formatStr == "R8_SNORM") return VK_FORMAT_R8_SNORM;
                if (formatStr == "R8_USCALED") return VK_FORMAT_R8_USCALED;
                if (formatStr == "R8_SSCALED") return VK_FORMAT_R8_SSCALED;
                if (formatStr == "R8_UINT") return VK_FORMAT_R8_UINT;
                if (formatStr == "R8_SINT") return VK_FORMAT_R8_SINT;
                if (formatStr == "R8_SRGB") return VK_FORMAT_R8_SRGB;
                if (formatStr == "R8G8_UNORM") return VK_FORMAT_R8G8_UNORM;
                if (formatStr == "R8G8_SNORM") return VK_FORMAT_R8G8_SNORM;
                if (formatStr == "R8G8_USCALED") return VK_FORMAT_R8G8_USCALED;
                if (formatStr == "R8G8_SSCALED") return VK_FORMAT_R8G8_SSCALED;
                if (formatStr == "R8G8_UINT") return VK_FORMAT_R8G8_UINT;
                if (formatStr == "R8G8_SINT") return VK_FORMAT_R8G8_SINT;
                if (formatStr == "R8G8_SRGB") return VK_FORMAT_R8G8_SRGB;
                if (formatStr == "R8G8B8_UNORM") return VK_FORMAT_R8G8B8_UNORM;
                if (formatStr == "R8G8B8_SNORM") return VK_FORMAT_R8G8B8_SNORM;
                if (formatStr == "R8G8B8_USCALED") return VK_FORMAT_R8G8B8_USCALED;
                if (formatStr == "R8G8B8_SSCALED") return VK_FORMAT_R8G8B8_SSCALED;
                if (formatStr == "R8G8B8_UINT") return VK_FORMAT_R8G8B8_UINT;
                if (formatStr == "R8G8B8_SINT") return VK_FORMAT_R8G8B8_SINT;
                if (formatStr == "R8G8B8_SRGB") return VK_FORMAT_R8G8B8_SRGB;
                if (formatStr == "B8G8R8_UNORM") return VK_FORMAT_B8G8R8_UNORM;
                if (formatStr == "B8G8R8_SNORM") return VK_FORMAT_B8G8R8_SNORM;
                if (formatStr == "B8G8R8_USCALED") return VK_FORMAT_B8G8R8_USCALED;
                if (formatStr == "B8G8R8_SSCALED") return VK_FORMAT_B8G8R8_SSCALED;
                if (formatStr == "B8G8R8_UINT") return VK_FORMAT_B8G8R8_UINT;
                if (formatStr == "B8G8R8_SINT") return VK_FORMAT_B8G8R8_SINT;
                if (formatStr == "B8G8R8_SRGB") return VK_FORMAT_B8G8R8_SRGB;
                if (formatStr == "R8G8B8A8_UNORM") return VK_FORMAT_R8G8B8A8_UNORM;
                if (formatStr == "R8G8B8A8_SNORM") return VK_FORMAT_R8G8B8A8_SNORM;
                if (formatStr == "R8G8B8A8_USCALED") return VK_FORMAT_R8G8B8A8_USCALED;
                if (formatStr == "R8G8B8A8_SSCALED") return VK_FORMAT_R8G8B8A8_SSCALED;
                if (formatStr == "R8G8B8A8_UINT") return VK_FORMAT_R8G8B8A8_UINT;
                if (formatStr == "R8G8B8A8_SINT") return VK_FORMAT_R8G8B8A8_SINT;
                if (formatStr == "R8G8B8A8_SRGB") return VK_FORMAT_R8G8B8A8_SRGB;
                if (formatStr == "B8G8R8A8_UNORM") return VK_FORMAT_B8G8R8A8_UNORM;
                if (formatStr == "B8G8R8A8_SNORM") return VK_FORMAT_B8G8R8A8_SNORM;
                if (formatStr == "B8G8R8A8_USCALED") return VK_FORMAT_B8G8R8A8_USCALED;
                if (formatStr == "B8G8R8A8_SSCALED") return VK_FORMAT_B8G8R8A8_SSCALED;
                if (formatStr == "B8G8R8A8_UINT") return VK_FORMAT_B8G8R8A8_UINT;
                if (formatStr == "B8G8R8A8_SINT") return VK_FORMAT_B8G8R8A8_SINT;
                if (formatStr == "B8G8R8A8_SRGB") return VK_FORMAT_B8G8R8A8_SRGB;
                if (formatStr == "A8B8G8R8_UNORM_PACK32") return VK_FORMAT_A8B8G8R8_UNORM_PACK32;
                if (formatStr == "A8B8G8R8_SNORM_PACK32") return VK_FORMAT_A8B8G8R8_SNORM_PACK32;
                if (formatStr == "A8B8G8R8_USCALED_PACK32") return VK_FORMAT_A8B8G8R8_USCALED_PACK32;
                if (formatStr == "A8B8G8R8_SSCALED_PACK32") return VK_FORMAT_A8B8G8R8_SSCALED_PACK32;
                if (formatStr == "A8B8G8R8_UINT_PACK32") return VK_FORMAT_A8B8G8R8_UINT_PACK32;
                if (formatStr == "A8B8G8R8_SINT_PACK32") return VK_FORMAT_A8B8G8R8_SINT_PACK32;
                if (formatStr == "A8B8G8R8_SRGB_PACK32") return VK_FORMAT_A8B8G8R8_SRGB_PACK32;
                if (formatStr == "A2R10G10B10_UNORM_PACK32") return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
                if (formatStr == "A2R10G10B10_SNORM_PACK32") return VK_FORMAT_A2R10G10B10_SNORM_PACK32;
                if (formatStr == "A2R10G10B10_USCALED_PACK32") return VK_FORMAT_A2R10G10B10_USCALED_PACK32;
                if (formatStr == "A2R10G10B10_SSCALED_PACK32") return VK_FORMAT_A2R10G10B10_SSCALED_PACK32;
                if (formatStr == "A2R10G10B10_UINT_PACK32") return VK_FORMAT_A2R10G10B10_UINT_PACK32;
                if (formatStr == "A2R10G10B10_SINT_PACK32") return VK_FORMAT_A2R10G10B10_SINT_PACK32;
                if (formatStr == "A2B10G10R10_UNORM_PACK32") return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
                if (formatStr == "A2B10G10R10_SNORM_PACK32") return VK_FORMAT_A2B10G10R10_SNORM_PACK32;
                if (formatStr == "A2B10G10R10_USCALED_PACK32") return VK_FORMAT_A2B10G10R10_USCALED_PACK32;
                if (formatStr == "A2B10G10R10_SSCALED_PACK32") return VK_FORMAT_A2B10G10R10_SSCALED_PACK32;
                if (formatStr == "A2B10G10R10_UINT_PACK32") return VK_FORMAT_A2B10G10R10_UINT_PACK32;
                if (formatStr == "A2B10G10R10_SINT_PACK32") return VK_FORMAT_A2B10G10R10_SINT_PACK32;
                if (formatStr == "R16_UNORM") return VK_FORMAT_R16_UNORM;
                if (formatStr == "R16_SNORM") return VK_FORMAT_R16_SNORM;
                if (formatStr == "R16_USCALED") return VK_FORMAT_R16_USCALED;
                if (formatStr == "R16_SSCALED") return VK_FORMAT_R16_SSCALED;
                if (formatStr == "R16_UINT") return VK_FORMAT_R16_UINT;
                if (formatStr == "R16_SINT") return VK_FORMAT_R16_SINT;
                if (formatStr == "R16_SFLOAT") return VK_FORMAT_R16_SFLOAT;
                if (formatStr == "R16G16_UNORM") return VK_FORMAT_R16G16_UNORM;
                if (formatStr == "R16G16_SNORM") return VK_FORMAT_R16G16_SNORM;
                if (formatStr == "R16G16_USCALED") return VK_FORMAT_R16G16_USCALED;
                if (formatStr == "R16G16_SSCALED") return VK_FORMAT_R16G16_SSCALED;
                if (formatStr == "R16G16_UINT") return VK_FORMAT_R16G16_UINT;
                if (formatStr == "R16G16_SINT") return VK_FORMAT_R16G16_SINT;
                if (formatStr == "R16G16_SFLOAT") return VK_FORMAT_R16G16_SFLOAT;
                if (formatStr == "R16G16B16_UNORM") return VK_FORMAT_R16G16B16_UNORM;
                if (formatStr == "R16G16B16_SNORM") return VK_FORMAT_R16G16B16_SNORM;
                if (formatStr == "R16G16B16_USCALED") return VK_FORMAT_R16G16B16_USCALED;
                if (formatStr == "R16G16B16_SSCALED") return VK_FORMAT_R16G16B16_SSCALED;
                if (formatStr == "R16G16B16_UINT") return VK_FORMAT_R16G16B16_UINT;
                if (formatStr == "R16G16B16_SINT") return VK_FORMAT_R16G16B16_SINT;
                if (formatStr == "R16G16B16_SFLOAT") return VK_FORMAT_R16G16B16_SFLOAT;
                if (formatStr == "R16G16B16A16_UNORM") return VK_FORMAT_R16G16B16A16_UNORM;
                if (formatStr == "R16G16B16A16_SNORM") return VK_FORMAT_R16G16B16A16_SNORM;
                if (formatStr == "R16G16B16A16_USCALED") return VK_FORMAT_R16G16B16A16_USCALED;
                if (formatStr == "R16G16B16A16_SSCALED") return VK_FORMAT_R16G16B16A16_SSCALED;
                if (formatStr == "R16G16B16A16_UINT") return VK_FORMAT_R16G16B16A16_UINT;
                if (formatStr == "R16G16B16A16_SINT") return VK_FORMAT_R16G16B16A16_SINT;
                if (formatStr == "R16G16B16A16_SFLOAT") return VK_FORMAT_R16G16B16A16_SFLOAT;
                if (formatStr == "R32_UINT") return VK_FORMAT_R32_UINT;
                if (formatStr == "R32_SINT") return VK_FORMAT_R32_SINT;
                if (formatStr == "R32_SFLOAT") return VK_FORMAT_R32_SFLOAT;
                if (formatStr == "R32G32_UINT") return VK_FORMAT_R32G32_UINT;
                if (formatStr == "R32G32_SINT") return VK_FORMAT_R32G32_SINT;
                if (formatStr == "R32G32_SFLOAT") return VK_FORMAT_R32G32_SFLOAT;
                if (formatStr == "R32G32B32_UINT") return VK_FORMAT_R32G32B32_UINT;
                if (formatStr == "R32G32B32_SINT") return VK_FORMAT_R32G32B32_SINT;
                if (formatStr == "R32G32B32_SFLOAT") return VK_FORMAT_R32G32B32_SFLOAT;
                if (formatStr == "R32G32B32A32_UINT") return VK_FORMAT_R32G32B32A32_UINT;
                if (formatStr == "R32G32B32A32_SINT") return VK_FORMAT_R32G32B32A32_SINT;
                if (formatStr == "R32G32B32A32_SFLOAT") return VK_FORMAT_R32G32B32A32_SFLOAT;
                if (formatStr == "D16_UNORM") return VK_FORMAT_D16_UNORM;
                if (formatStr == "X8_D24_UNORM_PACK32") return VK_FORMAT_X8_D24_UNORM_PACK32;
                if (formatStr == "D32_SFLOAT") return VK_FORMAT_D32_SFLOAT;
                if (formatStr == "S8_UINT") return VK_FORMAT_S8_UINT;
                if (formatStr == "D16_UNORM_S8_UINT") return VK_FORMAT_D16_UNORM_S8_UINT;
                if (formatStr == "D24_UNORM_S8_UINT") return VK_FORMAT_D24_UNORM_S8_UINT;
                if (formatStr == "D32_SFLOAT_S8_UINT") return VK_FORMAT_D32_SFLOAT_S8_UINT;
                return VK_FORMAT_UNDEFINED;
            }

            struct Format final
            {
            public:
                CGS_INLINE constexpr Format(const VkFormat defaultFormat) noexcept
                    : DefaultFormat(defaultFormat)
                    , SrgbFormat()
                {
                    switch (defaultFormat)
                    {
                    case VK_FORMAT_R8_UNORM: SrgbFormat = VK_FORMAT_R8_SRGB; break;
                    case VK_FORMAT_R8G8_UNORM: SrgbFormat = VK_FORMAT_R8G8_SRGB; break;
                    case VK_FORMAT_R8G8B8_UNORM: SrgbFormat = VK_FORMAT_R8G8B8_SRGB; break;
                    case VK_FORMAT_B8G8R8_UNORM: SrgbFormat = VK_FORMAT_B8G8R8_SRGB; break;
                    case VK_FORMAT_R8G8B8A8_UNORM: SrgbFormat = VK_FORMAT_R8G8B8A8_SRGB; break;
                    case VK_FORMAT_B8G8R8A8_UNORM: SrgbFormat = VK_FORMAT_B8G8R8A8_SRGB; break;
                    case VK_FORMAT_A8B8G8R8_UNORM_PACK32: SrgbFormat = VK_FORMAT_A8B8G8R8_SRGB_PACK32; break;
                    default: SrgbFormat = DefaultFormat; break;
                    }
                }

                CGS_INLINE constexpr Format() noexcept : Format(VK_FORMAT_UNDEFINED) {}

                CGS_INLINE constexpr bool HasSrgbFormat() const noexcept
                {
                    return SrgbFormat != VK_FORMAT_UNDEFINED && SrgbFormat != DefaultFormat;
                }

            public:
                VkFormat DefaultFormat = VK_FORMAT_UNDEFINED; // Default format
                VkFormat SrgbFormat = VK_FORMAT_UNDEFINED; // SRGB format
            };
        } // namespace rhi

        using VertexLayoutsMap = std::unordered_map<std::string, std::unique_ptr<rhi::VertexLayout>>;
    } // namespace graphics
} // namespace cgs::graphics
