#pragma once

#include "Renderer.h"

namespace cgs
{
    template<eCoordinateSpace SPACE, eRasterizationMethod METHOD /*= eRasterizationMethod::DEFAULT*/>
    static void
    Rasterize(Texture& outTexture, const std::vector<TriangleMesh<SPACE>>& meshes) noexcept
    {
        const uint32 width = outTexture.GetWidth();
        const uint32 height = outTexture.GetHeight();

        for (const TriangleMesh<SPACE>& mesh : meshes)
        {
            if constexpr (METHOD == eRasterizationMethod::BARYCENTRIC)
            {
                for (uint32 y = 0; y < height; ++y)
                {
                    for (uint32 x = 0; x < width; ++x)
                    {
                        if constexpr (SPACE == eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE)
                        {
                            const Coordinate<eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE> point{
                                static_cast<float>(x) / static_cast<float>(width) * 2.0f - 1.0f,
                                static_cast<float>(y) / static_cast<float>(height) * 2.0f - 1.0f,
                                0.0f
                            };
                            const Coordinate<eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE> barycentricCoords = ComputeBarycentricCoordinates(mesh, point);
                            const bool isInTriangle = 0.0f <= barycentricCoords.X && barycentricCoords.X <= 1.0f &&
                                0.0f <= barycentricCoords.Y && barycentricCoords.Y <= 1.0f &&
                                0.0f <= barycentricCoords.Z && barycentricCoords.Z <= 1.0f;
                            if (isInTriangle)
                            {
                                // Simple rasterization logic: set every pixel to a color
                                // In a real application, you would perform actual rasterization here
                                outTexture.SetFragmentValue(x, y, 255, 0, 0, 255); // Set to red
                            }
                        }
                    }
                }
            }
        }
    }
}