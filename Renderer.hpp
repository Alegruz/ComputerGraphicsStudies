#pragma once

#include "Renderer.h"

namespace cgs
{
    extern std::vector<SubRenderThreadInfo> gSubRenderThreads;

    template<eRasterizationMethod METHOD /*= eRasterizationMethod::DEFAULT*/>
    void
    Rasterize(RenderWork& renderWork) noexcept
    {
        const uint32 width = renderWork.OutTexture.GetWidth();
        const uint32 height = renderWork.OutTexture.GetHeight();

        const Geometry* emissiveGeometryOrNull = nullptr;
        for (const Geometry& geometry : renderWork.Geometries)
        {
            if (geometry.IsEmissive() == true)
            {
                emissiveGeometryOrNull = &geometry;
                break;
            }
        }
        
        if (emissiveGeometryOrNull == nullptr)
        {
            assert(false && "No emissive geometry found in the scene");
            return;
        }
        
        const Geometry& emissiveGeometry = *emissiveGeometryOrNull;

        const uint32 subRenderThreadsCount = static_cast<uint32>(gSubRenderThreads.size());
        std::vector<uint32> finalTileIndicesPerThread;
        finalTileIndicesPerThread.resize(subRenderThreadsCount, 0);
        for (const Geometry& geometry : renderWork.Geometries)
        {
            const VertexBuffer& vertexBuffer = geometry.GetVertexBuffer();
            const std::vector<uint16>& indices = geometry.GetIndices();

            const uint32 trianglesCount = static_cast<uint32>(indices.size()) / 3;
            for (uint32 i = 0; i < trianglesCount; ++i)
            {
                const uint16 i0 = indices[i * 3 + 0];
                const uint16 i1 = indices[i * 3 + 1];
                const uint16 i2 = indices[i * 3 + 2];

                const VertexPN* v0OrNull = nullptr;
                vertexBuffer.GetVertexOrNull(v0OrNull, i0);
                if (v0OrNull == nullptr) continue;
                const CornellBoxVertexShaderOutput v0 = CornellBoxVertexShader(*v0OrNull);
                const VertexPN* v1OrNull = nullptr;
                vertexBuffer.GetVertexOrNull(v1OrNull, i1);
                if (v1OrNull == nullptr) continue;
                const CornellBoxVertexShaderOutput v1 = CornellBoxVertexShader(*v1OrNull);
                const VertexPN* v2OrNull = nullptr;
                vertexBuffer.GetVertexOrNull(v2OrNull, i2);
                if (v2OrNull == nullptr) continue;
                const CornellBoxVertexShaderOutput v2 = CornellBoxVertexShader(*v2OrNull);

                if constexpr (METHOD == eRasterizationMethod::BARYCENTRIC)
                {
                    constexpr uint32 TILE_SIZE = 64;
                    const uint32 tilesCountX = (width + TILE_SIZE - 1) / TILE_SIZE;

                    const float minNdcX = ((std::min(v0.NdcPosition.X, std::min(v1.NdcPosition.X, v2.NdcPosition.X)) + 1.0f) * 0.5f) * static_cast<float>(width);
                    const float maxNdcX = ((std::max(v0.NdcPosition.X, std::max(v1.NdcPosition.X, v2.NdcPosition.X)) + 1.0f) * 0.5f) * static_cast<float>(width);
                    const float minNdcY = ((std::min(v0.NdcPosition.Y, std::min(v1.NdcPosition.Y, v2.NdcPosition.Y)) + 1.0f) * 0.5f) * static_cast<float>(height);
                    const float maxNdcY = ((std::max(v0.NdcPosition.Y, std::max(v1.NdcPosition.Y, v2.NdcPosition.Y)) + 1.0f) * 0.5f) * static_cast<float>(height);

                    const uint32 minTileXIndex = static_cast<uint32>(std::floor(minNdcX / static_cast<float>(TILE_SIZE)));
                    const uint32 maxTileXIndex = static_cast<uint32>(std::ceil(maxNdcX / static_cast<float>(TILE_SIZE)));
                    const uint32 minTileYIndex = static_cast<uint32>(std::floor(minNdcY / static_cast<float>(TILE_SIZE)));
                    const uint32 maxTileYIndex = static_cast<uint32>(std::ceil(maxNdcY / static_cast<float>(TILE_SIZE)));

                    for(uint32 tileYIndex = minTileYIndex; tileYIndex < maxTileYIndex; ++tileYIndex)
                    {
                        const uint32 minY = tileYIndex * TILE_SIZE;
                        uint32 maxY = minY + TILE_SIZE;
                        if(maxY > height)
                        {
                            maxY = height;
                        }

                        for(uint32 tileXIndex = minTileXIndex; tileXIndex < maxTileXIndex; ++tileXIndex)
                        {
                            const uint32 minX = tileXIndex * TILE_SIZE;
                            uint32 maxX = minX + TILE_SIZE;
                            if(maxX > width)
                            {
                                maxX = width;
                            }

                            {
                                const uint32 tileIndex = tileYIndex * tilesCountX + tileXIndex;
                                const uint32 subRenderThreadIndex = tileIndex % subRenderThreadsCount;
                                finalTileIndicesPerThread[subRenderThreadIndex] = tileIndex;

#if 1
                                std::lock_guard<std::mutex> lock(gSubRenderThreads[subRenderThreadIndex].RenderWorksMutex);
                                gSubRenderThreads[subRenderThreadIndex].SubRenderWorks.push(
                                    SubRenderWork
                                    {
                                        .ParentRenderWork = renderWork,
                                        .CurrentGeometry = geometry,
                                        .EmissiveGeometry = emissiveGeometry,
                                        .V0 = v0,
                                        .V1 = v1,
                                        .V2 = v2,
                                        .MinX = minX,
                                        .MaxX = maxX,
                                        .MinY = minY,
                                        .MaxY = maxY,
                                        .WorkIndex = tileIndex,
                                    }
                                    );
#else
                                SubRenderWork subRenderWork
                                {
                                    .ParentRenderWork = renderWork,
                                    .CurrentGeometry = geometry,
                                    .EmissiveGeometry = emissiveGeometry,
                                    .V0 = v0,
                                    .V1 = v1,
                                    .V2 = v2,
                                    .MinX = minX,
                                    .MaxX = maxX,
                                    .MinY = minY,
                                    .MaxY = maxY,
                                    .WorkIndex = tileIndex,
                                };
                                SubRasterize(subRenderWork);
#endif
                            }
                        }
                    }
                }
            }
        }

        for (uint32 i = 0; i < subRenderThreadsCount; ++i)
        {
            const uint64 currentThreadFinalWorkIndex = finalTileIndicesPerThread[i];
            SubRenderThreadInfo& subRenderThreadInfo = gSubRenderThreads[i];
            while (true)
            {
                const uint64 lastCompleteWorkIndex = subRenderThreadInfo.LastCompleteWorkIndex.load();
                const bool isThreadIdle = lastCompleteWorkIndex == std::numeric_limits<uint64>::max();
                std::lock_guard<std::mutex> lockGuard(subRenderThreadInfo.RenderWorksMutex);
                if (isThreadIdle == false && subRenderThreadInfo.SubRenderWorks.empty() == true && lastCompleteWorkIndex >= currentThreadFinalWorkIndex)
                {
                    break;
                }

                cgs::Yield();
            }
        }
    }

    template<typename T>
    CGS_INLINE void 
    RenderResource::GetElementOrNull(const T*& outElementOrNull, const uint32 index) const noexcept
    {
        outElementOrNull = nullptr;

        constexpr uint32 ELEMENT_TYPE_STRIDE = sizeof(T);
        if (ELEMENT_TYPE_STRIDE != mStrideInBytes)
        {
            assert(false && "Element type stride mismatch");
            return;
        }

        // Calculate the offset in bytes
        const size_t offset = index * mStrideInBytes;
        // Check if the offset is within the bounds of the data vector
        if (offset + mStrideInBytes <= mData.size())
        {
            outElementOrNull = reinterpret_cast<const T*>(mData.data() + offset);
        }
    }

    template <eRenderDeviceType RENDER_DEVICE_TYPE>
    bool
    InitializeRenderer() noexcept
    {
        static_assert(RENDER_DEVICE_TYPE != eRenderDeviceType::COUNT, "Invalid render device type");
        return false;
    }

    template<typename T>
    CGS_INLINE constexpr bool 
    VertexBuffer::AddVertex(const T& vertex) noexcept
    {
        // Calculate the size of the vertex in bytes
        constexpr uint32 VERTEX_STRIDE = sizeof(T);
        if(VERTEX_STRIDE != mStrideInBytes)
        {
            assert(false && "Vertex stride mismatch");
            return false;
        }
        
        const uint32 verticesCount = static_cast<uint32>(mData.size() / mStrideInBytes);
        if (verticesCount >= std::numeric_limits<uint16>::max())
        {
            assert(false && "Vertex count exceeds maximum limit");
            return false; // Prevent excessive memory usage
        }

        // Resize the data vector to accommodate the new vertex
        mData.resize(mData.size() + VERTEX_STRIDE);
        // Copy the vertex data into the buffer
        std::memcpy(mData.data() + mData.size() - VERTEX_STRIDE, &vertex, VERTEX_STRIDE);
        return true;
    }
}