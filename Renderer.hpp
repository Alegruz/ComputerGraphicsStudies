#pragma once

#include "Renderer.h"

namespace cgs
{
    extern std::vector<std::shared_ptr<ThreadHandle>> gShaderThreads;

    struct SubRasterizeInfo final
    {
        Texture& InoutTexture;
        const Geometry& CurrentGeometry;
        const Geometry& EmissiveGeometry;

        const CornellBoxVertexShaderOutput& V0;
        const CornellBoxVertexShaderOutput& V1;
        const CornellBoxVertexShaderOutput& V2;
        uint32 MinX = 0;
        uint32 MaxX = 0;
        uint32 MinY = 0;
        uint32 MaxY = 0;
    };

    void
    SubRasterize(ThreadProcessArgument& arg) noexcept;

    template<eRasterizationMethod METHOD /*= eRasterizationMethod::DEFAULT*/>
    void
    Rasterize(Texture& outTexture, const std::vector<Geometry>& geometries) noexcept
    {
        if (gShaderThreads.empty() == true)
        {
            const uint32 availableProcessorsCount = GetLogicalProcessorsCount() - 2;  // Leave 2 cores free
            gShaderThreads.resize(static_cast<size_t>(availableProcessorsCount));
        }

        const uint32 width = outTexture.GetWidth();
        const uint32 height = outTexture.GetHeight();

        const Geometry* emissiveGeometryOrNull = nullptr;
        for (const Geometry& geometry : geometries)
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
                    const uint32 tilesCountY = (height + TILE_SIZE - 1) / TILE_SIZE;

                    std::vector<SubRasterizeInfo> subRasterizeInfos;
                    subRasterizeInfos.reserve(tilesCountX * tilesCountY);

                    for(uint32 tileYIndex = 0; tileYIndex < tilesCountY; ++tileYIndex)
                    {
                        const uint32 minY = tileYIndex * TILE_SIZE;
                        uint32 maxY = minY + TILE_SIZE;
                        if(maxY > height)
                        {
                            maxY = height;
                        }

                        for(uint32 tileXIndex = 0; tileXIndex < tilesCountX; ++tileXIndex)
                        {
                            const uint32 minX = tileXIndex * TILE_SIZE;
                            uint32 maxX = minX + TILE_SIZE;
                            if(maxX > width)
                            {
                                maxX = width;
                            }

                            subRasterizeInfos.push_back(
                                SubRasterizeInfo
                                {
                                    .InoutTexture = outTexture,
                                    .CurrentGeometry = geometry,
                                    .EmissiveGeometry = emissiveGeometry,
                                    .V0 = v0,
                                    .V1 = v1,
                                    .V2 = v2,
                                    .MinX = minX,
                                    .MaxX = maxX,
                                    .MinY = minY,
                                    .MaxY = maxY,
                                }
                            );
                        }
                    }

                    uint32 finishedTileCount = 0;
                    uint32 tileIndexToProcess = 0;
                    uint32 activeThreadsCount = 0;
                    const uint32 maximumActiveThreadsCount = static_cast<uint32>(gShaderThreads.size());
                    if(maximumActiveThreadsCount == 0)
                    {
                        assert(false && "No available threads for rasterization");
                        return;
                    }

                    while (finishedTileCount < subRasterizeInfos.size())
                    {
                        for(uint32 threadIndex = 0; threadIndex < gShaderThreads.size(); ++threadIndex)
                        {
                            std::shared_ptr<ThreadHandle>& threadHandle = gShaderThreads[threadIndex];

                            const bool isValidThread = threadHandle != nullptr && IsThreadValid(*threadHandle);
                            const bool isFinished = isValidThread && IsThreadAlive(*threadHandle) == false;
                            if (isValidThread == false || isFinished)
                            {
                                if(isFinished)
                                {
                                    Join(*threadHandle);
                                    --activeThreadsCount;
                                    ++finishedTileCount;
                                }

                                if (tileIndexToProcess < subRasterizeInfos.size() && activeThreadsCount < maximumActiveThreadsCount)
                                {
                                    ThreadCreateInfo threadCreateInfo =
                                    {
                                        .Name = "RasterizationThread",
                                        .StackSize = 0,
                                        .Process = &SubRasterize,
                                        .Argument = &subRasterizeInfos[tileIndexToProcess]
                                    };
                                    bool threadCreateResult = Create(threadHandle, threadCreateInfo);
                                    if (threadCreateResult == false)
                                    {
                                        assert(false && "Failed to create rasterization thread");
                                    }
                                    else
                                    {
                                        ++activeThreadsCount;
                                        ++tileIndexToProcess;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    template <eRenderDeviceType RENDER_DEVICE_TYPE>
    bool
    InitializeRenderer() noexcept
    {
        static_assert(RENDER_DEVICE_TYPE != eRenderDeviceType::COUNT, "Invalid render device type");
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

    template<typename T>
    CGS_INLINE void 
    VertexBuffer::GetVertexOrNull(const T*& outVertex, const uint16 index) const noexcept
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