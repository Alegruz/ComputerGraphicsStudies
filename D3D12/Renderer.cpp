#include "pch.hpp"

#include "Common/Renderer.cpp"

#if defined(CGS_GRAPHICS_API_D3D12)
#include "D3D12/Renderer.h"

namespace cgs
{
    static GlobalRenderContext gGlobalRenderContext;

    using TDXGIGetDebugInterface = HRESULT (*)(REFIID, void**);

    struct ASBuildInfo final
    {
        D3DPtr<ID3D12Resource>                              ScratchResource = D3DPtr<ID3D12Resource>();
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS BottomLevelInputs = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS TopLevelInputs = {};
        D3DPtr<ID3D12Resource>                              InstanceDescs = D3DPtr<ID3D12Resource>();
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC  BottomLevelBuildDesc = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC  TopLevelBuildDesc = {};
    };
    
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

    struct SceneConstantBuffer final
    {
        float4x4 ProjectionToWorldTransformMatrix;
        float4 CameraPosition;
        uint32 ParallelogramAreaLightInfosCount;
        uint32 PointLightInfosCount;
    };

    struct ParallelogramAreaLightInfo final
    {
        float3 Positions[3];
        float3 Color;
        uint32 PrimitiveIndices[2];
    };

    struct PointLightInfo final
    {
        float3 Positions;
        float3 Color;
        float Radius;
    };

    struct ParallelogramAreaLightReservoir final
    {
        float3 ParallelogramAreaLightPositionSample;
        float Pdf;
        float3 ParallelogramAreaLightNormalSample;
        float WeightSum;

        float Weight;
        uint32 SamplesCount;
    };

    struct PointLightReservoir final
    {
        float Weight;
        uint32 LightIndexSample;

        float WeightSum;
        float Pdf;

        uint32 SamplesCount;
    };

    struct IndirectLightReservoir final
    {
        float3 Direction;
        float Pdf;

        float Weight;
        float WeightSum;

        uint32 SamplesCount;
    };

    struct GBufferRayPayload final
    {
        float4 Color;
        float RayT;
        uint32 PrimitiveIndex;
        uint32 Depth;
    };

    class DescriptorHeap final
    {
    public:
        using Type = uint8;
        enum class eTypeBit : uint8
        {
            INVALID             = 0x00,
            RENDER_TARGET       = 0x01,
            DEPTH               = 0x02,
            STENCIL             = 0x04,
            DEPTH_STENCIL       = 0x06,
            CONSTANT_BUFFER     = 0x08,
            SHADER_RESOURCE     = 0x10,
            UNORDERED_ACCESS    = 0x20,
            CONSTANT_BUFFER_SHADER_RESOURCE_UNORDERED_ACCESS = 0x38,
        };

        struct CreateInfo final
        {
            D3DPtr<ID3D12DescriptorHeap> Heap;
            Type HeapType;
            uint32 DescriptorSize = 0;
            uint32 MaxStaticDescriptorsCount = 0;
            uint32 MaxDynamicDescriptorsCount = 0;
            uint32 DynamicBlocksCount = 0;
        };

    public:
        CGS_INLINE constexpr
        DescriptorHeap() noexcept
            : mHeap()
            , mType(0)
            , mDescriptorSize(0)
            , mMaxStaticDescriptorsCount(0)
            , mMaxDynamicDescriptorsCount(0)
            , mDynamicBlocksCount(0)
            , mMaxDescriptorsCount(0)
            , mIsDescriptorAllocated()
        {}

        CGS_INLINE constexpr
        DescriptorHeap(CreateInfo&& createInfo) noexcept
            : mHeap(std::move(createInfo.Heap))
            , mType(createInfo.HeapType)
            , mDescriptorSize(createInfo.DescriptorSize)
            , mMaxStaticDescriptorsCount(createInfo.MaxStaticDescriptorsCount)
            , mMaxDynamicDescriptorsCount(createInfo.MaxDynamicDescriptorsCount)
            , mDynamicBlocksCount(createInfo.DynamicBlocksCount)
            , mMaxDescriptorsCount(createInfo.MaxStaticDescriptorsCount + createInfo.MaxDynamicDescriptorsCount * createInfo.DynamicBlocksCount)
            , mIsDescriptorAllocated(mMaxDescriptorsCount)
        {
            assert(mDescriptorSize > 0 && "Invalid descriptor size");   
        }

        CGS_INLINE
        ~DescriptorHeap() noexcept
        {
            DestroyD3D12Object(mHeap);
        }

        [[nodiscard]] CGS_INLINE constexpr const D3DPtr<ID3D12DescriptorHeap>&
        GetHeap() const noexcept
        {
            return mHeap;
        }

        [[nodiscard]] CGS_INLINE constexpr D3DPtr<ID3D12DescriptorHeap>&
        GetHeap() noexcept
        {
            return mHeap;
        }

        [[nodiscard]] CGS_INLINE constexpr Type
        GetType() const noexcept
        {
            return mType;
        }

        CGS_INLINE constexpr void
        Initialize(CreateInfo&& createInfo) noexcept
        {
            mHeap = std::move(createInfo.Heap);
            mType = createInfo.HeapType;
            mDescriptorSize = createInfo.DescriptorSize;
            mMaxStaticDescriptorsCount = createInfo.MaxStaticDescriptorsCount;
            mMaxDynamicDescriptorsCount = createInfo.MaxDynamicDescriptorsCount;
            mDynamicBlocksCount = createInfo.DynamicBlocksCount;
            mMaxDescriptorsCount = createInfo.MaxStaticDescriptorsCount + createInfo.MaxDynamicDescriptorsCount * createInfo.DynamicBlocksCount;
            mIsDescriptorAllocated.resize(mMaxDescriptorsCount, false);
        }

        [[nodiscard]] CGS_INLINE constexpr bool
        AllocateStaticDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& outCpuDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE& outGpuDescriptor, const uint32 index) noexcept
        {
            std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> cpuDescriptor{ outCpuDescriptor };
            std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> gpuDescriptor{ outGpuDescriptor };
            const bool result = allocate(cpuDescriptor, gpuDescriptor, index, true);
            outCpuDescriptor = cpuDescriptor.back();
            outGpuDescriptor = gpuDescriptor.back();
            return result;
        }

        [[nodiscard]] CGS_INLINE constexpr bool
        AllocateDynamicDescriptors(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& outCpuDescriptors, std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>& outGpuDescriptors, const uint32 index) noexcept
        {
            if(outCpuDescriptors.size() != BACK_BUFFERS_COUNT || outGpuDescriptors.size() != BACK_BUFFERS_COUNT)
            {
                assert(false && "Number of dynamic descriptors requested must be equal to the number of frame buffers!");
                return false;
            }

            return allocate(outCpuDescriptors, outGpuDescriptors, index, false);
        }

        [[nodiscard]] CGS_INLINE constexpr bool
        AllocateDynamicDescriptors(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& outCpuDescriptors, const uint32 index) noexcept
        {
            if(outCpuDescriptors.size() != BACK_BUFFERS_COUNT)
            {
                assert(false && "Number of dynamic descriptors requested must be equal to the number of frame buffers!");
                return false;
            }

            return allocate(outCpuDescriptors, index, false);
        }

    private:
        [[nodiscard]] CGS_INLINE constexpr bool
        allocate(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& outCpuDescriptors, std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>& outGpuDescriptors, const uint32 index, const bool isStatic) noexcept
        {
            const uint32 descriptorsCount = static_cast<uint32>(outCpuDescriptors.size());
            if(descriptorsCount != outGpuDescriptors.size())
            {
                assert(false && "Number of cpu and gpu descriptors requested must be equal!!");
                return false;
            }

            if(index >= mMaxDescriptorsCount)
            {
                assert(false && "Index exceeds maximum number of descriptors allocatable");
                return false;
            }

            uint32 descriptorIndex = isStatic ? index : (index + mMaxStaticDescriptorsCount);
            for(uint32 i = 0; i < descriptorsCount; ++i, descriptorIndex += mMaxDynamicDescriptorsCount)
            {
                const bool isAllocated = mIsDescriptorAllocated[descriptorIndex];
                if(isAllocated == true)
                {
                    assert(false && "There already is a descriptor allocated here!");
                    return false;
                }

                outCpuDescriptors[i] = mHeap->GetCPUDescriptorHandleForHeapStart();
                outCpuDescriptors[i].ptr += mDescriptorSize * descriptorIndex;
                outGpuDescriptors[i] = mHeap->GetGPUDescriptorHandleForHeapStart();
                outGpuDescriptors[i].ptr += mDescriptorSize * descriptorIndex;

                mIsDescriptorAllocated[descriptorIndex] = true;
            }
            return true;
        }
        [[nodiscard]] CGS_INLINE constexpr bool
        allocate(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& outCpuDescriptors, const uint32 index, const bool isStatic) noexcept
        {
            const uint32 descriptorsCount = static_cast<uint32>(outCpuDescriptors.size());
            if(descriptorsCount != outCpuDescriptors.size())
            {
                assert(false && "Number of cpu descriptors requested must be equal!!");
                return false;
            }

            if(index >= mMaxDescriptorsCount)
            {
                assert(false && "Index exceeds maximum number of descriptors allocatable");
                return false;
            }

            uint32 descriptorIndex = isStatic ? index : (index + mMaxStaticDescriptorsCount);
            for(uint32 i = 0; i < descriptorsCount; ++i, descriptorIndex += mMaxDynamicDescriptorsCount)
            {
                const bool isAllocated = mIsDescriptorAllocated[descriptorIndex];
                if(isAllocated == true)
                {
                    assert(false && "There already is a descriptor allocated here!");
                    return false;
                }

                outCpuDescriptors[i] = mHeap->GetCPUDescriptorHandleForHeapStart();
                outCpuDescriptors[i].ptr += mDescriptorSize * descriptorIndex;

                mIsDescriptorAllocated[descriptorIndex] = true;
            }
            return true;
        }

    private:
        D3DPtr<ID3D12DescriptorHeap> mHeap;
        Type mType;
        uint32 mDescriptorSize;
        uint32 mMaxStaticDescriptorsCount;
        uint32 mMaxDynamicDescriptorsCount;
        uint32 mDynamicBlocksCount;
        uint32 mMaxDescriptorsCount;
        std::vector<bool> mIsDescriptorAllocated;
    };

    static CGS_INLINE DescriptorHeap::Type
    operator|(const DescriptorHeap::Type type, const DescriptorHeap::eTypeBit typeBit) noexcept
    {
        return type | static_cast<uint8>(typeBit);
    }

    static CGS_INLINE DescriptorHeap::Type
    operator&(const DescriptorHeap::Type type, const DescriptorHeap::eTypeBit typeBit) noexcept
    {
        return type & static_cast<uint8>(typeBit);
    }

    static CGS_INLINE DescriptorHeap::Type&
    operator|=(DescriptorHeap::Type& type, const DescriptorHeap::eTypeBit typeBit) noexcept
    {
        type = type | static_cast<uint8>(typeBit);
        return type;
    }

    static CGS_INLINE DescriptorHeap::Type
    operator&=(DescriptorHeap::Type& type, const DescriptorHeap::eTypeBit typeBit) noexcept
    {
        type = type & static_cast<uint8>(typeBit);
        return type;
    }

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
    static D3DPtr<ID3D12InfoQueue1> gD3D12InfoQueue;
#endif  // defined(CGS_DEBUG)
    static D3DPtr<ID3D12Device5> gDevice;
    static std::vector<D3DPtr<ID3D12GraphicsCommandList4>> gGraphicsCommandLists(BACK_BUFFERS_COUNT);
    static std::vector<D3DPtr<ID3D12CommandAllocator>> gGraphicsCommandAllocators(BACK_BUFFERS_COUNT);
    static D3DPtr<ID3D12CommandQueue> gGraphicsCommandQueue;
    static D3DPtr<ID3D12DescriptorHeap> gRtvHeap;
    static D3DPtr<ID3D12DescriptorHeap> gDsvHeap;
    static constexpr uint32 GLOBAL_CBV_SRV_UAV_HEAP_START_INDEX = 0;
    static constexpr uint32 GLOBAL_CBV_SRV_UAV_DESCRIPTORS_COUNT = 1024;
    static constexpr uint32 PER_FRAME_CBV_SRV_UAV_HEAP_START_INDEX = GLOBAL_CBV_SRV_UAV_DESCRIPTORS_COUNT;
    static constexpr uint32 PER_FRAME_CBV_SRV_UAV_DESCRIPTORS_COUNT = 1024;
    static constexpr uint32 MAX_DESCRIPTORS_COUNT = GLOBAL_CBV_SRV_UAV_DESCRIPTORS_COUNT + PER_FRAME_CBV_SRV_UAV_DESCRIPTORS_COUNT * BACK_BUFFERS_COUNT;
    static std::unique_ptr<DescriptorHeap> gCbvSrvUavHeap;
    static std::unique_ptr<DescriptorHeap> gCbvSrvUavHeapOnlyCpu;
    static uint32 gRtvIncrementSize = 0;
    static uint32 gDsvIncrementSize = 0;
    static std::vector<SceneRenderTarget> gSceneRenderTargets(BACK_BUFFERS_COUNT);
    static HANDLE gFenceEvent = NULL;
    static D3DPtr<ID3D12Fence> gFence;
    static std::vector<uint64> gFenceValues(BACK_BUFFERS_COUNT, std::numeric_limits<uint64>::max());
    
    static Slang::ComPtr<slang::IGlobalSession> gSlangGlobalSession;
    static Slang::ComPtr<slang::IBlob> gVsShader;
    static Slang::ComPtr<slang::IBlob> gFsShader;
    static D3DPtr<ID3D12RootSignature> gRasterizationRootSignature;
    static D3DPtr<ID3D12PipelineState> gRasterizationPipelineState;
    static std::vector<std::unique_ptr<ConstantBuffer>> gCameraBuffers(BACK_BUFFERS_COUNT);
    static std::vector<std::unique_ptr<ConstantBuffer>> gEmissiveBuffers(BACK_BUFFERS_COUNT);

    // Raytracing
    class RaytracingPipeline final
    {
    public:
        struct CreateInfo final
        {
            D3DPtr<ID3D12RootSignature> GlobalRootSignature;
            D3DPtr<ID3D12RootSignature> LocalRootSignature;
            Slang::ComPtr<slang::IBlob> RayGenShader;
            Slang::ComPtr<slang::IBlob> ClosestHitShader;
            Slang::ComPtr<slang::IBlob> MissShader;
            D3DPtr<ID3D12StateObject> StateObject;
            D3DPtr<ID3D12Resource> RayGenShaderTable;
            D3DPtr<ID3D12Resource> MissShaderTable;
            D3DPtr<ID3D12Resource> HitGroupShaderTable;
        };

    public:
        CGS_INLINE constexpr 
        RaytracingPipeline() noexcept = default;
        CGS_INLINE 
        RaytracingPipeline(CreateInfo&& createInfo) noexcept
            : mGlobalRootSignature(std::move(createInfo.GlobalRootSignature))
            , mLocalRootSignature(std::move(createInfo.LocalRootSignature))
            , mRayGenShader(std::move(createInfo.RayGenShader))
            , mClosestHitShader(std::move(createInfo.ClosestHitShader))
            , mMissShader(std::move(createInfo.MissShader))
            , mStateObject(std::move(createInfo.StateObject))
            , mRayGenShaderTable(std::move(createInfo.RayGenShaderTable))
            , mMissShaderTable(std::move(createInfo.MissShaderTable))
            , mHitGroupShaderTable(std::move(createInfo.HitGroupShaderTable))
        {
        }
        CGS_INLINE 
        ~RaytracingPipeline() noexcept
        {
            DestroyD3D12Object(mGlobalRootSignature);
            DestroyD3D12Object(mLocalRootSignature);
            DestroyD3D12Object(mRayGenShaderTable);
            DestroyD3D12Object(mMissShaderTable);
            DestroyD3D12Object(mHitGroupShaderTable);
            DestroyD3D12Object(mStateObject);
        }

        CGS_INLINE const D3DPtr<ID3D12RootSignature>&
        GetGlobalRootSignature() const noexcept { return mGlobalRootSignature; }
        CGS_INLINE D3DPtr<ID3D12RootSignature>&
        GetGlobalRootSignature() noexcept { return mGlobalRootSignature; }
        CGS_INLINE const D3DPtr<ID3D12RootSignature>&
        GetLocalRootSignature() const noexcept { return mLocalRootSignature; }
        CGS_INLINE D3DPtr<ID3D12RootSignature>&
        GetLocalRootSignature() noexcept { return mLocalRootSignature; }
        CGS_INLINE const D3DPtr<ID3D12StateObject>&
        GetStateObject() const noexcept { return mStateObject; }
        CGS_INLINE D3DPtr<ID3D12StateObject>&
        GetStateObject() noexcept { return mStateObject; }
        CGS_INLINE const D3DPtr<ID3D12Resource>&
        GetRayGenShaderTable() const noexcept { return mRayGenShaderTable; }
        CGS_INLINE D3DPtr<ID3D12Resource>&
        GetRayGenShaderTable() noexcept { return mRayGenShaderTable; }
        CGS_INLINE const D3DPtr<ID3D12Resource>&
        GetMissShaderTable() const noexcept { return mMissShaderTable; }
        CGS_INLINE D3DPtr<ID3D12Resource>&
        GetMissShaderTable() noexcept { return mMissShaderTable; }
        CGS_INLINE const D3DPtr<ID3D12Resource>&
        GetHitGroupShaderTable() const noexcept { return mHitGroupShaderTable; }
        CGS_INLINE D3DPtr<ID3D12Resource>&
        GetHitGroupShaderTable() noexcept { return mHitGroupShaderTable; }

    private:
        D3DPtr<ID3D12RootSignature> mGlobalRootSignature;
        D3DPtr<ID3D12RootSignature> mLocalRootSignature;
        Slang::ComPtr<slang::IBlob> mRayGenShader;
        Slang::ComPtr<slang::IBlob> mClosestHitShader;
        Slang::ComPtr<slang::IBlob> mMissShader;
        D3DPtr<ID3D12StateObject> mStateObject;
        D3DPtr<ID3D12Resource> mRayGenShaderTable;
        D3DPtr<ID3D12Resource> mMissShaderTable;
        D3DPtr<ID3D12Resource> mHitGroupShaderTable;
    };
    
    static std::unique_ptr<RaytracingPipeline> gRisPipeline;
    static std::unique_ptr<RaytracingPipeline> gTemporalResamplingPipeline;
    static std::unique_ptr<RaytracingPipeline> gSpatialResamplingPipeline;
    static std::vector<std::unique_ptr<Texture>> gRaytracingOutputs(BACK_BUFFERS_COUNT);
    static std::vector<std::unique_ptr<Texture>> gRaytracingGBuffers(BACK_BUFFERS_COUNT);
    
    static std::vector<std::unique_ptr<RenderResource>> gRaytracingParallelogramAreaLightSampleReservoirs(BACK_BUFFERS_COUNT);
    static std::vector<std::unique_ptr<RenderResource>> gRaytracingPointLightReservoirs(BACK_BUFFERS_COUNT);
    static std::vector<std::unique_ptr<RenderResource>> gRaytracingIndirectLightReservoirs(BACK_BUFFERS_COUNT);
    
    static std::vector<std::unique_ptr<RenderResource>> gRaytracingPrevParallelogramAreaLightSampleReservoirs(BACK_BUFFERS_COUNT);
    static std::vector<std::unique_ptr<RenderResource>> gRaytracingPrevPointLightReservoirs(BACK_BUFFERS_COUNT);
    static std::vector<std::unique_ptr<RenderResource>> gRaytracingPrevIndirectLightReservoirs(BACK_BUFFERS_COUNT);

    static std::vector<std::unique_ptr<RenderResource>> gUploadBuffers(BACK_BUFFERS_COUNT);
    static D3DPtr<ID3D12Resource> gAccelerationStructure;
    static std::vector<D3DPtr<ID3D12Resource>> gBottomLevelAccelerationStructures;
    static std::vector<D3DPtr<ID3D12Resource>> gTopLevelAccelerationStructures;
    static std::vector<std::unique_ptr<ConstantBuffer>> gSceneConstantBuffers(BACK_BUFFERS_COUNT);
    static std::unique_ptr<RenderResource> gParallelogramAreaLightInfosBuffer;
    static std::unique_ptr<RenderResource> gPointLightInfosBuffer;
    static std::vector<PointLightInfo> gPointLightInfos;
    static Coordinate<eCoordinateSpace::WORLD> gRotationAxis;
    
    enum class eCbvSrvUavRaytracingDescriptorType : uint8
    {
        OUTPUT_TEXTURE = 0,
        INDICES,
        VERTICES,
        COLORS,
        IS_EMISSIVES,
        PARALLELOGRAM_AREA_LIGHT_INFOS,
        POINT_LIGHT_INFOS,
        COUNT,
    };

    enum class eCbvSrvUavRasterizationDescriptorType : uint8
    {
        COLORS,
        COUNT,
    };

    enum class ShaderType : uint8
    {
        INVALID = 0,
        VERTEX,
        FRAGMENT,
        RAY_GEN,
        CLOSEST_HIT,
        MISS,
    };

    static void
    addQuadVertices(std::vector<VertexPN>& inoutVertices, std::vector<uint16>& inoutIndices, const Coordinate<eCoordinateSpace::WORLD>& v0, const Coordinate<eCoordinateSpace::WORLD>& v1, const Coordinate<eCoordinateSpace::WORLD>& v2, const Coordinate<eCoordinateSpace::WORLD>& v3);

    [[nodiscard]] static Slang::ComPtr<slang::IBlob>
    compileShader(Slang::ComPtr<slang::IComponentType>& program, const std::filesystem::path& shaderAbsPath, const ShaderType type, const uint32 index) noexcept;

    [[nodiscard]] static bool
    createIndexBuffer(IndexBuffer& outIndexBuffer, const std::vector<uint16>& indices, const std::string& name);

    [[nodiscard]] static bool
    createShaders(const eRenderMethod renderMethod) noexcept;
    
    [[nodiscard]] static bool
    createVertexBuffer(VertexBuffer& outVertexBuffer, const std::vector<VertexPN>& vertices, const std::string& name);

    void
    rasterize(ID3D12GraphicsCommandList& graphicsCommandList, SceneRenderTarget& sceneRenderTarget, RenderWork& renderWork) noexcept;
    
    void
    raytracing(ID3D12GraphicsCommandList4& graphicsCommandList, SceneRenderTarget& sceneRenderTarget, RenderWork& renderWork) noexcept;

    void
    waitForFrame(const uint32 frameIndex) noexcept;

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
    
    bool
    RenderResource::Map(const uint32 size, void* data) noexcept
    {
        D3D12_RANGE range = { .Begin = 0, .End = 0 };
        byte* dataBegin = nullptr;
        HRESULT hr = mData->Map(
            0,
            &range,
            reinterpret_cast<void**>(&dataBegin)
        );
        if(FAILED(hr))
        {
            assert(false && "Failed to map point light infos buffer");
            return false;
        }

        std::memcpy(dataBegin, data, size);
        mData->Unmap(0, nullptr);
        return true;
    }

    Slang::ComPtr<slang::IBlob>
    compileShader(Slang::ComPtr<slang::IComponentType>& program, const std::filesystem::path& shaderAbsPath, const ShaderType type, const uint32 index) noexcept
    {
        std::string shaderName = shaderAbsPath.stem().string() + "_";
        switch(type)
        {
        case ShaderType::VERTEX:
        {
            shaderName += "vs";
        }
        break;
        case ShaderType::FRAGMENT:
        {
            shaderName += "fs";
        }
        break;
        case ShaderType::RAY_GEN:
        {
            shaderName += "rg";
        }
        break;
        case ShaderType::CLOSEST_HIT:
        {
            shaderName += "ch";
        }
        break;
        case ShaderType::MISS:
        {
            shaderName += "m";
        }
        break;
        case ShaderType::INVALID:
            [[fallthrough]];
        default:
            assert(false && "compileShader: invalid shader type");
            return nullptr;
        };
        std::filesystem::path shaderCacheFilePath = shaderAbsPath.parent_path() / (shaderName + ".dxil");

        Slang::ComPtr<slang::IBlob> dxilBlob;
        {
            Slang::ComPtr<slang::IBlob> diagnosticBlob;
            SlangResult result = program->getEntryPointCode(
                index,
                0,
                dxilBlob.writeRef(),
                diagnosticBlob.writeRef()
            );
            if (diagnosticBlob)
            {
                OutputDebugStringA(reinterpret_cast<const char*>(diagnosticBlob->getBufferPointer()));
                OutputDebugStringA("\n");
                DebugBreak();
                return nullptr;
            }

            if(result != SLANG_OK || dxilBlob == nullptr)
            {
                assert(false && "Failed to compile shader");
                return nullptr;
            }
        }

        Slang::ComPtr<slang::IBlob> dxilAsmBlob;
        {
            Slang::ComPtr<slang::IBlob> diagnosticBlob;
            SlangResult result = program->getEntryPointCode(
                index,
                1,
                dxilAsmBlob.writeRef(),
                diagnosticBlob.writeRef()
            );
            if (diagnosticBlob)
            {
                OutputDebugStringA(reinterpret_cast<const char*>(diagnosticBlob->getBufferPointer()));
                OutputDebugStringA("\n");
                DebugBreak();
                return nullptr;
            }

            if(result != SLANG_OK || dxilAsmBlob == nullptr)
            {
                assert(false && "Failed to compile shader");
                return nullptr;
            }
        }

        std::ofstream ofs(shaderCacheFilePath.string(), std::ios::binary);
        assert(ofs.is_open());
        ofs.write(reinterpret_cast<const char*>(dxilBlob->getBufferPointer()), dxilBlob->getBufferSize());
        ofs.close();
        
        const std::filesystem::path shaderCacheAsmPath = shaderAbsPath.parent_path() / "asms";
        if (!std::filesystem::exists(shaderCacheAsmPath))
        {
            std::filesystem::create_directories(shaderCacheAsmPath);
        }
        std::filesystem::path shaderCacheAsmFilePath = shaderCacheAsmPath / (shaderName + ".asm");
        ofs.open(shaderCacheAsmFilePath.string(), std::ios::binary);
        assert(ofs.is_open());
        ofs.write(reinterpret_cast<const char*>(dxilAsmBlob->getBufferPointer()), dxilAsmBlob->getBufferSize());
        ofs.close();

        return dxilBlob;
    }

    bool
    createRootSignature(D3DPtr<ID3D12RootSignature>& outRootSignature, const std::vector<D3D12_ROOT_PARAMETER>* rootParametersOrNull = nullptr, const bool isLocal = false) noexcept
    {
        HRESULT hr = S_OK;
        const D3D12_ROOT_SIGNATURE_DESC ROOT_SIGNATURE_DESC =
        {
            .NumParameters = static_cast<uint32_t>(rootParametersOrNull ? rootParametersOrNull->size() : 0),
            .pParameters = rootParametersOrNull ? rootParametersOrNull->data() : nullptr,
            .NumStaticSamplers = 0,
            .pStaticSamplers = nullptr,
            .Flags = isLocal ? D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE : D3D12_ROOT_SIGNATURE_FLAG_NONE,
        };

        D3DPtr<ID3DBlob> signature;
        D3DPtr<ID3DBlob> error;
        hr = D3D12SerializeRootSignature(&ROOT_SIGNATURE_DESC, D3D_ROOT_SIGNATURE_VERSION_1, signature.GetAddressOf(), error.GetAddressOf());
        if (FAILED(hr))
        {
            if (error != nullptr)
            {
                OutputDebugStringA(reinterpret_cast<const char*>(error->GetBufferPointer()));
                OutputDebugStringA("\n");
            }
            DebugBreak();
            return false;
        }

        hr = gDevice->CreateRootSignature(1, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(outRootSignature.GetAddressOf()));
        if(FAILED(hr))
        {
            assert(false && "Failed to create root signature");
            return false;
        }

        return true;
    }

    CGS_INLINE bool
    createRootSignature(D3DPtr<ID3D12RootSignature>& outRootSignature, const std::vector<D3D12_ROOT_PARAMETER>& rootParametersOrNull, const bool isLocal = false) noexcept
    {
        return createRootSignature(outRootSignature, &rootParametersOrNull, isLocal);
    }

    CGS_INLINE bool
    createLocalRootSignature(D3DPtr<ID3D12RootSignature>& outRootSignature) noexcept
    {
        return createRootSignature(outRootSignature, nullptr, true);
    }

    struct CreateRaytracingPipelineInfo final
    {
        std::unique_ptr<RaytracingPipeline>& OutPipeline;

        std::vector<slang::CompilerOptionEntry>& CompilerOptions;
        const std::filesystem::path& ShaderAbsoluteParentPath;
        std::filesystem::path ShaderName;
        std::string RayGenEntryPointName;
        std::string ClosestHitEntryPointName;
        std::string MissEntryPointName;
        std::wstring HitGroupName;
        const uint32 PayloadSizeInBytes;
        const uint32 AttributeSizeInBytes;
        std::vector<D3D12_ROOT_PARAMETER> RootParameters;
    };

    bool
    createRaytracingPipeline(CreateRaytracingPipelineInfo& createInfo) noexcept
    {
        RaytracingPipeline::CreateInfo raytracingPipelineCreateInfo = {};

        std::vector<slang::TargetDesc> targetDescs = 
        {
            slang::TargetDesc
            {
                .format = SlangCompileTarget::SLANG_DXIL,
                .profile = gSlangGlobalSession->findProfile("sm_6_7"),
                .flags = 0,
            },
            slang::TargetDesc
            {
                .format = SlangCompileTarget::SLANG_DXIL_ASM,
                .profile = gSlangGlobalSession->findProfile("sm_6_7"),
                .flags = 0,
            },
        };

        slang::SessionDesc sessionDesc = 
        {
            .targets = targetDescs.data(),
            .targetCount = static_cast<uint32_t>(targetDescs.size()),
            .compilerOptionEntries = createInfo.CompilerOptions.data(),
            .compilerOptionEntryCount = static_cast<uint32_t>(createInfo.CompilerOptions.size()),
        };


        Slang::ComPtr<slang::ISession> session;
        gSlangGlobalSession->createSession(sessionDesc, session.writeRef());
        const std::filesystem::path shaderAbsPath = createInfo.ShaderAbsoluteParentPath / createInfo.ShaderName;

        slang::IModule* module = nullptr;
        {
            Slang::ComPtr<slang::IBlob> diagnosticBlob;
            module = session->loadModule(shaderAbsPath.string().c_str(), diagnosticBlob.writeRef());
            if (diagnosticBlob)
            {
                OutputDebugStringA(reinterpret_cast<const char*>(diagnosticBlob->getBufferPointer()));
                OutputDebugStringA("\n");
                DebugBreak();
                return false;
            }
            
            if(module == nullptr)
            {
                assert(false && "Failed to load shader module");
                return false;
            }
        }
        
        std::vector<slang::IComponentType*> componentTypes =
        {
            module,
        };
        Slang::ComPtr<slang::IEntryPoint> rayGenEntryPoint;
        module->findEntryPointByName(createInfo.RayGenEntryPointName.c_str(), rayGenEntryPoint.writeRef());
        componentTypes.push_back(rayGenEntryPoint);
        Slang::ComPtr<slang::IEntryPoint> closestHitEntryPoint;
        module->findEntryPointByName(createInfo.ClosestHitEntryPointName.c_str(), closestHitEntryPoint.writeRef());
        componentTypes.push_back(closestHitEntryPoint);
        Slang::ComPtr<slang::IEntryPoint> missEntryPoint;
        module->findEntryPointByName(createInfo.MissEntryPointName.c_str(), missEntryPoint.writeRef());
        componentTypes.push_back(missEntryPoint);
        
        Slang::ComPtr<slang::IComponentType> program;
        {
            Slang::ComPtr<slang::IBlob> diagnosticBlob;
            session->createCompositeComponentType(componentTypes.data(), componentTypes.size(), program.writeRef(), diagnosticBlob.writeRef());
            if (diagnosticBlob)
            {
                OutputDebugStringA(reinterpret_cast<const char*>(diagnosticBlob->getBufferPointer()));
                OutputDebugStringA("\n");
                DebugBreak();
                return false;
            }

            if(program == nullptr)
            {
                assert(false && "Failed to create composite component type");
                return false;
            }
        }

        Slang::ComPtr<slang::IComponentType> linkedProgram;
        {
        const std::vector<slang::IComponentType*> components =
        {
            program.get(),
        };
            Slang::ComPtr<slang::IBlob> diagnosticBlob;
            session->createCompositeComponentType(components.data(), components.size(), linkedProgram.writeRef(), diagnosticBlob.writeRef());
            if (diagnosticBlob)
            {
                OutputDebugStringA(reinterpret_cast<const char*>(diagnosticBlob->getBufferPointer()));
                OutputDebugStringA("\n");
                DebugBreak();
                return false;
            }

            if(linkedProgram == nullptr)
            {
                assert(false && "Failed to create composite component type");
                return false;
            }
        }

        raytracingPipelineCreateInfo.RayGenShader = compileShader(program, shaderAbsPath, ShaderType::RAY_GEN, 0);
        if(raytracingPipelineCreateInfo.RayGenShader == nullptr)
        {
            assert(false && "Failed to compile ray generation shader");
            return false;
        }
        raytracingPipelineCreateInfo.ClosestHitShader = compileShader(program, shaderAbsPath, ShaderType::CLOSEST_HIT, 1);
        if(raytracingPipelineCreateInfo.ClosestHitShader == nullptr)
        {
            assert(false && "Failed to compile closest hit shader");
            return false;
        }
        raytracingPipelineCreateInfo.MissShader = compileShader(program, shaderAbsPath, ShaderType::MISS, 2);
        if(raytracingPipelineCreateInfo.MissShader == nullptr)
        {
            assert(false && "Failed to compile miss shader");
            return false;
        }

        // Global root signature
        createRootSignature(raytracingPipelineCreateInfo.GlobalRootSignature, createInfo.RootParameters);

        // Local root signature
        createLocalRootSignature(raytracingPipelineCreateInfo.LocalRootSignature);

        // DXIL
        const std::wstring rayGenEntryPointNameW(createInfo.RayGenEntryPointName.begin(), createInfo.RayGenEntryPointName.end());
        const std::wstring closestHitEntryPointNameW(createInfo.ClosestHitEntryPointName.begin(), createInfo.ClosestHitEntryPointName.end());
        const std::wstring missEntryPointNameW(createInfo.MissEntryPointName.begin(), createInfo.MissEntryPointName.end());
        std::vector<D3D12_EXPORT_DESC> exportDescs =
        {
            {
                .Name = rayGenEntryPointNameW.c_str(),
                .ExportToRename = nullptr,
                .Flags = D3D12_EXPORT_FLAG_NONE,
            },
            {
                .Name = closestHitEntryPointNameW.c_str(),
                .ExportToRename = nullptr,
                .Flags = D3D12_EXPORT_FLAG_NONE,
            },
            {
                .Name = missEntryPointNameW.c_str(),
                .ExportToRename = nullptr,
                .Flags = D3D12_EXPORT_FLAG_NONE,
            },
        };
        const std::vector<D3D12_DXIL_LIBRARY_DESC> libraries = 
        {
            {
                .DXILLibrary = 
                {
                    .pShaderBytecode = raytracingPipelineCreateInfo.RayGenShader->getBufferPointer(),
                    .BytecodeLength = raytracingPipelineCreateInfo.RayGenShader->getBufferSize(),
                },
                .NumExports = 1,
                .pExports = &exportDescs[0],
            },
            {
                .DXILLibrary = 
                {
                    .pShaderBytecode = raytracingPipelineCreateInfo.ClosestHitShader->getBufferPointer(),
                    .BytecodeLength = raytracingPipelineCreateInfo.ClosestHitShader->getBufferSize(),
                },
                .NumExports = 1,
                .pExports = &exportDescs[1],
            },
            {
                .DXILLibrary = 
                {
                    .pShaderBytecode = raytracingPipelineCreateInfo.MissShader->getBufferPointer(),
                    .BytecodeLength = raytracingPipelineCreateInfo.MissShader->getBufferSize(),
                },
                .NumExports = 1,
                .pExports = &exportDescs[2],
            },
        };

        // Triangle hit group
        const D3D12_HIT_GROUP_DESC hitGroupDesc =
        {
            .HitGroupExport = createInfo.HitGroupName.c_str(),
            .Type = D3D12_HIT_GROUP_TYPE_TRIANGLES,
            .ClosestHitShaderImport = closestHitEntryPointNameW.c_str(),
        };

        // Shader config
        const D3D12_RAYTRACING_SHADER_CONFIG raytracingShaderConfig =
        {
            .MaxPayloadSizeInBytes = createInfo.PayloadSizeInBytes,
            .MaxAttributeSizeInBytes = createInfo.AttributeSizeInBytes,
        };

        // Local config
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localRootSignatureAssociation =
        {
            .pSubobjectToAssociate = nullptr,
            .NumExports = 1,
            .pExports = &exportDescs[0].Name,
        };

        const D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig =
        {
            .MaxTraceRecursionDepth = 5,
        };

        std::vector<D3D12_STATE_SUBOBJECT> subObjects = 
        {
            {
                .Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
                .pDesc = &libraries[0],
            },
            {
                .Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
                .pDesc = &libraries[1],
            },
            {
                .Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
                .pDesc = &libraries[2],
            },
            {
                .Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,
                .pDesc = &hitGroupDesc,
            },
            {
                .Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,
                .pDesc = &raytracingShaderConfig,
            },
            {
                .Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE,
                .pDesc = raytracingPipelineCreateInfo.LocalRootSignature.GetAddressOf(),
            },
            {
                .Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
                .pDesc = &localRootSignatureAssociation,
            },
            {
                .Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,
                .pDesc = raytracingPipelineCreateInfo.GlobalRootSignature.GetAddressOf(),
            },
            {
                .Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,
                .pDesc = &pipelineConfig,
            },
        };
        localRootSignatureAssociation.pSubobjectToAssociate = &subObjects[5];

        const D3D12_STATE_OBJECT_DESC stateObjectDesc =
        {
            .Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
            .NumSubobjects = static_cast<uint32>(subObjects.size()),
            .pSubobjects = subObjects.data(),
        };
        HRESULT hr = gDevice->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(raytracingPipelineCreateInfo.StateObject.GetAddressOf()));
        if(FAILED(hr))
        {
            assert(false && "Failed to create raytracing pipeline state object");
            return false;
        }

        D3DPtr<ID3D12StateObjectProperties> stateObjectProperties;
        hr = raytracingPipelineCreateInfo.StateObject->QueryInterface(IID_PPV_ARGS(stateObjectProperties.GetAddressOf()));
        if (FAILED(hr))
        {
            assert(false && "Failed to get raytracing state object properties");
            return false;
        }

        void* rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(rayGenEntryPointNameW.c_str());
        void* missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(missEntryPointNameW.c_str());
        void* hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(createInfo.HitGroupName.c_str());
        const uint32 shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
            
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
        // Ray gen shader table
        {
            const uint32 shaderRecordsCount = 1;
            const uint32 shaderRecordSize = Align(shaderIdentifierSize, static_cast<uint32>(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT));
            bufferDesc.Width = shaderRecordsCount * shaderRecordSize;
            
            hr = gDevice->CreateCommittedResource(
                &bufferHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(raytracingPipelineCreateInfo.RayGenShaderTable.GetAddressOf()));
            if(FAILED(hr))
            {
                assert(false && "Failed to create ray gen shader table");
                return false;
            }
            raytracingPipelineCreateInfo.RayGenShaderTable->SetName(TEXT("RayGenShaderTable"));

            uint8_t* mappedData = nullptr;
            // We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
            D3D12_RANGE range = { .Begin = 0, .End = 0 };
            hr = raytracingPipelineCreateInfo.RayGenShaderTable->Map(0, &range, reinterpret_cast<void**>(&mappedData));
            if(FAILED(hr))
            {
                assert(false && "Failed to map ray gen shader table");
                return false;
            }
            
            memcpy(mappedData, rayGenShaderIdentifier, shaderIdentifierSize);
            mappedData += shaderRecordSize;
        }

        // Miss shader table
        {
            const uint32 shaderRecordsCount = 1;
            const uint32 shaderRecordSize = Align(shaderIdentifierSize, static_cast<uint32>(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT));
            bufferDesc.Width = shaderRecordsCount * shaderRecordSize;
            
            hr = gDevice->CreateCommittedResource(
                &bufferHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(raytracingPipelineCreateInfo.MissShaderTable.GetAddressOf()));
            if(FAILED(hr))
            {
                assert(false && "Failed to create miss shader table");
                return false;
            }

            uint8_t* mappedData = nullptr;
            // We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
            D3D12_RANGE range = { .Begin = 0, .End = 0 };
            hr = raytracingPipelineCreateInfo.MissShaderTable->Map(0, &range, reinterpret_cast<void**>(&mappedData));
            if(FAILED(hr))
            {
                assert(false && "Failed to map miss shader table");
                return false;
            }

            memcpy(mappedData, missShaderIdentifier, shaderIdentifierSize);
            mappedData += shaderRecordSize;
        }

        // Hit group shader table
        {
            const uint32 shaderRecordsCount = 1;
            const uint32 shaderRecordSize = Align(shaderIdentifierSize, static_cast<uint32>(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT));
            bufferDesc.Width = shaderRecordsCount * shaderRecordSize;
            
            hr = gDevice->CreateCommittedResource(
                &bufferHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(raytracingPipelineCreateInfo.HitGroupShaderTable.GetAddressOf()));
            if(FAILED(hr))
            {
                assert(false && "Failed to create hit group shader table");
                return false;
            }

            uint8_t* mappedData = nullptr;
            // We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
            D3D12_RANGE range = { .Begin = 0, .End = 0 };
            hr = raytracingPipelineCreateInfo.HitGroupShaderTable->Map(0, &range, reinterpret_cast<void**>(&mappedData));
            if(FAILED(hr))
            {
                assert(false && "Failed to map hit group shader table");
                return false;
            }

            memcpy(mappedData, hitGroupShaderIdentifier, shaderIdentifierSize);
            mappedData += shaderRecordSize;
        }
        createInfo.OutPipeline = std::make_unique<RaytracingPipeline>(std::move(raytracingPipelineCreateInfo));
        return true;
    }

    bool
    createShaders(const eRenderMethod renderMethod) noexcept
    {
        HRESULT hr = S_OK;
        SlangGlobalSessionDesc slangGlobalSessionDesc = {};
        slang::createGlobalSession(&slangGlobalSessionDesc, gSlangGlobalSession.writeRef());

        const std::filesystem::path shaderAbsoluteParentPath = std::filesystem::current_path() / "Assets/Shaders";
		std::vector<slang::CompilerOptionEntry> compilerOptions =
		{
			slang::CompilerOptionEntry
			{
				.name = slang::CompilerOptionName::Include,
				.value = slang::CompilerOptionValue
				{
					.kind = slang::CompilerOptionValueKind::String,
					.stringValue0 = shaderAbsoluteParentPath.string().c_str(),
				},
			},
			slang::CompilerOptionEntry
			{
				.name = slang::CompilerOptionName::EmitSpirvDirectly,
				.value = slang::CompilerOptionValue
				{
					.kind = slang::CompilerOptionValueKind::Int,
					.intValue0 = 1,
				},
			},
#if defined(CGS_DEBUG)
			slang::CompilerOptionEntry
			{
				.name = slang::CompilerOptionName::DebugInformation,
				.value = slang::CompilerOptionValue
				{
					.kind = slang::CompilerOptionValueKind::Int,
					.intValue0 = SlangDebugInfoLevel::SLANG_DEBUG_INFO_LEVEL_MAXIMAL,
				},
			},
			slang::CompilerOptionEntry
			{
				.name = slang::CompilerOptionName::DebugInformationFormat,
				.value = slang::CompilerOptionValue
				{
					.kind = slang::CompilerOptionValueKind::Int,
					.intValue0 = SlangDebugInfoFormat::SLANG_DEBUG_INFO_FORMAT_PDB,
				},
			},
#endif	// NOT defined(CGS_DEBUG)
			slang::CompilerOptionEntry
			{
				.name = slang::CompilerOptionName::Optimization,
				.value = slang::CompilerOptionValue
				{
					.kind = slang::CompilerOptionValueKind::Int,
#if defined(CGS_DEBUG)
					.intValue0 = SlangOptimizationLevel::SLANG_OPTIMIZATION_LEVEL_NONE,
#else	// NOT defined(CGS_DEBUG)
					.intValue0 = SlangOptimizationLevel::SLANG_OPTIMIZATION_LEVEL_HIGH,
#endif	// NOT defined(CGS_DEBUG)
				},
			},
			slang::CompilerOptionEntry
			{
				.name = slang::CompilerOptionName::WarningsAsErrors,
				.value = slang::CompilerOptionValue
				{
					.kind = slang::CompilerOptionValueKind::String,
					.stringValue0 = "all",
				},
			},
		};

        // Rasterization
        switch(renderMethod)
        {
        case eRenderMethod::RASTERIZATION:
        {
            std::vector<slang::TargetDesc> targetDescs = 
            {
                slang::TargetDesc
                {
                    .format = SlangCompileTarget::SLANG_DXIL,
                    .profile = gSlangGlobalSession->findProfile("sm_6_6"),
                    .flags = 0,
                },
                slang::TargetDesc
                {
                    .format = SlangCompileTarget::SLANG_DXIL_ASM,
                    .profile = gSlangGlobalSession->findProfile("sm_6_6"),
                    .flags = 0,
                },
            };

            slang::SessionDesc sessionDesc = 
            {
                .targets = targetDescs.data(),
                .targetCount = static_cast<uint32_t>(targetDescs.size()),
                .compilerOptionEntries = compilerOptions.data(),
                .compilerOptionEntryCount = static_cast<uint32_t>(compilerOptions.size()),
            };


            Slang::ComPtr<slang::ISession> session;
            gSlangGlobalSession->createSession(sessionDesc, session.writeRef());
            const std::filesystem::path shaderAbsPath = shaderAbsoluteParentPath / "SimpleRasterization.slang";

            slang::IModule* module = nullptr;
            {
                Slang::ComPtr<slang::IBlob> diagnosticBlob;
                module = session->loadModule(shaderAbsPath.string().c_str(), diagnosticBlob.writeRef());
                if (diagnosticBlob)
                {
                    OutputDebugStringA(reinterpret_cast<const char*>(diagnosticBlob->getBufferPointer()));
                    OutputDebugStringA("\n");
                    DebugBreak();
                    return false;
                }
                
                if(module == nullptr)
                {
                    assert(false && "Failed to load shader module");
                    return false;
                }
            }
            
            std::vector<slang::IComponentType*> componentTypes =
            {
                module,
            };
            Slang::ComPtr<slang::IEntryPoint> vsEntryPoint;
            module->findEntryPointByName("VSMain", vsEntryPoint.writeRef());
            componentTypes.push_back(vsEntryPoint);
            Slang::ComPtr<slang::IEntryPoint> fsEntryPoint;
            module->findEntryPointByName("FSMain", fsEntryPoint.writeRef());
            componentTypes.push_back(fsEntryPoint);
            
            Slang::ComPtr<slang::IComponentType> program;
            {
                Slang::ComPtr<slang::IBlob> diagnosticBlob;
                session->createCompositeComponentType(componentTypes.data(), componentTypes.size(), program.writeRef(), diagnosticBlob.writeRef());
                if (diagnosticBlob)
                {
                    OutputDebugStringA(reinterpret_cast<const char*>(diagnosticBlob->getBufferPointer()));
                    OutputDebugStringA("\n");
                    DebugBreak();
                    return false;
                }

                if(program == nullptr)
                {
                    assert(false && "Failed to create composite component type");
                    return false;
                }
            }

            gVsShader = compileShader(program, shaderAbsPath, ShaderType::VERTEX, 0);
            if(gVsShader == nullptr)
            {
                assert(false && "Failed to compile vertex shader");
                return false;
            }
            gFsShader = compileShader(program, shaderAbsPath, ShaderType::FRAGMENT, 1);
            if(gFsShader == nullptr)
            {
                assert(false && "Failed to compile fragment shader");
                return false;
            }
            
            // Create an empty root signature.
            const std::vector<D3D12_ROOT_PARAMETER> rootParameters =
            {
                {
                    .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
                    .Descriptor = 
                    {
                        .ShaderRegister = 0,
                        .RegisterSpace = 0,
                    },
                    .ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX,
                },
                {
                    .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
                    .Descriptor = 
                    {
                        .ShaderRegister = 1,
                        .RegisterSpace = 0,
                    },
                    .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
                },
                {
                    .ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
                    .Descriptor = 
                    {
                        .ShaderRegister = 0,
                        .RegisterSpace = 0,
                    },
                    .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
                },
            };

            createRootSignature(gRasterizationRootSignature, rootParameters);

            constexpr D3D12_INPUT_ELEMENT_DESC INPUT_ELEMENT_DESCS[2] = 
            {
                {
                    .SemanticName = "POSITION",
                    .SemanticIndex = 0,
                    .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                    .InputSlot = 0,
                    .AlignedByteOffset = 0,
                    .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                    .InstanceDataStepRate = 0,
                },
                {
                    .SemanticName = "NORMAL",
                    .SemanticIndex = 0,
                    .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                    .InputSlot = 0,
                    .AlignedByteOffset = 12,
                    .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                    .InstanceDataStepRate = 0,
                },
            };

            D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = 
            {
                .pRootSignature = gRasterizationRootSignature.Get(),
                .VS = gVsShader ? D3D12_SHADER_BYTECODE{ gVsShader->getBufferPointer(), gVsShader->getBufferSize() } : D3D12_SHADER_BYTECODE{ nullptr, 0 },
                .PS = gFsShader ? D3D12_SHADER_BYTECODE{ gFsShader->getBufferPointer(), gFsShader->getBufferSize() } : D3D12_SHADER_BYTECODE{ nullptr, 0 },
                .BlendState =
                {
                    .AlphaToCoverageEnable = FALSE,
                    .IndependentBlendEnable = FALSE,
                    .RenderTarget = 
                    {
                        {
                            .BlendEnable = FALSE,
                            .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
                        },
                    },             
                },
                .SampleMask = std::numeric_limits<uint32>::max(),
                .RasterizerState = 
                {
                    .FillMode = D3D12_FILL_MODE_SOLID,
                    .CullMode = D3D12_CULL_MODE_BACK,
                    .FrontCounterClockwise = FALSE,
                    .DepthBias = 0,
                    .DepthBiasClamp = 0.0f,
                    .SlopeScaledDepthBias = 0.0f,
                    .DepthClipEnable = TRUE,
                    .MultisampleEnable = FALSE,
                    .AntialiasedLineEnable = FALSE,
                    .ForcedSampleCount = 0,
                    .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
                },
                .DepthStencilState =
                {
                    .DepthEnable = TRUE,
                    .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
                    .DepthFunc = D3D12_COMPARISON_FUNC_LESS,
                },
                .InputLayout = 
                {
                    .pInputElementDescs = INPUT_ELEMENT_DESCS,
                    .NumElements = static_cast<UINT>(CGS_ARRAYSIZE(INPUT_ELEMENT_DESCS)),
                },
                .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                .NumRenderTargets = 1,
                .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB },
                .DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT,
                .SampleDesc = { 1, 0 },
                .NodeMask = 0,
                .CachedPSO = {},
                .Flags = D3D12_PIPELINE_STATE_FLAG_NONE,
            };

            hr = gDevice->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(gRasterizationPipelineState.GetAddressOf()));
            if(FAILED(hr))
            {
                assert(false && "Failed to create pipeline state");
                return false;
            }
        }
        break;
        case eRenderMethod::RAYTRACING:
        {
            static constexpr D3D12_DESCRIPTOR_RANGE DESCRIPTOR_RANGE[]
            {
                // UAVs
                {
                    .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
                    .NumDescriptors = 8,    // u0: output texture, u1: gbuffers, 
                                            // u2: parallelogram area light sample reservoir, u3: point light reservoir, u4: indirect light reservoir, 
                                            // u5: previous parallelogram area light reservoir, u6: previous point light reservoir, u7: previous indirect light reservoir
                    .BaseShaderRegister = 0,
                    .RegisterSpace = 0,
                    .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND,
                },
                // Geometry Information
                {
                    .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                    .NumDescriptors = 4,
                    .BaseShaderRegister = 1,    // t0: AS, t1: index buffer, t2: vertex buffer, t3: color buffer, t4: is emissives buffer
                    .RegisterSpace = 0,
                    .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND,
                },
                // Emissive Information
                {
                    .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                    .NumDescriptors = 2,
                    .BaseShaderRegister = 5,    // t5: parallelogram area light info buffer, t6: point light info buffer
                    .RegisterSpace = 0,
                    .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND,
                },
            };

            CreateRaytracingPipelineInfo createRaytracingPipelineInfo = 
            {
                .OutPipeline = gRisPipeline,

                .CompilerOptions = compilerOptions,
                .ShaderAbsoluteParentPath = shaderAbsoluteParentPath,
                .ShaderName = "SimpleRaytracing.slang",
                .RayGenEntryPointName = "RayGenMain",
                .ClosestHitEntryPointName = "ClosestHitMain",
                .MissEntryPointName = "MissMain",
                .HitGroupName = L"HitGroup",
                .PayloadSizeInBytes = sizeof(GBufferRayPayload),
                .AttributeSizeInBytes = sizeof(float) * 2,  // BuiltInTriangleIntersectionAttributes
                .RootParameters = 
                {
                    // UAVs
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                        .DescriptorTable = 
                        {
                            .NumDescriptorRanges = 1,
                            .pDescriptorRanges = &DESCRIPTOR_RANGE[0],
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    }, 
                    // Acceleration structure
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
                        .Descriptor = 
                        {
                            .ShaderRegister = 0,
                            .RegisterSpace = 0,
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    },
                    // Scene constant buffer
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
                        .Descriptor = 
                        {
                            .ShaderRegister = 0,
                            .RegisterSpace = 0,
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    },
                    // Geometry information
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                        .DescriptorTable = 
                        {
                            .NumDescriptorRanges = 1,
                            .pDescriptorRanges = &DESCRIPTOR_RANGE[1],
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    },
                    // Emissive information
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                        .DescriptorTable = 
                        {
                            .NumDescriptorRanges = 1,
                            .pDescriptorRanges = &DESCRIPTOR_RANGE[2],
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    },
                    // Push Constants
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
                        .Constants = 
                        {
                            .ShaderRegister = 1,
                            .RegisterSpace = 0,
                            .Num32BitValues = 2,
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    },
                },
            };
            createRaytracingPipeline(createRaytracingPipelineInfo);
            
            CreateRaytracingPipelineInfo createTemporalResamplingRaytracingPipelineInfo = 
            {
                .OutPipeline = gTemporalResamplingPipeline,

                .CompilerOptions = compilerOptions,
                .ShaderAbsoluteParentPath = shaderAbsoluteParentPath,
                .ShaderName = "TemporalResampling.slang",
                .RayGenEntryPointName = "RayGenMain",
                .ClosestHitEntryPointName = "ClosestHitMain",
                .MissEntryPointName = "MissMain",
                .HitGroupName = L"HitGroup",
                .PayloadSizeInBytes = sizeof(GBufferRayPayload),
                .AttributeSizeInBytes = sizeof(float) * 2,  // BuiltInTriangleIntersectionAttributes
                .RootParameters = 
                {
                    // UAVs
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                        .DescriptorTable = 
                        {
                            .NumDescriptorRanges = 1,
                            .pDescriptorRanges = &DESCRIPTOR_RANGE[0],
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    }, 
                    // Acceleration structure
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
                        .Descriptor = 
                        {
                            .ShaderRegister = 0,
                            .RegisterSpace = 0,
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    },
                    // Scene constant buffer
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
                        .Descriptor = 
                        {
                            .ShaderRegister = 0,
                            .RegisterSpace = 0,
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    },
                    // Geometry information
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                        .DescriptorTable = 
                        {
                            .NumDescriptorRanges = 1,
                            .pDescriptorRanges = &DESCRIPTOR_RANGE[1],
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    },
                    // Emissive information
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                        .DescriptorTable = 
                        {
                            .NumDescriptorRanges = 1,
                            .pDescriptorRanges = &DESCRIPTOR_RANGE[2],
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    },
                    // Push Constants
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
                        .Constants = 
                        {
                            .ShaderRegister = 1,
                            .RegisterSpace = 0,
                            .Num32BitValues = 2,
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    },
                },
            };
            createRaytracingPipeline(createTemporalResamplingRaytracingPipelineInfo);
            
            CreateRaytracingPipelineInfo createSpatialResamplingRaytracingPipelineInfo = 
            {
                .OutPipeline = gSpatialResamplingPipeline,

                .CompilerOptions = compilerOptions,
                .ShaderAbsoluteParentPath = shaderAbsoluteParentPath,
                .ShaderName = "SpatialResampling.slang",
                .RayGenEntryPointName = "RayGenMain",
                .ClosestHitEntryPointName = "ClosestHitMain",
                .MissEntryPointName = "MissMain",
                .HitGroupName = L"HitGroup",
                .PayloadSizeInBytes = sizeof(GBufferRayPayload),
                .AttributeSizeInBytes = sizeof(float) * 2,  // BuiltInTriangleIntersectionAttributes
                .RootParameters = 
                {
                    // UAVs
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                        .DescriptorTable = 
                        {
                            .NumDescriptorRanges = 1,
                            .pDescriptorRanges = &DESCRIPTOR_RANGE[0],
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    }, 
                    // Acceleration structure
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
                        .Descriptor = 
                        {
                            .ShaderRegister = 0,
                            .RegisterSpace = 0,
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    },
                    // Scene constant buffer
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
                        .Descriptor = 
                        {
                            .ShaderRegister = 0,
                            .RegisterSpace = 0,
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    },
                    // Geometry information
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                        .DescriptorTable = 
                        {
                            .NumDescriptorRanges = 1,
                            .pDescriptorRanges = &DESCRIPTOR_RANGE[1],
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    },
                    // Emissive information
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                        .DescriptorTable = 
                        {
                            .NumDescriptorRanges = 1,
                            .pDescriptorRanges = &DESCRIPTOR_RANGE[2],
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    },
                    // Push Constants
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
                        .Constants = 
                        {
                            .ShaderRegister = 1,
                            .RegisterSpace = 0,
                            .Num32BitValues = 2,
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    },
                },
            };
            createRaytracingPipeline(createSpatialResamplingRaytracingPipelineInfo);
        }
        break;
        default:
            assert(false && "Invalid render method!!");
            break;
        }

        return true;
    }

    void
    DestroyRenderer(std::vector<std::unique_ptr<Geometry>>& geometries) noexcept
    {
        for(uint32 frameBufferIndex = 0; frameBufferIndex < BACK_BUFFERS_COUNT; ++frameBufferIndex)
        {
            waitForFrame(frameBufferIndex);
        }

        geometries.clear();
        for (uint32 i = 0; i < BACK_BUFFERS_COUNT; ++i)
        {
            gEmissiveBuffers[i].reset();
            gCameraBuffers[i].reset();
            gSceneConstantBuffers[i].reset();
        }
        gRaytracingGBuffers.clear();
        gRaytracingOutputs.clear();
        gRaytracingParallelogramAreaLightSampleReservoirs.clear();
        gRaytracingPointLightReservoirs.clear();
        gRaytracingIndirectLightReservoirs.clear();
        gRaytracingPrevParallelogramAreaLightSampleReservoirs.clear();
        gRaytracingPrevPointLightReservoirs.clear();
        gRaytracingPrevIndirectLightReservoirs.clear();

        for(D3DPtr<ID3D12Resource>& bottomLevelAccelerationStructure : gBottomLevelAccelerationStructures)
        {
            DestroyD3D12Object(bottomLevelAccelerationStructure);
        }

        for(D3DPtr<ID3D12Resource>& topLevelAccelerationStructure : gTopLevelAccelerationStructures)
        {
            DestroyD3D12Object(topLevelAccelerationStructure);
        }
        gRisPipeline.reset();
        gTemporalResamplingPipeline.reset();
        gSpatialResamplingPipeline.reset();

        DestroyD3D12Object(gRasterizationPipelineState);
        DestroyD3D12Object(gRasterizationRootSignature);
        if(gVsShader != nullptr)
        {
            gVsShader->Release();
        }
        else
        {
            assert(false && "gVsShader is null");
        }

        if(gFsShader != nullptr)
        {
            gFsShader->Release();
        }
        else
        {
            assert(false && "gFsShader is null");
        }

        DestroyD3D12Object(gFence);
        
        gSceneRenderTargets.clear();

        gCbvSrvUavHeapOnlyCpu.reset();
        gCbvSrvUavHeap.reset();
        DestroyD3D12Object(gDsvHeap);
        DestroyD3D12Object(gRtvHeap);

        for(uint32 frameBufferIndex = 0; frameBufferIndex < BACK_BUFFERS_COUNT; ++frameBufferIndex)
        {
            DestroyD3D12Object(gGraphicsCommandLists[frameBufferIndex]);
            DestroyD3D12Object(gGraphicsCommandAllocators[frameBufferIndex]);
        }
        gGraphicsCommandLists.clear();
        gGraphicsCommandAllocators.clear();

        DestroyDXGIObject(gSwapChain);

        DestroyD3D12Object(gGraphicsCommandQueue);

#if defined(CGS_DEBUG)
        DestroyD3D12Object(gD3D12InfoQueue, false);
#endif  // defined(CGS_DEBUG)
        DestroyD3D12Object(gDevice, false);
#if defined(CGS_DEBUG)
        DestroyD3D12Object(gD3D12Debug);
#endif  // defined(CGS_DEBUG)

        DestroyDXGIObject(gAdapter);
        DestroyDXGIObject(gFactory);

#if defined(CGS_DEBUG)
        if(gDxgiDebug != nullptr)
        {
            gDxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        }
        else
        {
            assert(false && "gDxgiDebug is null");
        }
        DestroyDXGIObject(gInfoQueue, false);
        DestroyDXGIObject(gDxgiDebug, false);
#endif  // defined(CGS_DEBUG)
    }

    static void
    addQuadVertices(std::vector<VertexPN>& inoutVertices, std::vector<uint16>& inoutIndices, const Coordinate<eCoordinateSpace::WORLD>& v0, const Coordinate<eCoordinateSpace::WORLD>& v1, const Coordinate<eCoordinateSpace::WORLD>& v2, const Coordinate<eCoordinateSpace::WORLD>& v3)
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
    createVertexBuffer(VertexBuffer& outVertexBuffer, const std::vector<VertexPN>& vertices, const std::string& name)
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
        vertexBufferCreateInfo.View.SizeInBytes = vertexBufferCreateInfo.View.StrideInBytes * static_cast<uint32>(vertices.size());

        const D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
        {
            .Format = DXGI_FORMAT_UNKNOWN,
            .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Buffer = 
            {
                .NumElements = static_cast<uint32>(vertices.size()),
                .StructureByteStride = sizeof(VertexPN),
                .Flags = D3D12_BUFFER_SRV_FLAG_NONE,
            },
        };
        // eCbvSrvUavRaytracingDescriptorType
        const bool result = gCbvSrvUavHeap->AllocateStaticDescriptor(vertexBufferCreateInfo.ParentCreateInfo.CpuDescriptor, vertexBufferCreateInfo.ParentCreateInfo.GpuDescriptor, static_cast<uint32>(eCbvSrvUavRaytracingDescriptorType::VERTICES));
        if(result == false)
        {
            assert(false && "Failed to allocate static descriptor for vertex buffer");
            return false;
        }
        gDevice->CreateShaderResourceView(vertexBufferCreateInfo.ParentCreateInfo.Data.Get(), &srvDesc, vertexBufferCreateInfo.ParentCreateInfo.CpuDescriptor);

        outVertexBuffer.Initialize(std::move(vertexBufferCreateInfo));
        return true;
    }

    [[nodiscard]] static bool
    createIndexBuffer(IndexBuffer& outIndexBuffer, const std::vector<uint16>& indices, const std::string& name)
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
        indexBufferCreateInfo.View.SizeInBytes = static_cast<uint32>(sizeof(uint16) * indices.size());
        indexBufferCreateInfo.View.Format = DXGI_FORMAT_R16_UINT;

        const D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
        {
            .Format = DXGI_FORMAT_R32_TYPELESS,
            .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Buffer = 
            {
                .NumElements = static_cast<uint32>(indices.size() * sizeof(uint16) / 4),
                .StructureByteStride = 0,
                .Flags = D3D12_BUFFER_SRV_FLAG_RAW,
            },
        };
        // eCbvSrvUavRaytracingDescriptorType
        const bool result = gCbvSrvUavHeap->AllocateStaticDescriptor(indexBufferCreateInfo.ParentCreateInfo.CpuDescriptor, indexBufferCreateInfo.ParentCreateInfo.GpuDescriptor, static_cast<uint32>(eCbvSrvUavRaytracingDescriptorType::INDICES));
        if(result == false)
        {
            assert(false && "Failed to allocate static descriptor for index buffer");
            return false;
        }
        gDevice->CreateShaderResourceView(indexBufferCreateInfo.ParentCreateInfo.Data.Get(), &srvDesc, indexBufferCreateInfo.ParentCreateInfo.CpuDescriptor);

        outIndexBuffer.Initialize(std::move(indexBufferCreateInfo));
        return true;
    }

    bool
    CreateCornellBoxScene(const eRenderMethod renderMethod, std::vector<std::unique_ptr<Geometry>>& outGeometries) noexcept
    {
        Camera& mainCamera = InitializeCornellBoxCamera();
        
        HRESULT hr = S_OK;

        SceneConstantBuffer sceneConstantBuffer;
        if(renderMethod == eRenderMethod::RASTERIZATION)
        {
            const D3D12_RESOURCE_DESC cameraBufferDesc =
            {
                .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
                .Alignment = 0,
                .Width = Align(sizeof(Camera::Buffer), static_cast<size_t>(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)),
                .Height = 1,
                .DepthOrArraySize = 1,
                .MipLevels = 1,
                .Format = DXGI_FORMAT_UNKNOWN,
                .SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 },
                .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                .Flags = D3D12_RESOURCE_FLAG_NONE,
            };

            const D3D12_HEAP_PROPERTIES bufferHeapProperties = 
            {
                .Type = D3D12_HEAP_TYPE_UPLOAD,
                .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
                .CreationNodeMask = 1,
                .VisibleNodeMask = 1,
            };

            std::vector<ConstantBuffer::CreateInfo> cameraBufferCreateInfos(BACK_BUFFERS_COUNT);
            for(uint32 i = 0; i < BACK_BUFFERS_COUNT; ++i)
            {
                ConstantBuffer::CreateInfo& cameraBufferCreateInfo = cameraBufferCreateInfos[i];
                cameraBufferCreateInfo.State = D3D12_RESOURCE_STATE_GENERIC_READ;
                hr = gDevice->CreateCommittedResource(
                    &bufferHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &cameraBufferDesc,
                    cameraBufferCreateInfo.State,
                    nullptr,
                    IID_PPV_ARGS(cameraBufferCreateInfo.Data.GetAddressOf())
                );
                if(FAILED(hr))
                {
                    assert(false && "Failed to create camera buffer");
                    return false;
                }

                D3D12_RANGE range = { .Begin = 0, .End = 0 };
                byte* cameraDataBegin = nullptr;
                hr = cameraBufferCreateInfo.Data->Map(
                    0,
                    &range,
                    reinterpret_cast<void**>(&cameraDataBegin)
                );
                if(FAILED(hr))
                {
                    assert(false && "Failed to map camera buffer");
                    return false;
                }

                const Camera::Buffer& cameraBuffer = mainCamera.GetBuffer();
                std::memcpy(cameraDataBegin, &cameraBuffer, sizeof(Camera::Buffer));
                cameraBufferCreateInfo.Data->Unmap(0, nullptr);
            }

#if 0
            std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> cameraCpuDescriptors(BACK_BUFFERS_COUNT);
            std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> cameraGpuDescriptors(BACK_BUFFERS_COUNT);
            const bool result = gCbvSrvUavHeap->AllocateDynamicDescriptors(cameraCpuDescriptors, cameraGpuDescriptors, 0);
            if(result == false)
            {
                assert(false && "Failed to allocate dynamic descriptors for camera buffer");
                return false;
            }
#endif
            for(uint32 i = 0; i < BACK_BUFFERS_COUNT; ++i)
            {
                ConstantBuffer::CreateInfo& cameraBufferCreateInfo = cameraBufferCreateInfos[i];
#if 0
                const D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = 
                {
                    .BufferLocation = cameraBufferCreateInfo.Data->GetGPUVirtualAddress(),
                    .SizeInBytes = static_cast<uint32>(cameraBufferDesc.Width),
                };

                cameraBufferCreateInfo.View = cameraCpuDescriptors[i];
                cameraBufferCreateInfo.GpuDescriptor = cameraGpuDescriptors[i];
                gDevice->CreateConstantBufferView(&cbvDesc, cameraBufferCreateInfo.View);
#endif

                gCameraBuffers[i] = std::make_unique<ConstantBuffer>(std::move(cameraBufferCreateInfo));
            }
        }
        else
        {
            sceneConstantBuffer.CameraPosition = mainCamera.GetPosition();
        }
        
        outGeometries.clear();
        outGeometries.reserve(1);
        outGeometries.push_back(std::make_unique<Geometry>(std::string("Cornell Box")));
        Geometry& cornellBox = *outGeometries.back();

        std::vector<VertexPN> vertices;
        static constexpr uint32 QUAD_VERTICES_COUNT = 4;
        static constexpr uint32 FLOOR_VERTICES_COUNT = QUAD_VERTICES_COUNT;
        static constexpr uint32 LIGHT_VERTICES_COUNT = QUAD_VERTICES_COUNT;
        static constexpr uint32 CEILING_VERTICES_COUNT = QUAD_VERTICES_COUNT;
        static constexpr uint32 BACK_WALL_VERTICES_COUNT = QUAD_VERTICES_COUNT;
        static constexpr uint32 RIGHT_WALL_VERTICES_COUNT = QUAD_VERTICES_COUNT;
        static constexpr uint32 LEFT_WALL_VERTICES_COUNT = QUAD_VERTICES_COUNT;
        static constexpr uint32 CUBE_VERTICES_COUNT = QUAD_VERTICES_COUNT * 5;
        static constexpr uint32 SHORT_BLOCK_VERTICES_COUNT = CUBE_VERTICES_COUNT;
        static constexpr uint32 TALL_BLOCK_VERTICES_COUNT = CUBE_VERTICES_COUNT;
        vertices.reserve(FLOOR_VERTICES_COUNT + LIGHT_VERTICES_COUNT + CEILING_VERTICES_COUNT + BACK_WALL_VERTICES_COUNT + RIGHT_WALL_VERTICES_COUNT + LEFT_WALL_VERTICES_COUNT + SHORT_BLOCK_VERTICES_COUNT + TALL_BLOCK_VERTICES_COUNT);
        std::vector<uint16> indices;
        static constexpr uint32 QUAD_INDICES_COUNT = 6;
        static constexpr uint32 FLOOR_INDICES_COUNT = QUAD_INDICES_COUNT;
        static constexpr uint32 LIGHT_INDICES_COUNT = QUAD_INDICES_COUNT;
        static constexpr uint32 CEILING_INDICES_COUNT = QUAD_INDICES_COUNT;
        static constexpr uint32 BACK_WALL_INDICES_COUNT = QUAD_INDICES_COUNT;
        static constexpr uint32 RIGHT_WALL_INDICES_COUNT = QUAD_INDICES_COUNT;
        static constexpr uint32 LEFT_WALL_INDICES_COUNT = QUAD_INDICES_COUNT;
        static constexpr uint32 CUBE_INDICES_COUNT = QUAD_INDICES_COUNT * 5;
        static constexpr uint32 SHORT_BLOCK_INDICES_COUNT = CUBE_INDICES_COUNT;
        static constexpr uint32 TALL_BLOCK_INDICES_COUNT = CUBE_INDICES_COUNT;
        indices.reserve(FLOOR_INDICES_COUNT + LIGHT_INDICES_COUNT + CEILING_INDICES_COUNT + BACK_WALL_INDICES_COUNT + RIGHT_WALL_INDICES_COUNT + LEFT_WALL_INDICES_COUNT + SHORT_BLOCK_INDICES_COUNT + TALL_BLOCK_INDICES_COUNT);

        std::vector<float3> colors;
        static constexpr uint32 QUAD_TRIANGLES_COUNT = 2;
        static constexpr uint32 FLOOR_TRIANGLES_COUNT = QUAD_TRIANGLES_COUNT;
        static constexpr uint32 LIGHT_TRIANGLES_COUNT = QUAD_TRIANGLES_COUNT;
        static constexpr uint32 CEILING_TRIANGLES_COUNT = QUAD_TRIANGLES_COUNT;
        static constexpr uint32 BACK_WALL_TRIANGLES_COUNT = QUAD_TRIANGLES_COUNT;
        static constexpr uint32 RIGHT_WALL_TRIANGLES_COUNT = QUAD_TRIANGLES_COUNT;
        static constexpr uint32 LEFT_WALL_TRIANGLES_COUNT = QUAD_TRIANGLES_COUNT;
        static constexpr uint32 CUBE_TRIANGLES_COUNT = QUAD_TRIANGLES_COUNT * 5;
        static constexpr uint32 SHORT_BLOCK_TRIANGLES_COUNT = CUBE_TRIANGLES_COUNT;
        static constexpr uint32 TALL_BLOCK_TRIANGLES_COUNT = CUBE_TRIANGLES_COUNT;
        colors.reserve(FLOOR_TRIANGLES_COUNT + LIGHT_TRIANGLES_COUNT + CEILING_TRIANGLES_COUNT + BACK_WALL_TRIANGLES_COUNT + RIGHT_WALL_TRIANGLES_COUNT + LEFT_WALL_TRIANGLES_COUNT + SHORT_BLOCK_TRIANGLES_COUNT + TALL_BLOCK_TRIANGLES_COUNT);

        std::vector<uint8> isEmissives;
        isEmissives.reserve(FLOOR_TRIANGLES_COUNT + LIGHT_TRIANGLES_COUNT + CEILING_TRIANGLES_COUNT + BACK_WALL_TRIANGLES_COUNT + RIGHT_WALL_TRIANGLES_COUNT + LEFT_WALL_TRIANGLES_COUNT + SHORT_BLOCK_TRIANGLES_COUNT + TALL_BLOCK_TRIANGLES_COUNT);

        bool result = false;

        // Floor
        {
            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { 0.0f, 0.0f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -552.8f, 0.0f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -549.6f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 0.0f, 0.0f, 559.2f };
            addQuadVertices(vertices, indices, v0, v1, v2, v3);
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            isEmissives.push_back(false);
            isEmissives.push_back(false);

            gRotationAxis = (v0 + v1 + v2 + v3) / 4.0f;
        }
        
        // Light
        struct EmissiveBuffer final
        {
            float4 Position;
            float3 Color;
        };
        EmissiveBuffer emissiveBuffer;
        std::vector<ParallelogramAreaLightInfo> parallelogramAreaLightInfos;

        // Random point lights
        {
            constexpr uint32 POINT_LIGHTS_COUNT = 64;
            gPointLightInfos.reserve(POINT_LIGHTS_COUNT);
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dist(0.0f, 500.0f);
            std::uniform_real_distribution<float> uniformDist(0.0f, 1.0f);
            std::uniform_real_distribution<float> radiusDist(10.0f, 50.0f);

            for (uint32 i = 0; i < POINT_LIGHTS_COUNT; ++i)
            {
                gPointLightInfos.push_back(
                    PointLightInfo
                    {
                        .Positions = { -dist(gen), dist(gen), dist(gen), },
                        .Color = { uniformDist(gen), uniformDist(gen), uniformDist(gen), },
                        .Radius = radiusDist(gen),
                    }
                );
            }
        }

        {
            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { -343.0f, 548.8f - 0.001f, 332.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -343.0f, 548.8f - 0.001f, 227.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -213.0f, 548.8f - 0.001f, 227.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { -213.0f, 548.8f - 0.001f, 332.0f };
            const uint32 primitiveIndex = static_cast<uint32>(indices.size()) / 3;
            addQuadVertices(vertices, indices, v0, v1, v2, v3);

            emissiveBuffer.Position = v0;
            emissiveBuffer.Position += v1;
            emissiveBuffer.Position += v2;
            emissiveBuffer.Position += v3;
            emissiveBuffer.Position /= 4.0f;
            emissiveBuffer.Position.W = 1.0f;
            emissiveBuffer.Color = float3{1.0f, 1.0f, 1.0f};
            colors.push_back(float3{1.0f, 0.905f, 0.777f});
            colors.push_back(float3{1.0f, 0.905f, 0.777f});
            isEmissives.push_back(true);
            isEmissives.push_back(true);

            parallelogramAreaLightInfos.push_back(
                ParallelogramAreaLightInfo
                {
                    .Positions = { v0, v1, v3, },
                    .Color =  { 1.0f, 1.0f, 1.0f },
                    .PrimitiveIndices = { primitiveIndex, primitiveIndex + 1, }
                }
            );
        }

        if(renderMethod == eRenderMethod::RASTERIZATION)
        {
            const D3D12_HEAP_PROPERTIES bufferHeapProperties = 
            {
                .Type = D3D12_HEAP_TYPE_UPLOAD,
                .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
                .CreationNodeMask = 1,
                .VisibleNodeMask = 1,
            };

            const D3D12_RESOURCE_DESC emissiveBufferDesc =
            {
                .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
                .Alignment = 0,
                .Width = Align(sizeof(EmissiveBuffer), static_cast<size_t>(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)),
                .Height = 1,
                .DepthOrArraySize = 1,
                .MipLevels = 1,
                .Format = DXGI_FORMAT_UNKNOWN,
                .SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 },
                .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                .Flags = D3D12_RESOURCE_FLAG_NONE,
            };
            
            std::vector<ConstantBuffer::CreateInfo> emissiveBufferCreateInfos(BACK_BUFFERS_COUNT);
            for(uint32 i = 0; i < BACK_BUFFERS_COUNT; ++i)
            {
                ConstantBuffer::CreateInfo& emissiveBufferCreateInfo = emissiveBufferCreateInfos[i];
                emissiveBufferCreateInfo.State = D3D12_RESOURCE_STATE_GENERIC_READ;
                hr = gDevice->CreateCommittedResource(
                    &bufferHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &emissiveBufferDesc,
                    emissiveBufferCreateInfo.State,
                    nullptr,
                    IID_PPV_ARGS(emissiveBufferCreateInfo.Data.GetAddressOf())
                );
                if(FAILED(hr))
                {
                    assert(false && "Failed to create emissive buffer");
                    return false;
                }

                D3D12_RANGE range = { .Begin = 0, .End = 0 };
                byte* emissiveDataBegin = nullptr;
                hr = emissiveBufferCreateInfo.Data->Map(
                    0,
                    &range,
                    reinterpret_cast<void**>(&emissiveDataBegin)
                );
                if(FAILED(hr))
                {
                    assert(false && "Failed to map emissive buffer");
                    return false;
                }

                std::memcpy(emissiveDataBegin, &emissiveBuffer, sizeof(EmissiveBuffer));
                emissiveBufferCreateInfo.Data->Unmap(0, nullptr);
            }

#if 0
            std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> emissiveCpuDescriptors(BACK_BUFFERS_COUNT);
            std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> emissiveGpuDescriptors(BACK_BUFFERS_COUNT);
            bool result = gCbvSrvUavHeap->AllocateDynamicDescriptors(emissiveCpuDescriptors, emissiveGpuDescriptors, 0);
            if(result == false)
            {
                assert(false && "Failed to allocate dynamic descriptor for emissive buffer");
                return false;
            }
#endif
            for(uint32 i = 0; i < BACK_BUFFERS_COUNT; ++i)
            {
                ConstantBuffer::CreateInfo& emissiveBufferCreateInfo = emissiveBufferCreateInfos[i];
                
#if 0
                const D3D12_CONSTANT_BUFFER_VIEW_DESC emissiveCbvDesc = 
                {
                    .BufferLocation = emissiveBufferCreateInfo.Data->GetGPUVirtualAddress(),
                    .SizeInBytes = static_cast<uint32>(emissiveBufferDesc.Width),
                };

                emissiveBufferCreateInfo.View = emissiveCpuDescriptors[i];
                emissiveBufferCreateInfo.GpuDescriptor = emissiveGpuDescriptors[i];
                gDevice->CreateConstantBufferView(&emissiveCbvDesc, emissiveBufferCreateInfo.View);
#endif

                gEmissiveBuffers[i] = std::make_unique<ConstantBuffer>(std::move(emissiveBufferCreateInfo));
            }
        }
        else
        {
            const Camera::Buffer& cameraBuffer = mainCamera.GetBuffer();
            const float4x4 worldToProjectionMatrix = cameraBuffer.ProjectionMatrix * cameraBuffer.ViewMatrix;
            float4x4 inverseMatrix;
            GetInverse(worldToProjectionMatrix, inverseMatrix);

            sceneConstantBuffer.ProjectionToWorldTransformMatrix = inverseMatrix;
            sceneConstantBuffer.ParallelogramAreaLightInfosCount = static_cast<uint32>(parallelogramAreaLightInfos.size());
            sceneConstantBuffer.PointLightInfosCount = static_cast<uint32>(gPointLightInfos.size());
        }

        const D3D12_RESOURCE_DESC sceneConstantBufferDesc =
        {
            .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Alignment = 0,
            .Width = Align(sizeof(SceneConstantBuffer), static_cast<size_t>(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)),
            .Height = 1,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .Format = DXGI_FORMAT_UNKNOWN,
            .SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 },
            .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            .Flags = D3D12_RESOURCE_FLAG_NONE,
        };

        const D3D12_HEAP_PROPERTIES bufferHeapProperties = 
        {
            .Type = D3D12_HEAP_TYPE_UPLOAD,
            .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask = 1,
            .VisibleNodeMask = 1,
        };
        
        std::vector<ConstantBuffer::CreateInfo> sceneConstantBufferCreateInfos(BACK_BUFFERS_COUNT);
        for(uint32 i = 0; i < BACK_BUFFERS_COUNT; ++i)
        {
            ConstantBuffer::CreateInfo& sceneConstantBufferCreateInfo = sceneConstantBufferCreateInfos[i];
            sceneConstantBufferCreateInfo.State = D3D12_RESOURCE_STATE_GENERIC_READ;
            hr = gDevice->CreateCommittedResource(
                &bufferHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &sceneConstantBufferDesc,
                sceneConstantBufferCreateInfo.State,
                nullptr,
                IID_PPV_ARGS(sceneConstantBufferCreateInfo.Data.GetAddressOf())
            );
            if(FAILED(hr))
            {
                assert(false && "Failed to create scene constant buffer");
                return false;
            }

            D3D12_RANGE range = { .Begin = 0, .End = 0 };
            byte* sceneConstantBufferDataBegin = nullptr;
            hr = sceneConstantBufferCreateInfo.Data->Map(
                0,
                &range,
                reinterpret_cast<void**>(&sceneConstantBufferDataBegin)
            );
            if(FAILED(hr))
            {
                assert(false && "Failed to map scene constant buffer");
                return false;
            }

            std::memcpy(sceneConstantBufferDataBegin, &sceneConstantBuffer, sizeof(SceneConstantBuffer));
            sceneConstantBufferCreateInfo.Data->Unmap(0, nullptr);
        }
        
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> sceneConstantBufferCpuDescriptors(BACK_BUFFERS_COUNT);
        std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> sceneConstantBufferGpuDescriptors(BACK_BUFFERS_COUNT);
        result = gCbvSrvUavHeap->AllocateDynamicDescriptors(sceneConstantBufferCpuDescriptors, sceneConstantBufferGpuDescriptors, 8);
        if(result == false)
        {
            assert(false && "Failed to allocate dynamic descriptor for scene constant buffer");
            return false;
        }
        for(uint32 i = 0; i < BACK_BUFFERS_COUNT; ++i)
        {
            ConstantBuffer::CreateInfo& sceneConstantBufferCreateInfo = sceneConstantBufferCreateInfos[i];
            const D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = 
            {
                .BufferLocation = sceneConstantBufferCreateInfo.Data->GetGPUVirtualAddress(),
                .SizeInBytes = static_cast<uint32>(sceneConstantBufferDesc.Width),
            };

            sceneConstantBufferCreateInfo.CpuDescriptor = sceneConstantBufferCpuDescriptors[i];
            sceneConstantBufferCreateInfo.GpuDescriptor = sceneConstantBufferGpuDescriptors[i];
            gDevice->CreateConstantBufferView(&cbvDesc, sceneConstantBufferCreateInfo.CpuDescriptor);

            gSceneConstantBuffers[i] = std::make_unique<ConstantBuffer>(std::move(sceneConstantBufferCreateInfo));
        }

        // Ceiling
        {
            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { -556.0f, 548.8f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -556.0f, 548.8f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { 0.0f, 548.8f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 0.0f, 548.8f, 559.2f };
            addQuadVertices(vertices, indices, v0, v1, v2, v3);
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            isEmissives.push_back(false);
            isEmissives.push_back(false);
        }
        
        // Back wall
        {
            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { 0.0f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -549.6f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -556.0f, 548.8f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 0.0f, 548.8f, 559.2f };
            addQuadVertices(vertices, indices, v0, v1, v2, v3);
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            isEmissives.push_back(false);
            isEmissives.push_back(false);
        }

        // Right wall
        {
            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { 0.0f, 0.0f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { 0.0f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { 0.0f, 548.8f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 0.0f, 548.8f, 0.0f };
            addQuadVertices(vertices, indices, v0, v1, v2, v3);
            colors.push_back(float3{0.0f, 1.0f, 0.0f});
            colors.push_back(float3{0.0f, 1.0f, 0.0f});
            isEmissives.push_back(false);
            isEmissives.push_back(false);
        }
        
        // Left wall
        {
            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { -549.6f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -552.8f, 0.0f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -556.0f, 548.8f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { -556.0f, 548.8f, 559.2f };
            addQuadVertices(vertices, indices, v0, v1, v2, v3);
            colors.push_back(float3{1.0f, 0.0f, 0.0f});
            colors.push_back(float3{1.0f, 0.0f, 0.0f});
            isEmissives.push_back(false);
            isEmissives.push_back(false);
        }

        // Short block
        {
            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { -82.0f, 165.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -130.0f, 165.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -290.0f, 165.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { -240.0f, 165.0f, 272.0f };
            addQuadVertices(vertices, indices, v0, v1, v2, v3);
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            isEmissives.push_back(false);
            isEmissives.push_back(false);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v4 = { -290.0f, 165.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v5 = { -290.0f, 0.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v6 = { -240.0f, 0.0f, 272.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v7 = { -240.0f, 165.0f, 272.0f };
            addQuadVertices(vertices, indices, v4, v5, v6, v7);
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            isEmissives.push_back(false);
            isEmissives.push_back(false);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v8 = { -130.0f, 165.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v9 = { -130.0f, 0.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v10 = { -290.0f, 0.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v11 = { -290.0f, 165.0f, 114.0f };
            addQuadVertices(vertices, indices, v8, v9, v10, v11);
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            isEmissives.push_back(false);
            isEmissives.push_back(false);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v12 = { -82.0f, 165.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v13 = { -82.0f, 0.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v14 = { -130.0f, 0.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v15 = { -130.0f, 165.0f, 65.0f };
            addQuadVertices(vertices, indices, v12, v13, v14, v15);
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            isEmissives.push_back(false);
            isEmissives.push_back(false);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v16 = { -240.0f, 165.0f, 272.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v17 = { -240.0f, 0.0f, 272.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v18 = { -82.0f, 0.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v19 = { -82.0f, 165.0f, 225.0f };
            addQuadVertices(vertices, indices, v16, v17, v18, v19);
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            isEmissives.push_back(false);
            isEmissives.push_back(false);
        }

        // Tall block
        {
            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { -265.0f, 330.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -423.0f, 330.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -472.0f, 330.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { -314.0f, 330.0f, 456.0f };
            addQuadVertices(vertices, indices, v0, v1, v2, v3);
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            isEmissives.push_back(false);
            isEmissives.push_back(false);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v4 = { -423.0f, 330.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v5 = { -423.0f, 0.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v6 = { -472.0f, 0.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v7 = { -472.0f, 330.0f, 406.0f };
            addQuadVertices(vertices, indices, v4, v5, v6, v7);
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            isEmissives.push_back(false);
            isEmissives.push_back(false);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v8 = { -472.0f, 330.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v9 = { -472.0f, 0.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v10 = { -314.0f, 0.0f, 456.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v11 = { -314.0f, 330.0f, 456.0f };
            addQuadVertices(vertices, indices, v8, v9, v10, v11);
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            isEmissives.push_back(false);
            isEmissives.push_back(false);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v12 = { -314.0f, 330.0f, 456.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v13 = { -314.0f, 0.0f, 456.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v14 = { -265.0f, 0.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v15 = { -265.0f, 330.0f, 296.0f };
            addQuadVertices(vertices, indices, v12, v13, v14, v15);
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            isEmissives.push_back(false);
            isEmissives.push_back(false);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v16 = { -265.0f, 330.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v17 = { -265.0f, 0.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v18 = { -423.0f, 0.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v19 = { -423.0f, 330.0f, 247.0f };
            addQuadVertices(vertices, indices, v16, v17, v18, v19);
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            colors.push_back(float3{1.0f, 1.0f, 1.0f});
            isEmissives.push_back(false);
            isEmissives.push_back(false);
        }

        {
            VertexBuffer vertexBuffer;
            result = createVertexBuffer(vertexBuffer, vertices, "CornellBoxVertexBuffer");
            cornellBox.SetVertexBuffer(std::move(vertexBuffer));

            IndexBuffer indexBuffer;
            result = createIndexBuffer(indexBuffer, indices, "CornellBoxIndexBuffer");
            cornellBox.SetIndexBuffer(std::move(indexBuffer));

            D3D12_RESOURCE_DESC bufferDesc =
            {
                .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
                .Alignment = 0,
                .Width = static_cast<uint32>(sizeof(float3) * colors.size()),
                .Height = 1,
                .DepthOrArraySize = 1,
                .MipLevels = 1,
                .Format = DXGI_FORMAT_UNKNOWN,
                .SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 },
                .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                .Flags = D3D12_RESOURCE_FLAG_NONE,
            };

            RenderResource::CreateInfo colorsBufferCreateInfo;
            colorsBufferCreateInfo.State = D3D12_RESOURCE_STATE_GENERIC_READ;
            colorsBufferCreateInfo.Name = "CornellBoxColorsBuffer";
            hr = gDevice->CreateCommittedResource(
                &bufferHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                colorsBufferCreateInfo.State,
                nullptr,
                IID_PPV_ARGS(colorsBufferCreateInfo.Data.GetAddressOf())
            );
            if(FAILED(hr))
            {
                assert(false && "Failed to create color buffer");
                return false;
            }
            
            D3D12_RANGE range = { .Begin = 0, .End = 0 };
            byte* colorDataBegin = nullptr;
            hr = colorsBufferCreateInfo.Data->Map(
                0,
                &range,
                reinterpret_cast<void**>(&colorDataBegin)
            );
            if(FAILED(hr))
            {
                assert(false && "Failed to map color buffer");
                return false;
            }

            std::memcpy(colorDataBegin, colors.data(), sizeof(float3) * colors.size());
            colorsBufferCreateInfo.Data->Unmap(0, nullptr);

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
            {
                .Format = DXGI_FORMAT_UNKNOWN,
                .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
                .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                .Buffer = 
                {
                    .NumElements = static_cast<uint32>(colors.size()),
                    .StructureByteStride = sizeof(float3),
                    .Flags = D3D12_BUFFER_SRV_FLAG_NONE,
                },
            };
            
            uint32 descriptorIndex = renderMethod == eRenderMethod::RASTERIZATION ? static_cast<uint32>(eCbvSrvUavRasterizationDescriptorType::COLORS) : static_cast<uint32>(eCbvSrvUavRaytracingDescriptorType::COLORS);
            result = gCbvSrvUavHeap->AllocateStaticDescriptor(colorsBufferCreateInfo.CpuDescriptor, colorsBufferCreateInfo.GpuDescriptor, descriptorIndex);
            if(result == false)
            {
                assert(false && "Failed to allocate static descriptor for color buffer");
                return false;
            }
            gDevice->CreateShaderResourceView(colorsBufferCreateInfo.Data.Get(), &srvDesc, colorsBufferCreateInfo.CpuDescriptor);

            cornellBox.SetColorBuffer(RenderResource(std::move(colorsBufferCreateInfo)));
            
            if( renderMethod == eRenderMethod::RAYTRACING )
            {
                RenderResource::CreateInfo isEmissivesBufferCreateInfo;
                isEmissivesBufferCreateInfo.State = D3D12_RESOURCE_STATE_GENERIC_READ;
                isEmissivesBufferCreateInfo.Name = "CornellBoxIsEmissivesBuffer";
                bufferDesc.Width = sizeof(uint8) * isEmissives.size();
                hr = gDevice->CreateCommittedResource(
                    &bufferHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &bufferDesc,
                    isEmissivesBufferCreateInfo.State,
                    nullptr,
                    IID_PPV_ARGS(isEmissivesBufferCreateInfo.Data.GetAddressOf())
                );
                if(FAILED(hr))
                {
                    assert(false && "Failed to create is emissives buffer");
                    return false;
                }
                
                range = { .Begin = 0, .End = 0 };
                byte* isEmissivesDataBegin = nullptr;
                hr = isEmissivesBufferCreateInfo.Data->Map(
                    0,
                    &range,
                    reinterpret_cast<void**>(&isEmissivesDataBegin)
                );
                if(FAILED(hr))
                {
                    assert(false && "Failed to map is emissives buffer");
                    return false;
                }

                std::memcpy(isEmissivesDataBegin, isEmissives.data(), sizeof(uint8) * isEmissives.size());
                isEmissivesBufferCreateInfo.Data->Unmap(0, nullptr);

                srvDesc =
                {
                    .Format = DXGI_FORMAT_R32_TYPELESS,
                    .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
                    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                    .Buffer = 
                    {
                        .NumElements = static_cast<uint32>(isEmissives.size() * sizeof(uint8) / 4),
                        .StructureByteStride = 0,
                        .Flags = D3D12_BUFFER_SRV_FLAG_RAW,
                    },
                };
                // eCbvSrvUavRaytracingDescriptorType
                result = gCbvSrvUavHeap->AllocateStaticDescriptor(isEmissivesBufferCreateInfo.CpuDescriptor, isEmissivesBufferCreateInfo.GpuDescriptor, static_cast<uint32>(eCbvSrvUavRaytracingDescriptorType::IS_EMISSIVES));
                if(result == false)
                {
                    assert(false && "Failed to allocate static descriptor for is emissives buffer");
                    return false;
                }
                gDevice->CreateShaderResourceView(isEmissivesBufferCreateInfo.Data.Get(), &srvDesc, isEmissivesBufferCreateInfo.CpuDescriptor);

                cornellBox.SetIsEmissiveBuffer(RenderResource(std::move(isEmissivesBufferCreateInfo)));
            }
        }

        // Emissive Information
        if( renderMethod == eRenderMethod::RAYTRACING )
        {
            D3D12_RESOURCE_DESC bufferDesc =
            {
                .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
                .Alignment = 0,
                .Width = static_cast<uint32>(sizeof(ParallelogramAreaLightInfo) * parallelogramAreaLightInfos.size()),
                .Height = 1,
                .DepthOrArraySize = 1,
                .MipLevels = 1,
                .Format = DXGI_FORMAT_UNKNOWN,
                .SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 },
                .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                .Flags = D3D12_RESOURCE_FLAG_NONE,
            };

            RenderResource::CreateInfo parallelogramAreaLightInfosBufferCreateInfo;
            parallelogramAreaLightInfosBufferCreateInfo.State = D3D12_RESOURCE_STATE_GENERIC_READ;
            parallelogramAreaLightInfosBufferCreateInfo.Name = "CornellBoxParallelogramAreaLightInfosBuffer";
            hr = gDevice->CreateCommittedResource(
                &bufferHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                parallelogramAreaLightInfosBufferCreateInfo.State,
                nullptr,
                IID_PPV_ARGS(parallelogramAreaLightInfosBufferCreateInfo.Data.GetAddressOf())
            );
            if(FAILED(hr))
            {
                assert(false && "Failed to create parallelogram area light infos buffer");
                return false;
            }
            
            D3D12_RANGE range = { .Begin = 0, .End = 0 };
            byte* parallelogramAreaLightInfosDataBegin = nullptr;
            hr = parallelogramAreaLightInfosBufferCreateInfo.Data->Map(
                0,
                &range,
                reinterpret_cast<void**>(&parallelogramAreaLightInfosDataBegin)
            );
            if(FAILED(hr))
            {
                assert(false && "Failed to map parallelogram area light infos buffer");
                return false;
            }

            std::memcpy(parallelogramAreaLightInfosDataBegin, parallelogramAreaLightInfos.data(), sizeof(ParallelogramAreaLightInfo) * parallelogramAreaLightInfos.size());
            parallelogramAreaLightInfosBufferCreateInfo.Data->Unmap(0, nullptr);

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
            {
                .Format = DXGI_FORMAT_UNKNOWN,
                .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
                .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                .Buffer = 
                {
                    .NumElements = static_cast<uint32>(parallelogramAreaLightInfos.size()),
                    .StructureByteStride = sizeof(ParallelogramAreaLightInfo),
                    .Flags = D3D12_BUFFER_SRV_FLAG_NONE,
                },
            };
            
            uint32 descriptorIndex = static_cast<uint32>(eCbvSrvUavRaytracingDescriptorType::PARALLELOGRAM_AREA_LIGHT_INFOS);
            result = gCbvSrvUavHeap->AllocateStaticDescriptor(parallelogramAreaLightInfosBufferCreateInfo.CpuDescriptor, parallelogramAreaLightInfosBufferCreateInfo.GpuDescriptor, descriptorIndex);
            if(result == false)
            {
                assert(false && "Failed to allocate static descriptor for parallelogram area light infos buffer");
                return false;
            }
            gDevice->CreateShaderResourceView(parallelogramAreaLightInfosBufferCreateInfo.Data.Get(), &srvDesc, parallelogramAreaLightInfosBufferCreateInfo.CpuDescriptor);
            gParallelogramAreaLightInfosBuffer = std::make_unique<RenderResource>(std::move(parallelogramAreaLightInfosBufferCreateInfo));

            bufferDesc.Width = sizeof(PointLightInfo) * gPointLightInfos.size();
            RenderResource::CreateInfo pointLightInfosBufferCreateInfo;
            pointLightInfosBufferCreateInfo.State = D3D12_RESOURCE_STATE_GENERIC_READ;
            pointLightInfosBufferCreateInfo.Name = "CornellBoxPointLightInfosBuffer";
            hr = gDevice->CreateCommittedResource(
                &bufferHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                pointLightInfosBufferCreateInfo.State,
                nullptr,
                IID_PPV_ARGS(pointLightInfosBufferCreateInfo.Data.GetAddressOf())
            );
            if(FAILED(hr))
            {
                assert(false && "Failed to create point light infos buffer");
                return false;
            }
            
            range = { .Begin = 0, .End = 0 };
            byte* pointLightInfosDataBegin = nullptr;
            hr = pointLightInfosBufferCreateInfo.Data->Map(
                0,
                &range,
                reinterpret_cast<void**>(&pointLightInfosDataBegin)
            );
            if(FAILED(hr))
            {
                assert(false && "Failed to map point light infos buffer");
                return false;
            }

            std::memcpy(pointLightInfosDataBegin, gPointLightInfos.data(), sizeof(PointLightInfo) * gPointLightInfos.size());
            pointLightInfosBufferCreateInfo.Data->Unmap(0, nullptr);

            srvDesc.Buffer.NumElements = static_cast<uint32>(gPointLightInfos.size());
            srvDesc.Buffer.StructureByteStride = sizeof(PointLightInfo);
            
            descriptorIndex = static_cast<uint32>(eCbvSrvUavRaytracingDescriptorType::POINT_LIGHT_INFOS);
            result = gCbvSrvUavHeap->AllocateStaticDescriptor(pointLightInfosBufferCreateInfo.CpuDescriptor, pointLightInfosBufferCreateInfo.GpuDescriptor, descriptorIndex);
            if(result == false)
            {
                assert(false && "Failed to allocate static descriptor for point light infos buffer");
                return false;
            }
            gDevice->CreateShaderResourceView(pointLightInfosBufferCreateInfo.Data.Get(), &srvDesc, pointLightInfosBufferCreateInfo.CpuDescriptor);
            gPointLightInfosBuffer = std::make_unique<RenderResource>(std::move(pointLightInfosBufferCreateInfo));
        }

        if (result == false)
        {
            assert(false && "Failed to create vertex buffer");
            outGeometries.pop_back();
        }
        
        
        // AS
        {
            ID3D12CommandAllocator& graphicsCommandAllocator = *gGraphicsCommandAllocators[0].Get();
            hr = graphicsCommandAllocator.Reset();
            if(FAILED(hr))
            {
                assert(false && "Failed to reset command allocator");
                return false;
            }

            ID3D12GraphicsCommandList4& graphicsCommandList = *gGraphicsCommandLists[0].Get();
            hr = graphicsCommandList.Reset(&graphicsCommandAllocator, nullptr);
            if(FAILED(hr))
            {
                assert(false && "Failed to reset command list");
                return false;
            }

            std::vector<ASBuildInfo> asBuildInfos;
            const uint32 geometriesCount = static_cast<uint32>(outGeometries.size());
            asBuildInfos.reserve(geometriesCount);
            gBottomLevelAccelerationStructures.resize(geometriesCount);
            gTopLevelAccelerationStructures.resize(geometriesCount);
            for(std::unique_ptr<Geometry>& geometry : outGeometries)
            {
                if(geometry == nullptr)
                {
                    continue;
                }

                const uint32 index = static_cast<uint32>(asBuildInfos.size());
                asBuildInfos.push_back(ASBuildInfo{});
                ASBuildInfo& asBuildInfo = asBuildInfos.back();

                VertexBuffer& vertexBuffer = geometry->GetVertexBuffer();
                IndexBuffer& indexBuffer = geometry->GetIndexBuffer();

                const D3D12_INDEX_BUFFER_VIEW& ibView = indexBuffer.GetIndexBufferView();

                // Describe the geometry
                const D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = 
                {
                    .Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,
                    .Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE,
                    .Triangles = 
                    {
                        .Transform3x4 = 0,
                        .IndexFormat = DXGI_FORMAT_R16_UINT,
                        .VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT,
                        .IndexCount = static_cast<UINT>(ibView.SizeInBytes / sizeof(uint16)),
                        .VertexCount = static_cast<UINT>(vertexBuffer.GetVertexBufferView().SizeInBytes / sizeof(VertexPN)),
                        .IndexBuffer = indexBuffer.GetGPUVirtualAddress(),
                        .VertexBuffer = 
                        {
                            .StartAddress = vertexBuffer.GetGPUVirtualAddress(),
                            .StrideInBytes = sizeof(VertexPN),
                        },
                    },
                };
                
                const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
                asBuildInfo.TopLevelInputs = 
                {
                    .Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
                    .Flags = buildFlags,
                    .NumDescs = 1,
                    .DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
                };

                D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
                gDevice->GetRaytracingAccelerationStructurePrebuildInfo(&asBuildInfo.TopLevelInputs, &topLevelPrebuildInfo);
                if(topLevelPrebuildInfo.ResultDataMaxSizeInBytes <= 0)
                {
                    assert(false && "Invalid top-level AS prebuild info");
                    return false;
                }
                
                D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
                asBuildInfo.BottomLevelInputs = asBuildInfo.TopLevelInputs;
                asBuildInfo.BottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
                asBuildInfo.BottomLevelInputs.pGeometryDescs = &geometryDesc;
                gDevice->GetRaytracingAccelerationStructurePrebuildInfo(&asBuildInfo.BottomLevelInputs, &bottomLevelPrebuildInfo);
                if(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes <= 0)
                {
                    assert(false && "Invalid bottom-level AS prebuild info");
                    return false;
                }

                {
                    D3D12_HEAP_PROPERTIES heapProperties = 
                    {
                        .Type = D3D12_HEAP_TYPE_DEFAULT,
                        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
                        .CreationNodeMask = 1,
                        .VisibleNodeMask = 1,
                    };

                    D3D12_RESOURCE_DESC bufferDesc =
                    {
                        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
                        .Alignment = 0,
                        .Width = Align(std::max(bottomLevelPrebuildInfo.ScratchDataSizeInBytes, topLevelPrebuildInfo.ScratchDataSizeInBytes), static_cast<size_t>(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)),
                        .Height = 1,
                        .DepthOrArraySize = 1,
                        .MipLevels = 1,
                        .Format = DXGI_FORMAT_UNKNOWN,
                        .SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 },
                        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                        .Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                    };
                    hr = gDevice->CreateCommittedResource(
                        &heapProperties,
                        D3D12_HEAP_FLAG_NONE,
                        &bufferDesc,
                        D3D12_RESOURCE_STATE_COMMON,
                        nullptr,
                        IID_PPV_ARGS(asBuildInfo.ScratchResource.GetAddressOf())
                    );
                    if(FAILED(hr))
                    {
                        assert(false && "Failed to create scratch resource for AS");
                        return false;
                    }
                    asBuildInfo.ScratchResource->SetName(TEXT("ScratchResourceForAS"));
                    
                    // Allocate resources for acceleration structures.
                    // Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
                    // Default heap is OK since the application doesn’t need CPU read/write access to them. 
                    // The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
                    // and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
                    //  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
                    //  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
                    D3DPtr<ID3D12Resource>& bottomLevelAccelerationStructure = gBottomLevelAccelerationStructures[index];
                    D3DPtr<ID3D12Resource>& topLevelAccelerationStructure = gTopLevelAccelerationStructures[index];
                    {
                        bufferDesc.Width = Align(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, static_cast<uint64>(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
                        hr = gDevice->CreateCommittedResource(
                            &heapProperties,
                            D3D12_HEAP_FLAG_NONE,
                            &bufferDesc,
                            D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                            nullptr,
                            IID_PPV_ARGS(bottomLevelAccelerationStructure.GetAddressOf())
                        );
                        if(FAILED(hr))
                        {
                            assert(false && "Failed to create scratch resource for AS");
                            return false;
                        }
                        bottomLevelAccelerationStructure->SetName(TEXT("BottomLevelAccelerationStructure"));
                    
                        bufferDesc.Width = Align(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, static_cast<uint64>(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
                        hr = gDevice->CreateCommittedResource(
                            &heapProperties,
                            D3D12_HEAP_FLAG_NONE,
                            &bufferDesc,
                            D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                            nullptr,
                            IID_PPV_ARGS(topLevelAccelerationStructure.GetAddressOf())
                        );
                        if(FAILED(hr))
                        {
                            assert(false && "Failed to create scratch resource for AS");
                            return false;
                        }
                        topLevelAccelerationStructure->SetName(TEXT("TopLevelAccelerationStructure"));
                    }

                    // Create an instance desc for the bottom-level acceleration structure.
                    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = 
                    {
                        .InstanceMask = 1,
                        .AccelerationStructure = bottomLevelAccelerationStructure->GetGPUVirtualAddress(),
                    };
                    instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1.0f;
                    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
                    bufferDesc.Width = Align(sizeof(instanceDesc), static_cast<size_t>(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
                    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
                    hr = gDevice->CreateCommittedResource(
                        &heapProperties,
                        D3D12_HEAP_FLAG_NONE,
                        &bufferDesc,
                        D3D12_RESOURCE_STATE_GENERIC_READ,
                        nullptr,
                        IID_PPV_ARGS(asBuildInfo.InstanceDescs.GetAddressOf())
                    );
                    if(FAILED(hr))
                    {
                        assert(false && "Failed to create scratch resource for AS");
                        return false;
                    }
                    asBuildInfo.ScratchResource->SetName(TEXT("InstanceDescs"));
                    
                    void* mappedData = nullptr;
                    hr = asBuildInfo.InstanceDescs->Map(0, nullptr, &mappedData);
                    if(FAILED(hr))
                    {
                        assert(false && "Failed to map instance descs");
                        return false;
                    }
                    memcpy(mappedData, &instanceDesc, sizeof(instanceDesc));
                    asBuildInfo.InstanceDescs->Unmap(0, nullptr);

                    // Bottom Level Acceleration Structure desc
                    asBuildInfo.BottomLevelBuildDesc = 
                    {
                        .DestAccelerationStructureData = bottomLevelAccelerationStructure->GetGPUVirtualAddress(),
                        .Inputs = asBuildInfo.BottomLevelInputs,
                        .ScratchAccelerationStructureData = asBuildInfo.ScratchResource->GetGPUVirtualAddress(),
                    };

                    // Top Level Acceleration Structure desc
                    asBuildInfo.TopLevelInputs.InstanceDescs = asBuildInfo.InstanceDescs->GetGPUVirtualAddress();
                    asBuildInfo.TopLevelBuildDesc = 
                    {
                        .DestAccelerationStructureData = topLevelAccelerationStructure->GetGPUVirtualAddress(),
                        .Inputs = asBuildInfo.TopLevelInputs,
                        .ScratchAccelerationStructureData = asBuildInfo.ScratchResource->GetGPUVirtualAddress(),
                    };

                    graphicsCommandList.BuildRaytracingAccelerationStructure(&asBuildInfo.BottomLevelBuildDesc, 0, nullptr);
                    D3D12_RESOURCE_BARRIER barrier =
                    {
                        .Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
                        .UAV = 
                        {
                            .pResource = bottomLevelAccelerationStructure.Get(),
                        },
                    };
                    graphicsCommandList.ResourceBarrier(1, &barrier);
                    graphicsCommandList.BuildRaytracingAccelerationStructure(&asBuildInfo.TopLevelBuildDesc, 0, nullptr);
                }
            }

            ID3D12DescriptorHeap* heaps[] = { gCbvSrvUavHeap->GetHeap().Get(), };
            graphicsCommandList.SetDescriptorHeaps(CGS_ARRAYSIZE(heaps), heaps);
            
            for(uint32 i = 0; i < BACK_BUFFERS_COUNT; ++i)
            {
                gRaytracingParallelogramAreaLightSampleReservoirs[i]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_COPY_DEST);
                gRaytracingPointLightReservoirs[i]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_COPY_DEST);
                gRaytracingIndirectLightReservoirs[i]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_COPY_DEST);
            }

            for(uint32 i = 0; i < BACK_BUFFERS_COUNT; ++i)
            {
                graphicsCommandList.CopyBufferRegion(
                    gRaytracingParallelogramAreaLightSampleReservoirs[i]->GetResource().Get(), 0,
                    gUploadBuffers[i]->GetResource().Get(), 0,
                    sizeof(ParallelogramAreaLightReservoir) * cgs::gWidth * cgs::gHeight
                );
                graphicsCommandList.CopyBufferRegion(
                    gRaytracingPointLightReservoirs[i]->GetResource().Get(), 0,
                    gUploadBuffers[i]->GetResource().Get(), 0,
                    sizeof(PointLightReservoir) * cgs::gWidth * cgs::gHeight
                );
                graphicsCommandList.CopyBufferRegion(
                    gRaytracingIndirectLightReservoirs[i]->GetResource().Get(), 0,
                    gUploadBuffers[i]->GetResource().Get(), 0,
                    sizeof(IndirectLightReservoir) * cgs::gWidth * cgs::gHeight
                );
                graphicsCommandList.CopyBufferRegion(
                    gRaytracingPrevParallelogramAreaLightSampleReservoirs[i]->GetResource().Get(), 0,
                    gUploadBuffers[i]->GetResource().Get(), 0,
                    sizeof(ParallelogramAreaLightReservoir) * cgs::gWidth * cgs::gHeight
                );
                graphicsCommandList.CopyBufferRegion(
                    gRaytracingPrevPointLightReservoirs[i]->GetResource().Get(), 0,
                    gUploadBuffers[i]->GetResource().Get(), 0,
                    sizeof(PointLightReservoir) * cgs::gWidth * cgs::gHeight
                );
                graphicsCommandList.CopyBufferRegion(
                    gRaytracingPrevIndirectLightReservoirs[i]->GetResource().Get(), 0,
                    gUploadBuffers[i]->GetResource().Get(), 0,
                    sizeof(IndirectLightReservoir) * cgs::gWidth * cgs::gHeight
                );
            }

            for(uint32 i = 0; i < BACK_BUFFERS_COUNT; ++i)
            {
                gRaytracingParallelogramAreaLightSampleReservoirs[i]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                gRaytracingPointLightReservoirs[i]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                gRaytracingIndirectLightReservoirs[i]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            }
                    
            // Kick off acceleration structure construction.
            graphicsCommandList.Close();
            ID3D12CommandList* commandLists[] = { &graphicsCommandList, };
            gGraphicsCommandQueue->ExecuteCommandLists(CGS_ARRAYSIZE(commandLists), commandLists);
            gGraphicsCommandQueue->Signal(gFence.Get(), 0);

            // Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
            waitForFrame(0);
        }

        return true;
    }
    
    void
    Render(const float deltaTime, uint64& inoutWorkIndex, RenderThreadInfo& inoutRenderThreadInfo, const std::vector<std::unique_ptr<Geometry>>& geometries) noexcept
    {
        bool isFirstFrame = false;
        while (true)
        {
            const uint64 lastCompleteWorkIndex = inoutRenderThreadInfo.LastCompleteWorkIndex.load();
            isFirstFrame = inoutWorkIndex < cgs::BACK_BUFFERS_COUNT;
            const bool hasCompletedWork = lastCompleteWorkIndex != std::numeric_limits<uint64>::max();

            if (isFirstFrame == true || (hasCompletedWork && lastCompleteWorkIndex >= static_cast<uint64>(static_cast<int64>(inoutWorkIndex) - static_cast<int64>(cgs::BACK_BUFFERS_COUNT))))
            {
                break;
            }
            cgs::Yield();
        }

        {
            const std::lock_guard lock(inoutRenderThreadInfo.RenderWorksMutex);
            inoutRenderThreadInfo.RenderWorksPerFrame.push(
                cgs::RenderWork
                {
                    .Geometries = geometries,
                    .WorkIndex = inoutWorkIndex++,
                    .FrameIndex = 0,
                    .DeltaTime = deltaTime,
                });
        }
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
            assert(renderThreadInfo.RenderWorksPerFrame.size() <= BACK_BUFFERS_COUNT);
            if (renderThreadInfo.RenderWorksPerFrame.empty() == false)
            {
                HRESULT hr = S_OK;
                RenderWork renderWork = std::move(renderThreadInfo.RenderWorksPerFrame.front());
                renderThreadInfo.RenderWorksPerFrame.pop();
                uniqueLock.unlock();

                // Process the render work
                renderWork.FrameIndex = gSwapChain->GetCurrentBackBufferIndex();
                waitForFrame(renderWork.FrameIndex);
                renderThreadInfo.CurrentWorkIndex.store(renderWork.WorkIndex);

                D3DPtr<ID3D12CommandAllocator>& graphicsCommandAllocatorOrNull = gGraphicsCommandAllocators[renderWork.FrameIndex];
                if(graphicsCommandAllocatorOrNull == nullptr)
                {
                    assert(false && "Command allocator is null");
                    renderThreadInfo.LastCompleteWorkIndex.store(renderWork.WorkIndex);
                    continue;
                }
                ID3D12CommandAllocator& graphicsCommandAllocator = *graphicsCommandAllocatorOrNull.Get();

                D3DPtr<ID3D12GraphicsCommandList4>& graphicsCommandListOrNull = gGraphicsCommandLists[renderWork.FrameIndex];
                if(graphicsCommandListOrNull == nullptr)
                {
                    assert(false && "Command list is null");
                    renderThreadInfo.LastCompleteWorkIndex.store(renderWork.WorkIndex);
                    continue;
                }
                ID3D12GraphicsCommandList4& graphicsCommandList = *graphicsCommandListOrNull.Get();

                SceneRenderTarget& sceneRenderTarget = gSceneRenderTargets[renderWork.FrameIndex];

                // TODO(alegruz): DX ERROR: ID3D12CommandAllocator::Reset: A command allocator 0x0000017DF4B5BBA0:'Unnamed ID3D12CommandAllocator Object' is being reset before previous executions associated with the allocator have completed. [ EXECUTION ERROR #552: ]
                hr = graphicsCommandAllocator.Reset();
                if(FAILED(hr))
                {
                    assert(false && "Failed to reset command allocator");
                    renderThreadInfo.LastCompleteWorkIndex.store(renderWork.WorkIndex);
                    continue;
                }

                hr = graphicsCommandList.Reset(&graphicsCommandAllocator, gRasterizationPipelineState.Get());
                if(FAILED(hr))
                {
                    assert(false && "Failed to reset command list");
                    renderThreadInfo.LastCompleteWorkIndex.store(renderWork.WorkIndex);
                    continue;
                }

                switch (renderThreadInfo.RenderMethod)
                {
                case eRenderMethod::RASTERIZATION:
                    rasterize(graphicsCommandList, sceneRenderTarget, renderWork);
                    break;
                case eRenderMethod::RAYTRACING:
                    raytracing(graphicsCommandList, sceneRenderTarget, renderWork);
                    break;
                default:
                    assert(false && "Unknown render method");
                    renderThreadInfo.LastCompleteWorkIndex.store(renderWork.WorkIndex);
                    continue;
                };

                sceneRenderTarget.ColorBuffer.Transition(graphicsCommandList, D3D12_RESOURCE_STATE_PRESENT);

                graphicsCommandList.Close();
                ID3D12CommandList* commandLists[] = { &graphicsCommandList, };
                gGraphicsCommandQueue->ExecuteCommandLists(CGS_ARRAYSIZE(commandLists), commandLists);
                gSwapChain->Present(0, 0);
                gGraphicsCommandQueue->Signal(gFence.Get(), renderWork.WorkIndex);
                gFenceValues[renderWork.FrameIndex] = renderWork.WorkIndex;
                renderThreadInfo.LastCompleteWorkIndex.store(renderWork.WorkIndex);
            }
            else
            {
                uniqueLock.unlock();
            }
        }
    }

    bool
    createBackBufferSizeUavTextures(std::vector<std::unique_ptr<Texture>>& inoutTextures, const DXGI_FORMAT format, const std::string& name, const uint32 index) noexcept
    {
        HRESULT hr = S_OK;
        if(inoutTextures.size() < BACK_BUFFERS_COUNT)
        {
            inoutTextures.resize(BACK_BUFFERS_COUNT);
        }
        std::vector<Texture::CreateInfo> createInfos(BACK_BUFFERS_COUNT);
        for(uint32 i = 0; i < BACK_BUFFERS_COUNT; ++i)
        {
            Texture::CreateInfo& createInfo = createInfos[i];
            D3D12_HEAP_PROPERTIES heapProperties = 
            {
                .Type = D3D12_HEAP_TYPE_DEFAULT,
                .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
                .CreationNodeMask = 1,
                .VisibleNodeMask = 1,
            };

            D3D12_RESOURCE_DESC bufferDesc =
            {
                .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                .Alignment = 0,
                .Width = cgs::gWidth,
                .Height = cgs::gHeight,
                .DepthOrArraySize = 1,
                .MipLevels = 1,
                .Format = format,
                .SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 },
                .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                .Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            };
            
            createInfo.ParentCreateInfo.State = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            hr = gDevice->CreateCommittedResource(
                &heapProperties, 
                D3D12_HEAP_FLAG_NONE, 
                &bufferDesc, 
                createInfo.ParentCreateInfo.State, 
                nullptr, 
                IID_PPV_ARGS(createInfo.ParentCreateInfo.Data.GetAddressOf()));
            if(FAILED(hr))
            {
                assert(false && "Failed to create raytracing output resource");
                return false;
            }
            createInfo.ParentCreateInfo.Name = name + "[" + std::to_string(i) + "]";
        }

        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> cpuDescriptorHandles(BACK_BUFFERS_COUNT);
        std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> gpuDescriptorHandles(BACK_BUFFERS_COUNT);
        const bool result = gCbvSrvUavHeap->AllocateDynamicDescriptors(cpuDescriptorHandles, gpuDescriptorHandles, index);
        if(result == false)
        {
            assert(false && "Failed to allocate dynamic descriptor for raytracing output buffer");
            return false;
        }
        for(uint32 i = 0; i < BACK_BUFFERS_COUNT; ++i)
        {
            Texture::CreateInfo& createInfo = createInfos[i];

            createInfo.ParentCreateInfo.CpuDescriptor = cpuDescriptorHandles[i];
            createInfo.ParentCreateInfo.UavView.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            gDevice->CreateUnorderedAccessView(createInfo.ParentCreateInfo.Data.Get(), nullptr, &createInfo.ParentCreateInfo.UavView, createInfo.ParentCreateInfo.CpuDescriptor);
            createInfo.ParentCreateInfo.GpuDescriptor = gpuDescriptorHandles[i];
            inoutTextures[i] = std::make_unique<Texture>(std::move(createInfo));
        }
        return true;
    }

    template<typename T>
    bool
    createBackBufferSizeUavBuffers(std::vector<std::unique_ptr<RenderResource>>& inoutBuffers, const std::string& name, const uint32 index) noexcept
    {
        HRESULT hr = S_OK;
        if(inoutBuffers.size() < BACK_BUFFERS_COUNT)
        {
            inoutBuffers.resize(BACK_BUFFERS_COUNT);
        }
        std::vector<RenderResource::CreateInfo> createInfos(BACK_BUFFERS_COUNT);
        for(uint32 i = 0; i < BACK_BUFFERS_COUNT; ++i)
        {
            RenderResource::CreateInfo& createInfo = createInfos[i];
            D3D12_HEAP_PROPERTIES heapProperties = 
            {
                .Type = D3D12_HEAP_TYPE_DEFAULT,
                .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
                .CreationNodeMask = 1,
                .VisibleNodeMask = 1,
            };
            
            D3D12_RESOURCE_DESC bufferDesc =
            {
                .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
                .Alignment = 0,
                .Width = static_cast<uint32>(sizeof(T) * cgs::gWidth * cgs::gHeight),
                .Height = 1,
                .DepthOrArraySize = 1,
                .MipLevels = 1,
                .Format = DXGI_FORMAT_UNKNOWN,
                .SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 },
                .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                .Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            };
            
            createInfo.State = D3D12_RESOURCE_STATE_COMMON;
            hr = gDevice->CreateCommittedResource(
                &heapProperties, 
                D3D12_HEAP_FLAG_NONE, 
                &bufferDesc, 
                createInfo.State, 
                nullptr, 
                IID_PPV_ARGS(createInfo.Data.GetAddressOf()));
            if(FAILED(hr))
            {
                assert(false && "Failed to create raytracing output resource");
                return false;
            }
            createInfo.Name = name + "[" + std::to_string(i) + "]";
        }

        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> cpuDescriptorHandles(BACK_BUFFERS_COUNT);
        std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> gpuDescriptorHandles(BACK_BUFFERS_COUNT);
        bool result = gCbvSrvUavHeap->AllocateDynamicDescriptors(cpuDescriptorHandles, gpuDescriptorHandles, index);
        if(result == false)
        {
            assert(false && "Failed to allocate dynamic descriptor for raytracing output buffer");
            return false;
        }

        for(uint32 i = 0; i < BACK_BUFFERS_COUNT; ++i)
        {
            RenderResource::CreateInfo& createInfo = createInfos[i];

            createInfo.CpuDescriptor = cpuDescriptorHandles[i];
            createInfo.UavView.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            createInfo.UavView.Buffer = 
            {
                .NumElements = cgs::gWidth * cgs::gHeight,
                .StructureByteStride = sizeof(T),
            };
            gDevice->CreateUnorderedAccessView(createInfo.Data.Get(), nullptr, &createInfo.UavView, createInfo.CpuDescriptor);
            createInfo.GpuDescriptor = gpuDescriptorHandles[i];
            inoutBuffers[i] = std::make_unique<RenderResource>(std::move(createInfo));
        }
        return true;
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

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};
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
            D3DPtr<ID3D12Device> testDevice;
            hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(testDevice.GetAddressOf()));
            if(SUCCEEDED(hr))
            {
                hr = testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData));
                if(FAILED(hr))
                {
                    continue;
                }

                if(featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
                {
                    gAdapter = adapter;
                    break;
                }
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
            gDevice->SetName(TEXT("Main D3D12 Device"));
        }

#if defined(CGS_DEBUG)
        hr = gDevice->QueryInterface(IID_PPV_ARGS(gD3D12InfoQueue.GetAddressOf()));
        if(FAILED(hr))
        {
            assert(false && "Failed to get D3D12 Info Queue");
            return false;
        }

        gD3D12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        gD3D12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        gD3D12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
#endif  // defined(CGS_DEBUG)

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
        gGraphicsCommandQueue->SetName(TEXT("Main Graphics Command Queue"));

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
        gRtvHeap->SetName(TEXT("Main RTV Descriptor Heap"));

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
        gDsvHeap->SetName(TEXT("Main DSV Descriptor Heap"));

        DescriptorHeap::CreateInfo cbvSrvUavHeapCreateInfo = {};
        cbvSrvUavHeapCreateInfo.HeapType |= DescriptorHeap::eTypeBit::CONSTANT_BUFFER_SHADER_RESOURCE_UNORDERED_ACCESS;
        cbvSrvUavHeapCreateInfo.DescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc =
        {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = MAX_DESCRIPTORS_COUNT,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            .NodeMask = 0,
        };
        hr = gDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(cbvSrvUavHeapCreateInfo.Heap.GetAddressOf()));
        if (FAILED(hr))
        {
            assert(false && "Failed to create CBV descriptor heap");
            return false;
        }
        cbvSrvUavHeapCreateInfo.Heap->SetName(TEXT("Main CBV Descriptor Heap"));
        cbvSrvUavHeapCreateInfo.MaxStaticDescriptorsCount = GLOBAL_CBV_SRV_UAV_DESCRIPTORS_COUNT;
        cbvSrvUavHeapCreateInfo.MaxDynamicDescriptorsCount = PER_FRAME_CBV_SRV_UAV_DESCRIPTORS_COUNT;
        cbvSrvUavHeapCreateInfo.DynamicBlocksCount = BACK_BUFFERS_COUNT;
        gCbvSrvUavHeap = std::make_unique<DescriptorHeap>(std::move(cbvSrvUavHeapCreateInfo));

        DescriptorHeap::CreateInfo cbvSrvUavHeapOnlyCpuCreateInfo = {};
        cbvSrvUavHeapOnlyCpuCreateInfo.HeapType |= DescriptorHeap::eTypeBit::CONSTANT_BUFFER_SHADER_RESOURCE_UNORDERED_ACCESS;
        cbvSrvUavHeapOnlyCpuCreateInfo.DescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        hr = gDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(cbvSrvUavHeapOnlyCpuCreateInfo.Heap.GetAddressOf()));
        if (FAILED(hr))
        {
            assert(false && "Failed to create CPU-visible CBV descriptor heap for");
            return false;
        }
        cbvSrvUavHeapOnlyCpuCreateInfo.Heap->SetName(TEXT("Main CPU-visible CBV Descriptor Heap"));
        cbvSrvUavHeapOnlyCpuCreateInfo.MaxStaticDescriptorsCount = GLOBAL_CBV_SRV_UAV_DESCRIPTORS_COUNT;
        cbvSrvUavHeapOnlyCpuCreateInfo.MaxDynamicDescriptorsCount = PER_FRAME_CBV_SRV_UAV_DESCRIPTORS_COUNT;
        cbvSrvUavHeapOnlyCpuCreateInfo.DynamicBlocksCount = BACK_BUFFERS_COUNT;
        gCbvSrvUavHeapOnlyCpu = std::make_unique<DescriptorHeap>(std::move(cbvSrvUavHeapOnlyCpuCreateInfo));

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = 
        {
            .Width = createInfo.Width,
            .Height = createInfo.Height,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .Stereo = FALSE,
            .SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
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
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
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
            colorBufferInfo.IsBackBuffer = true;

            hr = gSwapChain->GetBuffer(frameIndex, IID_PPV_ARGS(colorBufferInfo.ParentCreateInfo.Data.GetAddressOf()));
            if(FAILED(hr))
            {
                assert(false && "Failed to get swap chain buffer");
                return false;
            }

            colorBufferInfo.ParentCreateInfo.CpuDescriptor.ptr = rtvStartHandle.ptr + (frameIndex * gRtvIncrementSize);
            colorBufferInfo.ParentCreateInfo.State = D3D12_RESOURCE_STATE_COMMON;    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12#initial-states-for-resources
            colorBufferInfo.ParentCreateInfo.Name = "SwapChainColorBuffer[" + std::to_string(frameIndex) + "]";
            gDevice->CreateRenderTargetView(colorBufferInfo.ParentCreateInfo.Data.Get(), &colorBufferViewDesc, colorBufferInfo.ParentCreateInfo.CpuDescriptor);

            gSceneRenderTargets[frameIndex].ColorBuffer.Initialize(std::move(colorBufferInfo));

            Texture::CreateInfo depthBufferInfo;
            depthBufferInfo.ParentCreateInfo.State = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            depthBufferInfo.ParentCreateInfo.Name = "SwapChainDepthBuffer[" + std::to_string(frameIndex) + "]";
            hr = gDevice->CreateCommittedResource(
                &depthBufferHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &depthBufferDesc,
                depthBufferInfo.ParentCreateInfo.State,
                &depthBufferClearValue,
                IID_PPV_ARGS(depthBufferInfo.ParentCreateInfo.Data.GetAddressOf())
            );
            if (FAILED(hr))
            {
                assert(false && "Failed to create depth buffer");
                return false;
            }

            depthBufferInfo.ParentCreateInfo.CpuDescriptor.ptr = dsvStartHandle.ptr + (frameIndex * gDsvIncrementSize);

            gDevice->CreateDepthStencilView(depthBufferInfo.ParentCreateInfo.Data.Get(), nullptr, depthBufferInfo.ParentCreateInfo.CpuDescriptor);

            gSceneRenderTargets[frameIndex].DepthBuffer.Initialize(std::move(depthBufferInfo));
        }

        for(uint32 frameBufferIndex = 0; frameBufferIndex < BACK_BUFFERS_COUNT; ++frameBufferIndex)
        {
            hr = gDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(gGraphicsCommandAllocators[frameBufferIndex].GetAddressOf()));
            if (FAILED(hr))
            {
                assert(false && "Failed to create command allocator");
                return false;
            }
            const std::wstring allocatorName = L"Graphics Command Allocator [" + std::to_wstring(frameBufferIndex) + L"]";
            gGraphicsCommandAllocators[frameBufferIndex]->SetName(allocatorName.c_str());
        }

        for (uint32 frameBufferIndex = 0; frameBufferIndex < BACK_BUFFERS_COUNT; ++frameBufferIndex)
        {
            {
                D3DPtr<ID3D12GraphicsCommandList> commandList;
                hr = gDevice->CreateCommandList1(
                    0,
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    D3D12_COMMAND_LIST_FLAG_NONE,
                    IID_PPV_ARGS(commandList.GetAddressOf())
                );
                if (FAILED(hr))
                {
                    assert(false && "Failed to create command list");
                    return false;
                }

                hr = commandList->QueryInterface(IID_PPV_ARGS(gGraphicsCommandLists[frameBufferIndex].GetAddressOf()));
                if (FAILED(hr))
                {
                    assert(false && "Failed to get command list interface");
                    return false;
                }
            }

            const std::wstring commandListName = L"Main Graphics Command List [" + std::to_wstring(frameBufferIndex) + L"]";
            gGraphicsCommandLists[frameBufferIndex]->SetName(commandListName.c_str());
        }
        
        hr = gDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(gFence.GetAddressOf()));
        if (FAILED(hr))
        {
            assert(false && "Failed to create frame fence");
            return false;
        }
        gFence->SetName(TEXT("Main Frame Fence"));

        gFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (gFenceEvent == NULL)
        {
            assert(false && "Failed to create fence event");
            return false;
        }

        bool result = createShaders(createInfo.RenderMethod);
        if(result == false)
        {
            assert(false && "Failed to create shaders");
            return false;
        }

        if(createInfo.RenderMethod == eRenderMethod::RAYTRACING)
        {       
            // Create the output resource. The dimensions and format should match the swap-chain.
            uint32 descriptorIndex = 0;
            createBackBufferSizeUavTextures(gRaytracingOutputs, DXGI_FORMAT_R8G8B8A8_UNORM, "RaytracingOutput", descriptorIndex++);
            createBackBufferSizeUavTextures(gRaytracingGBuffers, DXGI_FORMAT_R32G32B32A32_FLOAT, "RaytracingGBuffer", descriptorIndex++);

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
                .Width = cgs::gWidth * cgs::gHeight * sizeof(ParallelogramAreaLightReservoir),
                .Height = 1,
                .DepthOrArraySize = 1,
                .MipLevels = 1,
                .Format = DXGI_FORMAT_UNKNOWN,
                .SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 },
                .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                .Flags = D3D12_RESOURCE_FLAG_NONE,
            };

            for(uint32 i = 0; i < BACK_BUFFERS_COUNT; ++i)
            {
                RenderResource::CreateInfo uploadBufferCreateInfo;
                uploadBufferCreateInfo.State = D3D12_RESOURCE_STATE_GENERIC_READ;
                uploadBufferCreateInfo.Name = "UploadBuffer[" + std::to_string(i) + "]";
                hr = gDevice->CreateCommittedResource(
                    &bufferHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &bufferDesc,
                    uploadBufferCreateInfo.State,
                    nullptr,
                    IID_PPV_ARGS(uploadBufferCreateInfo.Data.GetAddressOf())
                );
                if(FAILED(hr))
                {
                    assert(false && "Failed to create upload buffer");
                    return false;
                }
                
                D3D12_RANGE range = { .Begin = 0, .End = 0 };
                byte* uploadBufferDataBegin = nullptr;
                hr = uploadBufferCreateInfo.Data->Map(
                    0,
                    &range,
                    reinterpret_cast<void**>(&uploadBufferDataBegin)
                );
                if(FAILED(hr))
                {
                    assert(false && "Failed to map upload buffer");
                    return false;
                }

                std::memset(uploadBufferDataBegin, 0, static_cast<size_t>(bufferDesc.Width));
                uploadBufferCreateInfo.Data->Unmap(0, nullptr);

                gUploadBuffers[i] = std::make_unique<RenderResource>(std::move(uploadBufferCreateInfo));
            }

            createBackBufferSizeUavBuffers<ParallelogramAreaLightReservoir>(gRaytracingParallelogramAreaLightSampleReservoirs, "RaytracingParallelogramAreaLightReservoir", descriptorIndex++);
            createBackBufferSizeUavBuffers<PointLightReservoir>(gRaytracingPointLightReservoirs, "RaytracingPointLightReservoir", descriptorIndex++);
            createBackBufferSizeUavBuffers<IndirectLightReservoir>(gRaytracingIndirectLightReservoirs, "RaytracingIndirectLightReservoir", descriptorIndex++);
            createBackBufferSizeUavBuffers<ParallelogramAreaLightReservoir>(gRaytracingPrevParallelogramAreaLightSampleReservoirs, "RaytracingPrevParallelogramAreaLightReservoir", descriptorIndex++);
            createBackBufferSizeUavBuffers<PointLightReservoir>(gRaytracingPrevPointLightReservoirs, "RaytracingPrevPointLightReservoir", descriptorIndex++);
            createBackBufferSizeUavBuffers<IndirectLightReservoir>(gRaytracingPrevIndirectLightReservoirs, "RaytracingPrevIndirectLightReservoir", descriptorIndex++);
        }

        gGlobalRenderContext.RenderDeviceType = eRenderDeviceType::D3D12;
        return true;
    }

    void
    rasterize(ID3D12GraphicsCommandList& graphicsCommandList, SceneRenderTarget& sceneRenderTarget, RenderWork& renderWork) noexcept
    {
        graphicsCommandList.SetGraphicsRootSignature(gRasterizationRootSignature.Get());

        ID3D12DescriptorHeap* heaps[] = { gCbvSrvUavHeap->GetHeap().Get(), };
        graphicsCommandList.SetDescriptorHeaps(CGS_ARRAYSIZE(heaps), heaps);

        graphicsCommandList.SetGraphicsRootConstantBufferView(0, gCameraBuffers[renderWork.FrameIndex]->GetGPUVirtualAddress());
        graphicsCommandList.SetGraphicsRootConstantBufferView(1, gEmissiveBuffers[renderWork.FrameIndex]->GetGPUVirtualAddress());
        const D3D12_VIEWPORT viewport =
        {
            .TopLeftX = 0.0f,
            .TopLeftY = 0.0f,
            .Width = static_cast<float>(sceneRenderTarget.ColorBuffer.GetWidth()),
            .Height = static_cast<float>(sceneRenderTarget.ColorBuffer.GetHeight()),
            .MinDepth = 0.0f,
            .MaxDepth = 1.0f,
        };
        graphicsCommandList.RSSetViewports(1, &viewport);
        const D3D12_RECT scissorRect = 
        { 
            .left = 0, 
            .top = 0, 
            .right = static_cast<LONG>(sceneRenderTarget.ColorBuffer.GetWidth()), 
            .bottom = static_cast<LONG>(sceneRenderTarget.ColorBuffer.GetHeight()) 
        };
        graphicsCommandList.RSSetScissorRects(1, &scissorRect);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { sceneRenderTarget.ColorBuffer.GetCpuDescriptor(), };
        graphicsCommandList.OMSetRenderTargets(CGS_ARRAYSIZE(rtvHandles), rtvHandles, FALSE, &sceneRenderTarget.DepthBuffer.GetCpuDescriptor());

        sceneRenderTarget.ColorBuffer.Transition(graphicsCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        constexpr float BLACK_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        graphicsCommandList.ClearRenderTargetView(
            sceneRenderTarget.ColorBuffer.GetCpuDescriptor(),
            BLACK_COLOR,
            0,
            nullptr
        );
        graphicsCommandList.ClearDepthStencilView(
            sceneRenderTarget.DepthBuffer.GetCpuDescriptor(),
            D3D12_CLEAR_FLAG_DEPTH,
            1.0f,
            0,
            0,
            nullptr
        );

        graphicsCommandList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        for(const std::unique_ptr<Geometry>& geometry : renderWork.Geometries)
        {
            if(geometry == nullptr)
            {
                assert(false && "Geometry is null");
                continue;
            }
            
            RenderResource& colorBuffer = geometry->GetColorBuffer();
            graphicsCommandList.SetGraphicsRootShaderResourceView(2, colorBuffer.GetGPUVirtualAddress());

            const VertexBuffer& vertexBuffer = geometry->GetVertexBuffer();
            const IndexBuffer& indexBuffer = geometry->GetIndexBuffer();

            const D3D12_VERTEX_BUFFER_VIEW& vbView = vertexBuffer.GetVertexBufferView();
            graphicsCommandList.IASetVertexBuffers(0, 1, &vbView);

            const D3D12_INDEX_BUFFER_VIEW& ibView = indexBuffer.GetIndexBufferView();
            graphicsCommandList.IASetIndexBuffer(&ibView);

            // TODO(alegruz): Set per-object constants (like world matrix)

            graphicsCommandList.DrawIndexedInstanced(
                static_cast<UINT>(ibView.SizeInBytes / sizeof(uint16)), // IndexCountPerInstance
                1,                                                      // InstanceCount
                0,                                                      // StartIndexLocation
                0,                                                      // BaseVertexLocation
                0                                                       // StartInstanceLocation
            );
        }
    }
    
    void
    raytracing(ID3D12GraphicsCommandList4& graphicsCommandList, SceneRenderTarget& sceneRenderTarget, RenderWork& renderWork) noexcept
    {
        const uint32 pointLightInfosCount = static_cast<uint32>(gPointLightInfos.size());
        for(uint32 lightIndex = 0; lightIndex < pointLightInfosCount; ++lightIndex)
        {
            PointLightInfo& pointLightInfo = gPointLightInfos[lightIndex];
            // Rotate the point light position around the rotation axis
            const float angle = renderWork.DeltaTime * 0.001f; // DeltaTime is in ms, convert to seconds
            const float3 pos = pointLightInfo.Positions;
            const float3 axis = Normalize(gRotationAxis);

            // Rodrigues' rotation formula
            const float cosA = std::cos(angle);
            const float sinA = std::sin(angle);
            const float3 rotatedPos =
                pos * cosA +
                axis * Dot(axis, pos) * (1.0f - cosA) +
                Cross(axis, pos) * sinA;

            pointLightInfo.Positions = rotatedPos;
        }
        gPointLightInfosBuffer->Map(static_cast<uint32>(sizeof(PointLightInfo) * pointLightInfosCount), gPointLightInfos.data());
            
        if(gRisPipeline == nullptr)
        {
            assert(false && "GBuffer pipeline is null");
            return;
        }
        graphicsCommandList.SetComputeRootSignature(gRisPipeline->GetGlobalRootSignature().Get());

        // Bind the heaps, acceleration structure and dispatch rays.
        ID3D12DescriptorHeap* heaps[] = { gCbvSrvUavHeap->GetHeap().Get(), };
        graphicsCommandList.SetDescriptorHeaps(CGS_ARRAYSIZE(heaps), heaps);

        graphicsCommandList.SetComputeRootDescriptorTable(0, gRaytracingOutputs[renderWork.FrameIndex]->GetGpuDescriptor());
        graphicsCommandList.SetComputeRootConstantBufferView(2, gSceneConstantBuffers[renderWork.FrameIndex]->GetGPUVirtualAddress());
        graphicsCommandList.SetComputeRootDescriptorTable(4, gParallelogramAreaLightInfosBuffer->GetGpuDescriptor());
        
        struct PushConstant final
        {
            uint32 FrameIndex = 0;
            uint32 BoundDepth = 0;
        };
        const PushConstant pushConstant = 
        {
            .FrameIndex = static_cast<uint32>(renderWork.WorkIndex),
            .BoundDepth = 3,
        };
        graphicsCommandList.SetComputeRoot32BitConstants(5, sizeof(PushConstant) / sizeof(uint32), &pushConstant, 0);

        {
            const D3D12_DISPATCH_RAYS_DESC dispatchDesc = 
            {
                .RayGenerationShaderRecord =
                {
                    .StartAddress = gRisPipeline->GetRayGenShaderTable()->GetGPUVirtualAddress(),
                    .SizeInBytes = gRisPipeline->GetRayGenShaderTable()->GetDesc().Width,
                },
                .MissShaderTable = 
                {
                    .StartAddress = gRisPipeline->GetMissShaderTable()->GetGPUVirtualAddress(),
                    .SizeInBytes = gRisPipeline->GetMissShaderTable()->GetDesc().Width,
                    .StrideInBytes = gRisPipeline->GetMissShaderTable()->GetDesc().Width,
                },
                .HitGroupTable = 
                {
                    .StartAddress = gRisPipeline->GetHitGroupShaderTable()->GetGPUVirtualAddress(),
                    .SizeInBytes = gRisPipeline->GetHitGroupShaderTable()->GetDesc().Width,
                    .StrideInBytes = gRisPipeline->GetHitGroupShaderTable()->GetDesc().Width,
                },
                .Width = sceneRenderTarget.ColorBuffer.GetWidth(),
                .Height = sceneRenderTarget.ColorBuffer.GetHeight(),
                .Depth = 1,
            };
            graphicsCommandList.SetPipelineState1(gRisPipeline->GetStateObject().Get());

            const uint32 geometriesCount = static_cast<uint32>(renderWork.Geometries.size());
            for(uint32 i = 0; i < geometriesCount; ++i)
        {
            const std::unique_ptr<Geometry>& geometry = renderWork.Geometries[i];
            if(geometry == nullptr)
            {
                assert(false && "Geometry is null");
                continue;
            }

            graphicsCommandList.SetComputeRootShaderResourceView(1, gTopLevelAccelerationStructures[i]->GetGPUVirtualAddress());

            IndexBuffer& indexBuffer = geometry->GetIndexBuffer();
            graphicsCommandList.SetComputeRootDescriptorTable(3, indexBuffer.GetGpuDescriptor());
            graphicsCommandList.DispatchRays(&dispatchDesc);
        }
        }

        {
            const D3D12_DISPATCH_RAYS_DESC dispatchDesc = 
            {
                .RayGenerationShaderRecord =
                {
                    .StartAddress = gTemporalResamplingPipeline->GetRayGenShaderTable()->GetGPUVirtualAddress(),
                    .SizeInBytes = gTemporalResamplingPipeline->GetRayGenShaderTable()->GetDesc().Width,
                },
                .MissShaderTable = 
                {
                    .StartAddress = gTemporalResamplingPipeline->GetMissShaderTable()->GetGPUVirtualAddress(),
                    .SizeInBytes = gTemporalResamplingPipeline->GetMissShaderTable()->GetDesc().Width,
                    .StrideInBytes = gTemporalResamplingPipeline->GetMissShaderTable()->GetDesc().Width,
                },
                .HitGroupTable = 
                {
                    .StartAddress = gTemporalResamplingPipeline->GetHitGroupShaderTable()->GetGPUVirtualAddress(),
                    .SizeInBytes = gTemporalResamplingPipeline->GetHitGroupShaderTable()->GetDesc().Width,
                    .StrideInBytes = gTemporalResamplingPipeline->GetHitGroupShaderTable()->GetDesc().Width,
                },
                .Width = sceneRenderTarget.ColorBuffer.GetWidth(),
                .Height = sceneRenderTarget.ColorBuffer.GetHeight(),
                .Depth = 1,
            };
            graphicsCommandList.SetPipelineState1(gTemporalResamplingPipeline->GetStateObject().Get());

            const uint32 geometriesCount = static_cast<uint32>(renderWork.Geometries.size());
            for(uint32 i = 0; i < geometriesCount; ++i)
            {
                const std::unique_ptr<Geometry>& geometry = renderWork.Geometries[i];
                if(geometry == nullptr)
                {
                    assert(false && "Geometry is null");
                    continue;
                }

                graphicsCommandList.SetComputeRootShaderResourceView(1, gTopLevelAccelerationStructures[i]->GetGPUVirtualAddress());

                IndexBuffer& indexBuffer = geometry->GetIndexBuffer();
                graphicsCommandList.SetComputeRootDescriptorTable(3, indexBuffer.GetGpuDescriptor());
                graphicsCommandList.DispatchRays(&dispatchDesc);
            }
        }
        

        {
            const D3D12_DISPATCH_RAYS_DESC dispatchDesc = 
            {
                .RayGenerationShaderRecord =
                {
                    .StartAddress = gSpatialResamplingPipeline->GetRayGenShaderTable()->GetGPUVirtualAddress(),
                    .SizeInBytes = gSpatialResamplingPipeline->GetRayGenShaderTable()->GetDesc().Width,
                },
                .MissShaderTable = 
                {
                    .StartAddress = gSpatialResamplingPipeline->GetMissShaderTable()->GetGPUVirtualAddress(),
                    .SizeInBytes = gSpatialResamplingPipeline->GetMissShaderTable()->GetDesc().Width,
                    .StrideInBytes = gSpatialResamplingPipeline->GetMissShaderTable()->GetDesc().Width,
                },
                .HitGroupTable = 
                {
                    .StartAddress = gSpatialResamplingPipeline->GetHitGroupShaderTable()->GetGPUVirtualAddress(),
                    .SizeInBytes = gSpatialResamplingPipeline->GetHitGroupShaderTable()->GetDesc().Width,
                    .StrideInBytes = gSpatialResamplingPipeline->GetHitGroupShaderTable()->GetDesc().Width,
                },
                .Width = sceneRenderTarget.ColorBuffer.GetWidth(),
                .Height = sceneRenderTarget.ColorBuffer.GetHeight(),
                .Depth = 1,
            };
            graphicsCommandList.SetPipelineState1(gSpatialResamplingPipeline->GetStateObject().Get());

            const uint32 geometriesCount = static_cast<uint32>(renderWork.Geometries.size());
            for(uint32 i = 0; i < geometriesCount; ++i)
            {
                const std::unique_ptr<Geometry>& geometry = renderWork.Geometries[i];
                if(geometry == nullptr)
                {
                    assert(false && "Geometry is null");
                    continue;
                }

                graphicsCommandList.SetComputeRootShaderResourceView(1, gTopLevelAccelerationStructures[i]->GetGPUVirtualAddress());

                IndexBuffer& indexBuffer = geometry->GetIndexBuffer();
                graphicsCommandList.SetComputeRootDescriptorTable(3, indexBuffer.GetGpuDescriptor());
                graphicsCommandList.DispatchRays(&dispatchDesc);
            }
        }
        sceneRenderTarget.ColorBuffer.Transition(graphicsCommandList, D3D12_RESOURCE_STATE_COPY_DEST);
        gRaytracingOutputs[renderWork.FrameIndex]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_COPY_SOURCE);

        graphicsCommandList.CopyResource(sceneRenderTarget.ColorBuffer.GetResource().Get(), gRaytracingOutputs[renderWork.FrameIndex]->GetResource().Get());

        gRaytracingOutputs[renderWork.FrameIndex]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        const uint32 nextFrameIndex = (renderWork.FrameIndex + 1) % BACK_BUFFERS_COUNT;
        gRaytracingPrevParallelogramAreaLightSampleReservoirs[nextFrameIndex]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_COPY_DEST);
        gRaytracingParallelogramAreaLightSampleReservoirs[renderWork.FrameIndex]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        gRaytracingPrevPointLightReservoirs[nextFrameIndex]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_COPY_DEST);
        gRaytracingPointLightReservoirs[renderWork.FrameIndex]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        gRaytracingPrevIndirectLightReservoirs[nextFrameIndex]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_COPY_DEST);
        gRaytracingIndirectLightReservoirs[renderWork.FrameIndex]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_COPY_SOURCE);

        graphicsCommandList.CopyResource(gRaytracingPrevParallelogramAreaLightSampleReservoirs[nextFrameIndex]->GetResource().Get(), gRaytracingParallelogramAreaLightSampleReservoirs[renderWork.FrameIndex]->GetResource().Get());
        graphicsCommandList.CopyResource(gRaytracingPrevPointLightReservoirs[nextFrameIndex]->GetResource().Get(), gRaytracingPointLightReservoirs[renderWork.FrameIndex]->GetResource().Get());
        graphicsCommandList.CopyResource(gRaytracingPrevIndirectLightReservoirs[nextFrameIndex]->GetResource().Get(), gRaytracingIndirectLightReservoirs[renderWork.FrameIndex]->GetResource().Get());

        gRaytracingPrevParallelogramAreaLightSampleReservoirs[nextFrameIndex]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        gRaytracingParallelogramAreaLightSampleReservoirs[renderWork.FrameIndex]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        gRaytracingPrevPointLightReservoirs[nextFrameIndex]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        gRaytracingPointLightReservoirs[renderWork.FrameIndex]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        gRaytracingPrevIndirectLightReservoirs[nextFrameIndex]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        gRaytracingIndirectLightReservoirs[renderWork.FrameIndex]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    void waitForFrame(const uint32 frameIndex) noexcept
    {
        const uint64 fenceValue = gFenceValues[frameIndex];
        if (fenceValue == std::numeric_limits<uint64>::max())
        {
            return;
        }

        const uint64 completedFenceValue = gFence->GetCompletedValue();
        if (completedFenceValue < fenceValue)
        {
            HRESULT hr = gFence->SetEventOnCompletion(fenceValue, gFenceEvent);
            if (FAILED(hr))
            {
                assert(false && "Failed to set event on fence completion");
                return;
            }
            WaitForSingleObject(gFenceEvent, INFINITE);
        }
    }
}
#endif  // defined(CGS_GRAPHICS_API_D3D12)