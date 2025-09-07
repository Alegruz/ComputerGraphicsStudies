#include "Common/Renderer.h"

namespace cgs
{
    static Camera gMainCamera;
    
    Camera::Camera(Camera::CreateInfo&& createInfo) noexcept
        : mPosition(createInfo.Position)
        , mFront(createInfo.Front)
        , mUp(createInfo.Up)
        , mBuffer()
    {
        const float3 zAxis = Normalize(mFront);
        const float3 xAxis = Normalize(Cross(mUp, zAxis));
        const float3 yAxis = Cross(zAxis, xAxis);

        mBuffer.ViewMatrix = float4x4
        {
            .Data =
            {
                float4( xAxis.X, xAxis.Y, xAxis.Z, -Dot(xAxis, mPosition) ),
                float4( yAxis.X, yAxis.Y, yAxis.Z, -Dot(yAxis, mPosition) ),
                float4( zAxis.X, zAxis.Y, zAxis.Z, -Dot(zAxis, mPosition) ),
                float4( 0.0f, 0.0f, 0.0f, 1.0f ),
            },
        };

        const float f = 1.0f / std::tan(createInfo.FieldOfViewYAxis / 2.0f);

        mBuffer.ProjectionMatrix = float4x4
        {
            .Data =
            {
                float4(f / createInfo.AspectRatio, 0.0f, 0.0f, 0.0f),
                float4(0.0f, f, 0.0f, 0.0f),
                float4(0.0f, 0.0f, createInfo.FarPlane / (createInfo.FarPlane - createInfo.NearPlane), (-createInfo.FarPlane * createInfo.NearPlane) / (createInfo.FarPlane - createInfo.NearPlane)),
                float4(0.0f, 0.0f, 1.0f, 0.0f),
            },
        };
    }
    
    Camera&
    InitializeCornellBoxCamera() noexcept
    {
        gMainCamera = Camera(
            Camera::CreateInfo
            {
                .Position = Coordinate<eCoordinateSpace::WORLD>{ -278.0f, 273.0f, -800.0f },
                .Front = Coordinate<eCoordinateSpace::WORLD>{ 0.0f, 0.0f, 1.0f },
                .Up = Coordinate<eCoordinateSpace::WORLD>{ 0.0f, 1.0f, 0.0f },
                .FieldOfViewYAxis = std::numbers::pi_v<float> / 4.0f,
                .AspectRatio = 1600.0f / 900.0f,
                .NearPlane = 0.1f,
                .FarPlane = 2000.0f,
            }
        );

        return gMainCamera;
    }
} // namespace cgs
