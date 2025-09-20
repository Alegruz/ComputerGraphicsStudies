#pragma once

#include "Common/Thread.h"

namespace cgs
{
    class Geometry;

    struct ThreadHandle;

    struct Rgba8 final
    {
        byte R = 0;
        byte G = 0;
        byte B = 0;
        byte A = 255;
    };

    struct Bgra8 final
    {
        byte B = 0;
        byte G = 0;
        byte R = 0;
        byte A = 255;
    };

    static constexpr Rgba8 WHITE{ 255, 255, 255, 255 };
    static constexpr Rgba8 BLACK{ 0, 0, 0, 255 };
    static constexpr Rgba8 RED{ 255, 0, 0, 255 };
    static constexpr Rgba8 GREEN{ 0, 255, 0, 255 };
    static constexpr Rgba8 BLUE{ 0, 0, 255, 255 };

    struct VertexPN final
    {
        Coordinate<eCoordinateSpace::WORLD> Position;
        Coordinate<eCoordinateSpace::WORLD> Normal;
    };

    constexpr uint32 BACK_BUFFERS_COUNT = 3;

    enum class eRasterizationMethod : uint8
    {
        DEFAULT,
        BARYCENTRIC = DEFAULT,
    };

    enum class eRenderMethod : uint8
    {
        RASTERIZATION = 0,
        RAYTRACING,
        DEFAULT = 0,
    };

    struct CornellBoxVertexShaderOutput final
    {
        Coordinate<eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE> NdcPosition;
        Coordinate<eCoordinateSpace::WORLD> WsPosition;
        Coordinate<eCoordinateSpace::WORLD> Normal;
    };

    struct RenderWork final
    {
        const std::vector<std::unique_ptr<Geometry>>& Geometries;
        uint64 WorkIndex;
        uint32 FrameIndex;
        const float DeltaTime;
    };

    struct RenderThreadInfo final
    {
        std::shared_ptr<ThreadHandle> CurrentThreadHandle;
        eRenderMethod RenderMethod;

        std::mutex RenderWorksMutex;
        std::queue<RenderWork> RenderWorksPerFrame;
        std::atomic<uint64> CurrentWorkIndex = std::numeric_limits<uint64>::max();
        std::atomic<uint64> LastCompleteWorkIndex = std::numeric_limits<uint64>::max();

        std::atomic<bool> IsActive = true;
    };

    void
    RenderThreadMain(ThreadProcessArgument& arg) noexcept;

    class Camera final
    {
    public:
        struct CreateInfo final
        {
            Coordinate<eCoordinateSpace::WORLD> Position;
            Coordinate<eCoordinateSpace::WORLD> Front;
            Coordinate<eCoordinateSpace::WORLD> Up;
            float FieldOfViewYAxis = std::numbers::pi_v<float> / 4.0f;
            float AspectRatio;
            float NearPlane = 0.1f;
            float FarPlane = 2000.0f;
        };

        struct Buffer final
        {
            float4x4 ViewMatrix;
            float4x4 ProjectionMatrix;
        };

    public:
        CGS_INLINE constexpr 
        Camera() noexcept
            : mPosition()
            , mFront()
            , mUp()
        {
        }
        Camera(CreateInfo&& createInfo) noexcept;
        CGS_INLINE constexpr
        Camera(const Camera&) noexcept = default;
        CGS_INLINE constexpr
        Camera(Camera&&) noexcept = default;
        CGS_INLINE
        ~Camera() noexcept = default;

        CGS_INLINE constexpr Camera& 
        operator=(const Camera&) noexcept = default;
        CGS_INLINE constexpr Camera& 
        operator=(Camera&&) noexcept = default;

        CGS_INLINE constexpr Coordinate<eCoordinateSpace::WORLD>
        GetPosition() const noexcept { return mPosition; }
        CGS_INLINE constexpr Coordinate<eCoordinateSpace::WORLD>
        GetFront() const noexcept { return mFront; }
        CGS_INLINE constexpr Coordinate<eCoordinateSpace::WORLD>
        GetUp() const noexcept { return mUp; }
        CGS_INLINE constexpr const Buffer&
        GetBuffer() const noexcept { return mBuffer; }

    private:
        Coordinate<eCoordinateSpace::WORLD> mPosition;
        Coordinate<eCoordinateSpace::WORLD> mFront;
        Coordinate<eCoordinateSpace::WORLD> mUp;

        Buffer mBuffer;
    };

    [[nodiscard]] bool
    CreateCornellBoxScene(const eRenderMethod renderMethod, std::vector<std::unique_ptr<Geometry>>& outGeometries) noexcept;

    CornellBoxVertexShaderOutput
    CornellBoxVertexShader(const VertexPN& input) noexcept;

    struct CornellBoxFragmentShaderInput final
    {
        CornellBoxVertexShaderOutput VSOutput;
        Rgba8 Color;
        const Geometry& EmissiveGeometry;
    };
    
    Rgba8
    CornellBoxFragmentShader(const CornellBoxFragmentShaderInput& input) noexcept;

    enum class eRenderDeviceType : uint8
    {
        CPU,
        CUDA,
        D3D12,
        COUNT,
    };

    struct GlobalRenderContext final
    {
        eRenderDeviceType RenderDeviceType;
    };

    // Primary template declaration
    template<typename T>
    CGS_INLINE constexpr T ConvertStringToEnumValue(const std::string&) noexcept
    {
        static_assert(sizeof(T) == 0, "ConvertStringToEnumValue: Unsupported enum type");
        return T{};
    }

    // Specialization for eRenderDeviceType
    template<>
    CGS_INLINE constexpr eRenderDeviceType ConvertStringToEnumValue<eRenderDeviceType>(const std::string& str) noexcept
    {
        if (str == "CPU" || str == "cpu")
        {
            return eRenderDeviceType::CPU;
        }
        else if (str == "CUDA" || str == "cuda")
        {
            return eRenderDeviceType::CUDA;
        }
        return eRenderDeviceType::CPU;
    }

    template<>
    CGS_INLINE constexpr eRenderMethod ConvertStringToEnumValue<eRenderMethod>(const std::string& str) noexcept
    {
        if (str == "Rasterization" || str == "rasterization" || str == "RASTERIZATION" || str == "raster")
        {
            return eRenderMethod::RASTERIZATION;
        }
        else if (str == "Raytracing" || str == "raytracing" || str == "RAYTRACING" || str == "ray")
        {
            return eRenderMethod::RAYTRACING;
        }
        
        return eRenderMethod::RASTERIZATION;
    }

    [[nodiscard]] Camera&
    InitializeCornellBoxCamera() noexcept;

    void
    Render(const float deltaTime, uint64& inoutWorkIndex, RenderThreadInfo& inoutRenderThreadInfo, const std::vector<std::unique_ptr<Geometry>>& geometries) noexcept;
}