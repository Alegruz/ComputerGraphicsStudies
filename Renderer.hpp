#pragma once

#include "Renderer.h"

namespace cgs
{
    template<eCoordinateSpace SPACE, eRasterizationMethod METHOD /*= eRasterizationMethod::DEFAULT*/>
    void
    Rasterize(Texture& outTexture, const std::vector<Geometry>& geometries) noexcept
    {
        const uint32 width = outTexture.GetWidth();
        const uint32 height = outTexture.GetHeight();

        for (const Geometry& geometry : geometries)
        {
            const VertexBuffer& vertexBuffer = geometry.GetVertexBuffer();
            const std::vector<uint16>& indices = geometry.GetIndices();

            const uint32 trianglesCount = static_cast<uint32>(indices.size()) / 3;
            for (uint32 i = 0; i < trianglesCount; ++i)
            {
                const uint16 i0 = indices[i * 3 + 0];
                const uint16 i1 = indices[i * 3 + 1];
                const uint16 i2 = indices[i * 3 + 2];

                const Coordinate<eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE>* v0OrNull = nullptr;
                vertexBuffer.GetVertexOrNull(v0OrNull, i0);
                if (v0OrNull == nullptr) continue;
                const Coordinate<eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE>& v0 = *v0OrNull;
                const Coordinate<eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE>* v1OrNull = nullptr;
                vertexBuffer.GetVertexOrNull(v1OrNull, i1);
                if (v1OrNull == nullptr) continue;
                const Coordinate<eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE>& v1 = *v1OrNull;
                const Coordinate<eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE>* v2OrNull = nullptr;
                vertexBuffer.GetVertexOrNull(v2OrNull, i2);
                if (v2OrNull == nullptr) continue;
                const Coordinate<eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE>& v2 = *v2OrNull;

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
                                const Coordinate<eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE> barycentricCoords = ComputeBarycentricCoordinates(v0, v1, v2, point);
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

    template<typename T>
    CGS_INLINE bool VertexBuffer::AddVertex(const T& vertex) noexcept
    {
        // Calculate the size of the vertex in bytes
        constexpr uint32 VERTEX_STRIDE = sizeof(T);
        if(VERTEX_STRIDE != mStrideInBytes)
        {
            return false;
        }

        const uint32 verticesCount = static_cast<uint32>(mData.size() / mStrideInBytes);
        if (verticesCount >= std::numeric_limits<uint16>::max())
        {
            return false; // Prevent excessive memory usage
        }

        // Resize the data vector to accommodate the new vertex
        mData.resize(mData.size() + VERTEX_STRIDE);
        // Copy the vertex data into the buffer
        std::memcpy(mData.data() + mData.size() - VERTEX_STRIDE, &vertex, VERTEX_STRIDE);
        return true;
    }

    template<typename T>
    CGS_INLINE void VertexBuffer::GetVertexOrNull(const T*& outVertex, const uint16 index) const noexcept
    {
        outVertex = nullptr;

        constexpr uint32 VERTEX_STRIDE = sizeof(T);
        if (VERTEX_STRIDE != mStrideInBytes)
        {
            return;
        }

        // Calculate the offset in bytes
        const size_t offset = index * mStrideInBytes;
        // Check if the offset is within the bounds of the data vector
        if (offset + mStrideInBytes <= mData.size())
        {
            outVertex = reinterpret_cast<const T*>(mData.data() + offset);
        }
    }
}