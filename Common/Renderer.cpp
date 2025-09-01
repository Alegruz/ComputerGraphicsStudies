#include "Common/Renderer.h"

#include <numbers>

namespace cgs
{
    static Camera gMainCamera;

    static float4x4 gViewMatrix;
    static float4x4 gProjectionMatrix;
    
    void
    InitializeCornellBoxCamera() noexcept
    {
        gMainCamera = Camera(
            Camera::CreateInfo
            {
                .Position = Coordinate<eCoordinateSpace::WORLD>{ -278.0f, 273.0f, -800.0f },
                .Front = Coordinate<eCoordinateSpace::WORLD>{ 0.0f, 0.0f, 1.0f },
                .Up = Coordinate<eCoordinateSpace::WORLD>{ 0.0f, 1.0f, 0.0f },
            }
        );
        
        float3 zAxis = Normalize(gMainCamera.GetFront());
        float3 xAxis = Normalize(Cross(gMainCamera.GetUp(), zAxis));
        float3 yAxis = Cross(zAxis, xAxis);

        gViewMatrix = float4x4
        {
            .Data =
            {
                float4( xAxis.X, xAxis.Y, xAxis.Z, -Dot(xAxis, gMainCamera.GetPosition()) ),
                float4( yAxis.X, yAxis.Y, yAxis.Z, -Dot(yAxis, gMainCamera.GetPosition()) ),
                float4( zAxis.X, zAxis.Y, zAxis.Z, -Dot(zAxis, gMainCamera.GetPosition()) ),
                float4( 0.0f, 0.0f, 0.0f, 1.0f ),
            },
        };

        const float fovY = std::numbers::pi_v<float> / 4.0f;
        const float f = 1.0f / std::tan(fovY / 2.0f);
        const float aspectRatio = 1600.0f / 900.0f;
        const float nearPlane = 0.1f;
        const float farPlane = 2000.0f;

        gProjectionMatrix = float4x4
        {
            .Data =
            {
                float4(f / aspectRatio, 0.0f, 0.0f, 0.0f),
                float4(0.0f, f, 0.0f, 0.0f),
                float4(0.0f, 0.0f, farPlane / (farPlane - nearPlane), (-farPlane * nearPlane) / (farPlane - nearPlane)),
                float4(0.0f, 0.0f, 1.0f, 0.0f),
            },
        };
    }
} // namespace cgs
