#include "pch.hpp"

#include "Common/Renderer.cpp"

#if defined(CGS_GRAPHICS_API_D3D12)
#include "D3D12/Renderer.h"

namespace cgs
{
    static GlobalRenderContext gGlobalRenderContext;

    using TDXGIGetDebugInterface = HRESULT (*)(REFIID, void**);

    struct SceneRenderTarget final
    {
        Texture ColorBuffer;
        Texture DepthBuffer;
    };

    struct GpuRenderWork final
    {
        SceneRenderTarget& InoutRenderTarget;
        RenderWork& Work;
    };

    // DXGI
#if defined(CGS_DEBUG)
    static D3DPtr<IDXGIDebug> gDxgiDebug;
    static D3DPtr<IDXGIInfoQueue> gInfoQueue;
#endif  // defined(CGS_DEBUG)
    static D3DPtr<IDXGIFactory6> gFactory;
    static D3DPtr<IDXGIAdapter> gAdapter;
    static D3DPtr<IDXGISwapChain3> gSwapChain;
    
    // D3D12
#if defined(CGS_DEBUG)
    static D3DPtr<ID3D12Debug5> gD3D12Debug;
#endif  // defined(CGS_DEBUG)
    static D3DPtr<ID3D12Device4> gDevice;
    static D3DPtr<ID3D12GraphicsCommandList> gGraphicsCommandList;
    static D3DPtr<ID3D12CommandAllocator> gGraphicsCommandAllocator;
    static D3DPtr<ID3D12CommandQueue> gGraphicsCommandQueue;
    static D3DPtr<ID3D12DescriptorHeap> gRtvHeap;
    static D3DPtr<ID3D12DescriptorHeap> gDsvHeap;
    static uint32 gRtvIncrementSize = 0;
    static uint32 gDsvIncrementSize = 0;
    static std::vector<SceneRenderTarget> gSceneRenderTargets(BACK_BUFFERS_COUNT);

    
    void 
    RenderResource::Transition(ID3D12GraphicsCommandList& commandList, const D3D12_RESOURCE_STATES newState) noexcept
    {
        const D3D12_RESOURCE_BARRIER barrier =
        {
            .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
            .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
            .Transition =
            {
                .pResource = mData.Get(),
                .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                .StateBefore = mState,
                .StateAfter = newState,
            },
        };
        commandList.ResourceBarrier(1, &barrier);
        mState = newState;
    }

    void
    DestroyRenderer() noexcept
    {
        CGS_DESTROY_D3D12_OBJECT(gGraphicsCommandList);
        CGS_DESTROY_D3D12_OBJECT(gGraphicsCommandAllocator);
        CGS_DESTROY_D3D12_OBJECT(gGraphicsCommandQueue);

        CGS_DESTROY_DXGI_OBJECT(gSwapChain);

        CGS_DESTROY_D3D12_OBJECT(gDevice);
#if defined(CGS_DEBUG)
        CGS_DESTROY_D3D12_OBJECT(gD3D12Debug);
#endif  // defined(CGS_DEBUG)

        CGS_DESTROY_DXGI_OBJECT(gAdapter);
        CGS_DESTROY_DXGI_OBJECT(gFactory);

#if defined(CGS_DEBUG)
        if(gDxgiDebug != nullptr)
        {
            gDxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        }
        else
        {
            assert(false && "gDxgiDebug is null");
        }
        CGS_DESTROY_DXGI_OBJECT(gInfoQueue);
        CGS_DESTROY_DXGI_OBJECT(gDxgiDebug);
#endif  // defined(CGS_DEBUG)
    }

    static void
    AddQuadVertices(std::vector<VertexPN>& inoutVertices, std::vector<uint16>& inoutIndices, const Coordinate<eCoordinateSpace::WORLD>& v0, const Coordinate<eCoordinateSpace::WORLD>& v1, const Coordinate<eCoordinateSpace::WORLD>& v2, const Coordinate<eCoordinateSpace::WORLD>& v3)
    {
        const Coordinate<eCoordinateSpace::WORLD> normal = Normalize(Cross(v1 - v0, v2 - v0));
        inoutVertices.push_back(VertexPN{ v0, normal });
        inoutVertices.push_back(VertexPN{ v1, normal });
        inoutVertices.push_back(VertexPN{ v2, normal });
        inoutVertices.push_back(VertexPN{ v3, normal });
        inoutIndices.push_back(static_cast<uint16>(inoutVertices.size() - 4));
        inoutIndices.push_back(static_cast<uint16>(inoutVertices.size() - 3));
        inoutIndices.push_back(static_cast<uint16>(inoutVertices.size() - 2));
        inoutIndices.push_back(static_cast<uint16>(inoutVertices.size() - 4));
        inoutIndices.push_back(static_cast<uint16>(inoutVertices.size() - 2));
        inoutIndices.push_back(static_cast<uint16>(inoutVertices.size() - 1));
    }

    [[nodiscard]] static bool
    CreateVertexBuffer(VertexBuffer& outVertexBuffer, const std::vector<VertexPN>& vertices, const std::string& name)
    {
        HRESULT hr = S_OK;
        const D3D12_HEAP_PROPERTIES bufferHeapProperties = 
        {
            .Type = D3D12_HEAP_TYPE_UPLOAD,
            .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask = 1,
            .VisibleNodeMask = 1,
        };

        D3D12_RESOURCE_DESC bufferDesc =
        {
            .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Alignment = 0,
            .Width = 0,
            .Height = 1,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .Format = DXGI_FORMAT_UNKNOWN,
            .SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 },
            .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            .Flags = D3D12_RESOURCE_FLAG_NONE,
        };

        VertexBuffer::CreateInfo vertexBufferCreateInfo;
        vertexBufferCreateInfo.ParentCreateInfo.State = D3D12_RESOURCE_STATE_GENERIC_READ;
        vertexBufferCreateInfo.ParentCreateInfo.Name = name;
        bufferDesc.Width = sizeof(VertexPN) * vertices.size();
        hr = gDevice->CreateCommittedResource(
            &bufferHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            vertexBufferCreateInfo.ParentCreateInfo.State,
            nullptr,
            IID_PPV_ARGS(vertexBufferCreateInfo.ParentCreateInfo.Data.GetAddressOf())
        );
        if(FAILED(hr))
        {
            assert(false && "Failed to create vertex buffer");
            return false;
        }
        
        D3D12_RANGE range = { .Begin = 0, .End = 0 };
        byte* vertexDataBegin = nullptr;
        hr = vertexBufferCreateInfo.ParentCreateInfo.Data->Map(
            0,
            &range,
            reinterpret_cast<void**>(&vertexDataBegin)
        );
        if(FAILED(hr))
        {
            assert(false && "Failed to map vertex buffer");
            return false;
        }

        std::memcpy(vertexDataBegin, vertices.data(), sizeof(VertexPN) * vertices.size());
        vertexBufferCreateInfo.ParentCreateInfo.Data->Unmap(0, nullptr);

        vertexBufferCreateInfo.View.BufferLocation = vertexBufferCreateInfo.ParentCreateInfo.Data->GetGPUVirtualAddress();
        vertexBufferCreateInfo.View.StrideInBytes = sizeof(VertexPN);
        vertexBufferCreateInfo.View.SizeInBytes = static_cast<uint32>(vertices.size());

        outVertexBuffer.Initialize(std::move(vertexBufferCreateInfo));
        return true;
    }

    [[nodiscard]] static bool
    CreateIndexBuffer(IndexBuffer& outIndexBuffer, const std::vector<uint16>& indices, const std::string& name)
    {
        HRESULT hr = S_OK;
        const D3D12_HEAP_PROPERTIES bufferHeapProperties = 
        {
            .Type = D3D12_HEAP_TYPE_UPLOAD,
            .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask = 1,
            .VisibleNodeMask = 1,
        };

        D3D12_RESOURCE_DESC bufferDesc =
        {
            .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Alignment = 0,
            .Width = 0,
            .Height = 1,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .Format = DXGI_FORMAT_UNKNOWN,
            .SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 },
            .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            .Flags = D3D12_RESOURCE_FLAG_NONE,
        };

        IndexBuffer::CreateInfo indexBufferCreateInfo;
        indexBufferCreateInfo.ParentCreateInfo.State = D3D12_RESOURCE_STATE_GENERIC_READ;
        indexBufferCreateInfo.ParentCreateInfo.Name = name;
        bufferDesc.Width = sizeof(uint16) * indices.size();
        hr = gDevice->CreateCommittedResource(
            &bufferHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            indexBufferCreateInfo.ParentCreateInfo.State,
            nullptr,
            IID_PPV_ARGS(indexBufferCreateInfo.ParentCreateInfo.Data.GetAddressOf())
        );
        if(FAILED(hr))
        {
            assert(false && "Failed to create index buffer");
            return false;
        }
        
        D3D12_RANGE range = { .Begin = 0, .End = 0 };
        byte* indexDataBegin = nullptr;
        hr = indexBufferCreateInfo.ParentCreateInfo.Data->Map(
            0,
            &range,
            reinterpret_cast<void**>(&indexDataBegin)
        );
        if(FAILED(hr))
        {
            assert(false && "Failed to map index buffer");
            return false;
        }

        std::memcpy(indexDataBegin, indices.data(), sizeof(uint16) * indices.size());
        indexBufferCreateInfo.ParentCreateInfo.Data->Unmap(0, nullptr);

        indexBufferCreateInfo.View.BufferLocation = indexBufferCreateInfo.ParentCreateInfo.Data->GetGPUVirtualAddress();
        indexBufferCreateInfo.View.SizeInBytes = static_cast<uint32>(indices.size());
        indexBufferCreateInfo.View.Format = DXGI_FORMAT_R16_UINT;
        outIndexBuffer.Initialize(std::move(indexBufferCreateInfo));
        return true;
    }

    void
    CreateCornellBoxScene(std::vector<std::unique_ptr<Geometry>>& outGeometries) noexcept
    {
        InitializeCornellBoxCamera();

        outGeometries.clear();
        outGeometries.reserve(8);

        std::vector<VertexPN> vertices;
        std::vector<uint16> indices;
        bool result = false;

        // Floor
        outGeometries.push_back(std::make_unique<Geometry>(std::string("Floor")));
        Geometry& floor = *outGeometries.back();
        {
            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { 0.0f, 0.0f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -552.8f, 0.0f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -549.6f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 0.0f, 0.0f, 559.2f };
            indices.reserve(6);
            AddQuadVertices(vertices, indices, v0, v1, v2, v3);

            VertexBuffer vertexBuffer;
            result = CreateVertexBuffer(vertexBuffer, vertices, "FloorVertexBuffer");
            floor.SetVertexBuffer(std::move(vertexBuffer));
                
            IndexBuffer indexBuffer;
            result = CreateIndexBuffer(indexBuffer, indices, "FloorIndexBuffer");
            floor.SetIndexBuffer(std::move(indexBuffer));
            // floor.SetColor(WHITE);
        }

        if (result == false)
        {
            assert(false && "Failed to create vertex buffer");
            outGeometries.pop_back();
        }
        
        // Light
        outGeometries.push_back(std::make_unique<Geometry>(std::string("Light")));
        Geometry& light = *outGeometries.back();
        {
            vertices.clear();
            indices.clear();

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { -343.0f, 548.8f, 332.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -343.0f, 548.8f, 227.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -213.0f, 548.8f, 227.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { -213.0f, 548.8f, 332.0f };
            AddQuadVertices(vertices, indices, v0, v1, v2, v3);

            VertexBuffer vertexBuffer;
            result = CreateVertexBuffer(vertexBuffer, vertices, "LightVertexBuffer");
            light.SetVertexBuffer(std::move(vertexBuffer));

            IndexBuffer indexBuffer;
            result = CreateIndexBuffer(indexBuffer, indices, "LightIndexBuffer");
            light.SetIndexBuffer(std::move(indexBuffer));

            // light.SetColor(WHITE);
            light.SetIsEmissive(true);
        }

        if (result == false)
        {
            assert(false && "Failed to create vertex buffer");
            outGeometries.pop_back();
        }

        // Ceiling
        outGeometries.push_back(std::make_unique<Geometry>(std::string("Ceiling")));
        Geometry& ceiling = *outGeometries.back();
        {
            vertices.clear();
            indices.clear();

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { -556.0f, 548.8f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -556.0f, 548.8f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { 0.0f, 548.8f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 0.0f, 548.8f, 559.2f };
            AddQuadVertices(vertices, indices, v0, v1, v2, v3);

            VertexBuffer vertexBuffer;
            result = CreateVertexBuffer(vertexBuffer, vertices, "CeilingVertexBuffer");
            ceiling.SetVertexBuffer(std::move(vertexBuffer));

            IndexBuffer indexBuffer;
            result = CreateIndexBuffer(indexBuffer, indices, "CeilingIndexBuffer");
            ceiling.SetIndexBuffer(std::move(indexBuffer));

            // ceiling.SetColor(WHITE);
        }

        if (result == false)
        {
            assert(false && "Failed to create vertex buffer");
            outGeometries.pop_back();
        }
        
        // Back wall
        outGeometries.push_back(std::make_unique<Geometry>(std::string("Back Wall")));
        Geometry& backWall = *outGeometries.back();
        {
            vertices.clear();
            indices.clear();

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { 0.0f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -549.6f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -556.0f, 548.8f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 0.0f, 548.8f, 559.2f };
            AddQuadVertices(vertices, indices, v0, v1, v2, v3);

            VertexBuffer vertexBuffer;
            result = CreateVertexBuffer(vertexBuffer, vertices, "BackWallVertexBuffer");
            backWall.SetVertexBuffer(std::move(vertexBuffer));

            IndexBuffer indexBuffer;
            result = CreateIndexBuffer(indexBuffer, indices, "BackWallIndexBuffer");
            backWall.SetIndexBuffer(std::move(indexBuffer));

            // backWall.SetColor(WHITE);
        }

        if (result == false)
        {
            assert(false && "Failed to create vertex buffer");
            outGeometries.pop_back();
        }

        // Right wall
        outGeometries.push_back(std::make_unique<Geometry>(std::string("Right Wall")));
        Geometry& rightWall = *outGeometries.back();
        {
            vertices.clear();
            indices.clear();

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { 0.0f, 0.0f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { 0.0f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { 0.0f, 548.8f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 0.0f, 548.8f, 0.0f };
            AddQuadVertices(vertices, indices, v0, v1, v2, v3);

            VertexBuffer vertexBuffer;
            result = CreateVertexBuffer(vertexBuffer, vertices, "RightWallVertexBuffer");
            rightWall.SetVertexBuffer(std::move(vertexBuffer));

            IndexBuffer indexBuffer;
            result = CreateIndexBuffer(indexBuffer, indices, "RightWallIndexBuffer");
            rightWall.SetIndexBuffer(std::move(indexBuffer));

            // rightWall.SetColor(GREEN);
        }

        if (result == false)
        {
            assert(false && "Failed to create vertex buffer");
            outGeometries.pop_back();
        }

        // Left wall
        outGeometries.push_back(std::make_unique<Geometry>(std::string("Left Wall")));
        Geometry& leftWall = *outGeometries.back();
        {
            vertices.clear();
            indices.clear();

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { -549.6f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -552.8f, 0.0f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -556.0f, 548.8f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { -556.0f, 548.8f, 559.2f };
            AddQuadVertices(vertices, indices, v0, v1, v2, v3);

            VertexBuffer vertexBuffer;
            result = CreateVertexBuffer(vertexBuffer, vertices, "LeftWallVertexBuffer");
            leftWall.SetVertexBuffer(std::move(vertexBuffer));

            IndexBuffer indexBuffer;
            result = CreateIndexBuffer(indexBuffer, indices, "LeftWallIndexBuffer");
            leftWall.SetIndexBuffer(std::move(indexBuffer));

            // leftWall.SetColor(RED);
        }

        if (result == false)
        {
            assert(false && "Failed to create vertex buffer");
            outGeometries.pop_back();
        }

        // Short block
        outGeometries.push_back(std::make_unique<Geometry>(std::string("Short Block")));
        Geometry& shortBlock = *outGeometries.back();
        {
            vertices.clear();
            indices.clear();
            indices.reserve(6 * 5);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { -82.0f, 165.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -130.0f, 165.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -290.0f, 165.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { -240.0f, 165.0f, 272.0f };
            AddQuadVertices(vertices, indices, v0, v1, v2, v3);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v4 = { -290.0f, 165.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v5 = { -290.0f, 0.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v6 = { -240.0f, 0.0f, 272.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v7 = { -240.0f, 165.0f, 272.0f };
            AddQuadVertices(vertices, indices, v4, v5, v6, v7);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v8 = { -130.0f, 165.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v9 = { -130.0f, 0.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v10 = { -290.0f, 0.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v11 = { -290.0f, 165.0f, 114.0f };
            AddQuadVertices(vertices, indices, v8, v9, v10, v11);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v12 = { -82.0f, 165.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v13 = { -82.0f, 0.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v14 = { -130.0f, 0.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v15 = { -130.0f, 165.0f, 65.0f };
            AddQuadVertices(vertices, indices, v12, v13, v14, v15);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v16 = { -240.0f, 165.0f, 272.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v17 = { -240.0f, 0.0f, 272.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v18 = { -82.0f, 0.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v19 = { -82.0f, 165.0f, 225.0f };
            AddQuadVertices(vertices, indices, v16, v17, v18, v19);

            VertexBuffer vertexBuffer;
            result = CreateVertexBuffer(vertexBuffer, vertices, "ShortBlockVertexBuffer");
            shortBlock.SetVertexBuffer(std::move(vertexBuffer));

            IndexBuffer indexBuffer;
            result = CreateIndexBuffer(indexBuffer, indices, "ShortBlockIndexBuffer");
            shortBlock.SetIndexBuffer(std::move(indexBuffer));

            // shortBlock.SetColor(WHITE);
        }

        if (result == false)
        {
            assert(false && "Failed to create vertex buffer");
            outGeometries.pop_back();
        }

        // Tall block
        outGeometries.push_back(std::make_unique<Geometry>(std::string("Tall Block")));
        Geometry& tallBlock = *outGeometries.back();
        {
            vertices.clear();
            indices.clear();

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { -265.0f, 330.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -423.0f, 330.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -472.0f, 330.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { -314.0f, 330.0f, 456.0f };
            AddQuadVertices(vertices, indices, v0, v1, v2, v3);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v4 = { -423.0f, 330.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v5 = { -423.0f, 0.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v6 = { -472.0f, 0.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v7 = { -472.0f, 330.0f, 406.0f };
            AddQuadVertices(vertices, indices, v4, v5, v6, v7);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v8 = { -472.0f, 330.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v9 = { -472.0f, 0.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v10 = { -314.0f, 0.0f, 456.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v11 = { -314.0f, 330.0f, 456.0f };
            AddQuadVertices(vertices, indices, v8, v9, v10, v11);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v12 = { -314.0f, 330.0f, 456.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v13 = { -314.0f, 0.0f, 456.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v14 = { -265.0f, 0.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v15 = { -265.0f, 330.0f, 296.0f };
            AddQuadVertices(vertices, indices, v12, v13, v14, v15);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v16 = { -265.0f, 330.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v17 = { -265.0f, 0.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v18 = { -423.0f, 0.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v19 = { -423.0f, 330.0f, 247.0f };
            AddQuadVertices(vertices, indices, v16, v17, v18, v19);

            VertexBuffer vertexBuffer;
            result = CreateVertexBuffer(vertexBuffer, vertices, "TallBlockVertexBuffer");
            tallBlock.SetVertexBuffer(std::move(vertexBuffer));

            IndexBuffer indexBuffer;
            result = CreateIndexBuffer(indexBuffer, indices, "TallBlockIndexBuffer");
            tallBlock.SetIndexBuffer(std::move(indexBuffer));

            // tallBlock.SetColor(WHITE);
        }

        if (result == false)
        {
            assert(false && "Failed to create vertex buffer");
            outGeometries.pop_back();
        }
    }
    
    void
    Render(uint64& inoutWorkIndex, RenderThreadInfo& inoutRenderThreadInfo, const std::vector<std::unique_ptr<Geometry>>& geometries) noexcept
    {
        // assert(false && "Render function not implemented");
        
        const uint32 currentFrameIndexToRender = gSwapChain->GetCurrentBackBufferIndex();

        const std::lock_guard lock(inoutRenderThreadInfo.RenderWorksMutex);
        inoutRenderThreadInfo.RenderWorksPerFrame.push(
            cgs::RenderWork
            {
                .Geometries = geometries,
                .WorkIndex = inoutWorkIndex++,
                .FrameIndex = currentFrameIndexToRender,
            });
    }

    void
    RenderThreadMain(ThreadProcessArgument& arg) noexcept
    {
        if (arg.Argument == nullptr)
        {
            assert(false && "RenderInfo argument is null");
            return;
        }

        RenderThreadInfo& renderThreadInfo = *static_cast<RenderThreadInfo*>(arg.Argument);
        while (renderThreadInfo.IsActive.load())
        {
            std::unique_lock<std::mutex> uniqueLock(renderThreadInfo.RenderWorksMutex, std::defer_lock);
            uniqueLock.lock();
            if (renderThreadInfo.RenderWorksPerFrame.empty() == false)
            {
                HRESULT hr = S_OK;
                RenderWork renderWork = std::move(renderThreadInfo.RenderWorksPerFrame.front());
                renderThreadInfo.RenderWorksPerFrame.pop();
                uniqueLock.unlock();

                // Process the render work
                // assert(false && "Render work not implemented");
#if 0
                renderWork.OutTexture.Clear();
                renderWork.OutDepthBuffer.Clear(std::numeric_limits<float>::max());
                if (renderThreadInfo.RenderMethod == eRenderMethod::RASTERIZATION)
                {
                    renderThreadInfo.CurrentWorkIndex.store(renderWork.WorkIndex);
                    Rasterize(renderWork);
                    renderThreadInfo.LastCompleteWorkIndex.store(renderWork.WorkIndex);
                    // std::cout << "RenderWork completed: " << renderWork.WorkIndex << std::endl;
                }
                else
                {
                    assert(false && "Unsupported render method in RenderThreadMain");
                }
#else
                renderThreadInfo.CurrentWorkIndex.store(renderWork.WorkIndex);
                // TODO(alegruz): DX ERROR: ID3D12CommandAllocator::Reset: A command allocator 0x0000017DF4B5BBA0:'Unnamed ID3D12CommandAllocator Object' is being reset before previous executions associated with the allocator have completed. [ EXECUTION ERROR #552: ]
                hr = gGraphicsCommandAllocator->Reset();
                if(FAILED(hr))
                {
                    assert(false && "Failed to reset command allocator");
                    renderThreadInfo.LastCompleteWorkIndex.store(renderWork.WorkIndex);
                    continue;
                }

                hr = gGraphicsCommandList->Reset(gGraphicsCommandAllocator.Get(), nullptr);
                if(FAILED(hr))
                {
                    assert(false && "Failed to reset command list");
                    renderThreadInfo.LastCompleteWorkIndex.store(renderWork.WorkIndex);
                    continue;
                }

                gSceneRenderTargets[renderWork.FrameIndex].ColorBuffer.Transition(*gGraphicsCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

                constexpr float BLACK_COLOR[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
                gGraphicsCommandList->ClearRenderTargetView(
                    gSceneRenderTargets[renderWork.FrameIndex].ColorBuffer.GetView(),
                    BLACK_COLOR,
                    0,
                    nullptr
                );
                gGraphicsCommandList->ClearDepthStencilView(
                    gSceneRenderTargets[renderWork.FrameIndex].DepthBuffer.GetView(),
                    D3D12_CLEAR_FLAG_DEPTH,
                    1.0f,
                    0,
                    0,
                    nullptr
                );

                gSceneRenderTargets[renderWork.FrameIndex].ColorBuffer.Transition(*gGraphicsCommandList, D3D12_RESOURCE_STATE_PRESENT);

                gGraphicsCommandList->Close();
                ID3D12CommandList* commandLists[] = { gGraphicsCommandList.Get(), };
                gGraphicsCommandQueue->ExecuteCommandLists(CGS_ARRAYSIZE(commandLists), commandLists);
                gSwapChain->Present(0, 0);
                renderThreadInfo.LastCompleteWorkIndex.store(renderWork.WorkIndex);
#endif
            }
            else
            {
                uniqueLock.unlock();
            }
        }
    }

    bool
    InitializeRenderer(const RendererCreateInfo& createInfo) noexcept
    {
        HRESULT hr = S_OK;

#if defined(CGS_DEBUG)
        HMODULE dxgiDebugModule = LoadLibrary(TEXT("dxgidebug.dll"));
        TDXGIGetDebugInterface DXGIGetDebugInterface = reinterpret_cast<TDXGIGetDebugInterface>(GetProcAddress(dxgiDebugModule, "DXGIGetDebugInterface"));

        hr = DXGIGetDebugInterface(IID_PPV_ARGS(gDxgiDebug.GetAddressOf()));
        if(FAILED(hr))
        {
            assert(false && "Failed to get DXGI Debug Interface");
            return false;
        }
        
        hr = DXGIGetDebugInterface(IID_PPV_ARGS(gInfoQueue.GetAddressOf()));
        if(FAILED(hr))
        {
            assert(false && "Failed to get DXGI Debug Interface");
            return false;
        }
#endif  // defined(CGS_DEBUG)

#if defined(CGS_DEBUG)
        UINT createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#else   // NOT defined(CGS_DEBUG)
        UINT createFactoryFlags = 0;
#endif  // NOT defined(CGS_DEBUG)
        {
            IDXGIFactory2* factory2 = nullptr;
            hr = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&factory2));
            if(FAILED(hr))
            {
                assert(false && "Failed to create DXGI Factory");
                return false;
            }

            hr = factory2->QueryInterface(IID_PPV_ARGS(gFactory.GetAddressOf()));
            factory2->Release();
        }

        for(UINT adapterIndex = 0; ; ++adapterIndex)
        {
            D3DPtr<IDXGIAdapter> adapter;
            constexpr DXGI_GPU_PREFERENCE GPU_PREFERENCE = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            hr = gFactory->EnumAdapterByGpuPreference(adapterIndex, GPU_PREFERENCE, IID_PPV_ARGS(adapter.GetAddressOf()));
            if(FAILED(hr))
            {
                break;
            }

            // Check if the adapter supports D3D12
            hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_2, _uuidof(ID3D12Device), nullptr);
            if(SUCCEEDED(hr))
            {
                gAdapter = adapter;
                break;
            }
        }

#if defined(CGS_DEBUG)
        {
            D3DPtr<ID3D12Debug> d3D12Debug;
            hr = D3D12GetDebugInterface(IID_PPV_ARGS(d3D12Debug.GetAddressOf()));
            if(FAILED(hr))
            {
                assert(false && "Failed to get D3D12 Debug Interface");
                return false;
            }

            hr = d3D12Debug->QueryInterface(IID_PPV_ARGS(gD3D12Debug.GetAddressOf()));
            if(FAILED(hr))
            {
                assert(false && "Failed to get D3D12 Debug Interface");
                return false;
            }
        }

        gD3D12Debug->EnableDebugLayer();
        gD3D12Debug->SetEnableGPUBasedValidation(TRUE);
        gD3D12Debug->SetEnableSynchronizedCommandQueueValidation(TRUE);
        gD3D12Debug->SetEnableAutoName(TRUE);

#endif  // defined(CGS_DEBUG)

        {
            D3DPtr<ID3D12Device> device;
            hr = D3D12CreateDevice(gAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(device.GetAddressOf()));
            if (FAILED(hr))
            {
                assert(false && "Failed to create D3D12 Device");
                return false;
            }

            hr = device->QueryInterface(IID_PPV_ARGS(gDevice.GetAddressOf()));
            if (FAILED(hr))
            {
                assert(false && "Failed to get D3D12 Device");
                return false;
            }
        }

#if 0
        hr = gDevice->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(gGraphicsCommandAllocator.GetAddressOf())
        );
        if(FAILED(hr))
        {
            assert(false && "Failed to create command allocator");
            return false;
        }
#endif

        const D3D12_COMMAND_QUEUE_DESC commandQueueDesc = 
        {
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
            .Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
            .NodeMask = 0,
        };
        hr = gDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(gGraphicsCommandQueue.GetAddressOf()));
        if (FAILED(hr))
        {
            assert(false && "Failed to create command queue");
            return false;
        }

        hr = gDevice->CreateCommandList1(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            D3D12_COMMAND_LIST_FLAG_NONE,
            IID_PPV_ARGS(gGraphicsCommandList.GetAddressOf())
        );
        if (FAILED(hr))
        {
            assert(false && "Failed to create command list");
            return false;
        }

        const D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc =
        {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = BACK_BUFFERS_COUNT,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            .NodeMask = 0,
        };
        hr = gDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(gRtvHeap.GetAddressOf()));
        if (FAILED(hr))
        {
            assert(false && "Failed to create RTV descriptor heap");
            return false;
        }

        const D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc =
        {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            .NumDescriptors = BACK_BUFFERS_COUNT,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            .NodeMask = 0,
        };
        hr = gDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(gDsvHeap.GetAddressOf()));
        if (FAILED(hr))
        {
            assert(false && "Failed to create DSV descriptor heap");
            return false;
        }

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = 
        {
            .Width = createInfo.Width,
            .Height = createInfo.Height,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .Stereo = FALSE,
            .SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 },
            .BufferUsage = DXGI_USAGE_BACK_BUFFER,
            .BufferCount = BACK_BUFFERS_COUNT,
            .Scaling = DXGI_SCALING_STRETCH,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
            .Flags = 0,
        };
        {
            D3DPtr<IDXGISwapChain1> swapChain1;
            hr = gFactory->CreateSwapChainForHwnd(gGraphicsCommandQueue.Get(), createInfo.Window, &swapChainDesc, nullptr, nullptr, swapChain1.GetAddressOf());
            if (FAILED(hr))
            {
                assert(false && "Failed to create swap chain");
                return false;
            }

            hr = swapChain1->QueryInterface(IID_PPV_ARGS(gSwapChain.GetAddressOf()));
            if (FAILED(hr))
            {
                assert(false && "Failed to query swap chain interface");
                return false;
            }
        }

        gSwapChain->GetDesc1(&swapChainDesc);
        if (swapChainDesc.BufferCount != BACK_BUFFERS_COUNT)
        {
            assert(false && "Unexpected swap chain buffer count");
            return false;
        }

        const D3D12_RENDER_TARGET_VIEW_DESC colorBufferViewDesc =
        {
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
            .Texture2D = { .MipSlice = 0 },
        };

        const D3D12_RESOURCE_DESC depthBufferDesc =
        {
            .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Alignment = 0,
            .Width = swapChainDesc.Width,
            .Height = swapChainDesc.Height,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
            .SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 },
            .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
        };

        const D3D12_CLEAR_VALUE depthBufferClearValue =
        {
            .Format = depthBufferDesc.Format,
            .DepthStencil = { .Depth = 1.0f, .Stencil = 0 }
        };

        const D3D12_HEAP_PROPERTIES depthBufferHeapProperties = 
        {
            .Type = D3D12_HEAP_TYPE_DEFAULT,
            .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask = 1,
            .VisibleNodeMask = 1,
        };

        gRtvIncrementSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        gDsvIncrementSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

        const D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = gRtvHeap->GetCPUDescriptorHandleForHeapStart();
        const D3D12_CPU_DESCRIPTOR_HANDLE dsvStartHandle = gDsvHeap->GetCPUDescriptorHandleForHeapStart();

        for(uint32 frameIndex = 0; frameIndex < BACK_BUFFERS_COUNT; ++frameIndex)
        {
            Texture::CreateInfo colorBufferInfo;
            hr = gSwapChain->GetBuffer(frameIndex, IID_PPV_ARGS(colorBufferInfo.Data.GetAddressOf()));
            if(FAILED(hr))
            {
                assert(false && "Failed to get swap chain buffer");
                return false;
            }

            colorBufferInfo.View.ptr = rtvStartHandle.ptr + (frameIndex * gRtvIncrementSize);
            colorBufferInfo.State = D3D12_RESOURCE_STATE_COMMON;    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12#initial-states-for-resources
            colorBufferInfo.Name = "SwapChainColorBuffer[" + std::to_string(frameIndex) + "]";
            gDevice->CreateRenderTargetView(colorBufferInfo.Data.Get(), &colorBufferViewDesc, colorBufferInfo.View);

            gSceneRenderTargets[frameIndex].ColorBuffer.Initialize(std::move(colorBufferInfo));

            Texture::CreateInfo depthBufferInfo;
            depthBufferInfo.State = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            depthBufferInfo.Name = "SwapChainDepthBuffer[" + std::to_string(frameIndex) + "]";
            hr = gDevice->CreateCommittedResource(
                &depthBufferHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &depthBufferDesc,
                depthBufferInfo.State,
                &depthBufferClearValue,
                IID_PPV_ARGS(depthBufferInfo.Data.GetAddressOf())
            );
            if (FAILED(hr))
            {
                assert(false && "Failed to create depth buffer");
                return false;
            }

            depthBufferInfo.View.ptr = dsvStartHandle.ptr + (frameIndex * gDsvIncrementSize);

            gDevice->CreateDepthStencilView(depthBufferInfo.Data.Get(), nullptr, depthBufferInfo.View);

            gSceneRenderTargets[frameIndex].DepthBuffer.Initialize(std::move(depthBufferInfo));
        }

        hr = gDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(gGraphicsCommandAllocator.GetAddressOf()));
        if (FAILED(hr))
        {
            assert(false && "Failed to create command allocator");
            return false;
        }

        gGlobalRenderContext.RenderDeviceType = eRenderDeviceType::D3D12;
        return true;
    }
}
#endif  // defined(CGS_GRAPHICS_API_D3D12)