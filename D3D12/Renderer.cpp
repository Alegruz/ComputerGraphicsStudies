#include "pch.hpp"

#include "Common/Renderer.cpp"

#if defined(CGS_GRAPHICS_API_D3D12)
#include "D3D12/Renderer.h"

namespace cgs
{
    static GlobalRenderContext gGlobalRenderContext;

    using TDXGIGetDebugInterface = HRESULT (*)(REFIID, void**);

    enum class GlobalRootSignatureParams : uint8
    {
        OUTPUT_VIEW_SLOT = 0,
        ACCELERATION_STRUCTURE_SLOT,
        COUNT,
    };

    enum class LocalRootSignatureParams : uint8
    {
        VIEWPORT_CONSTANT_SLOT = 0,
        COUNT,
    };

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

    struct Viewport final
    {
        float Left;
        float Top;
        float Right;
        float Bottom;
    };

    struct GpuRenderWork final
    {
        SceneRenderTarget& InoutRenderTarget;
        RenderWork& Work;
    };

    struct RayGenConstantBuffer final
    {
        Viewport CurrentViewport;
        Viewport CurrentStencil;
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
    static D3DPtr<ID3D12InfoQueue1> gD3D12InfoQueue;
#endif  // defined(CGS_DEBUG)
    static D3DPtr<ID3D12Device5> gDevice;
    static std::vector<D3DPtr<ID3D12GraphicsCommandList4>> gGraphicsCommandLists(BACK_BUFFERS_COUNT);
    static std::vector<D3DPtr<ID3D12CommandAllocator>> gGraphicsCommandAllocators(BACK_BUFFERS_COUNT);
    static D3DPtr<ID3D12CommandQueue> gGraphicsCommandQueue;
    static D3DPtr<ID3D12DescriptorHeap> gRtvHeap;
    static D3DPtr<ID3D12DescriptorHeap> gDsvHeap;
    static D3DPtr<ID3D12DescriptorHeap> gCbvSrvUavHeap;
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
    static std::unique_ptr<ConstantBuffer> gCameraBuffer;
    static std::unique_ptr<ConstantBuffer> gEmissiveBuffer;

    // Raytracing
    static std::vector<std::unique_ptr<Texture>> gRaytracingOutput(BACK_BUFFERS_COUNT);
    static std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> gRaytracingOutputResourceUAVGpuDescriptor(BACK_BUFFERS_COUNT);
    static uint32 gRaytracingOutputResourceUAVDescriptorHeapIndex = std::numeric_limits<uint32>::max();
    static RayGenConstantBuffer gRayGenConstantBuffer;
    static D3DPtr<ID3D12RootSignature> gRaytracingGlobalRootSignature;
    static D3DPtr<ID3D12RootSignature> gRaytracingLocalRootSignature;
    static Slang::ComPtr<slang::IBlob> gRayGenShader;
    static Slang::ComPtr<slang::IBlob> gClosestHitShader;
    static Slang::ComPtr<slang::IBlob> gMissShader;
    static D3DPtr<ID3D12StateObject> gRaytracingStateObject;
    static D3DPtr<ID3D12Resource> gAccelerationStructure;
    static std::vector<D3DPtr<ID3D12Resource>> gBottomLevelAccelerationStructures;
    static std::vector<D3DPtr<ID3D12Resource>> gTopLevelAccelerationStructures;
    static D3DPtr<ID3D12Resource> gRayGenShaderTable;
    static D3DPtr<ID3D12Resource> gMissShaderTable;
    static D3DPtr<ID3D12Resource> gHitGroupShaderTable;

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
    createShaders() noexcept;
    
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
    createShaders() noexcept
    {
        HRESULT hr = S_OK;
        SlangGlobalSessionDesc slangGlobalSessionDesc = {};
        slang::createGlobalSession(&slangGlobalSessionDesc, gSlangGlobalSession.writeRef());

		std::vector<slang::CompilerOptionEntry> compilerOptions =
		{
			slang::CompilerOptionEntry
			{
				.name = slang::CompilerOptionName::Include,
				.value = slang::CompilerOptionValue
				{
					.kind = slang::CompilerOptionValueKind::String,
					.stringValue0 = "Assets/Shaders",
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
            const std::filesystem::path shaderAbsoluteParentPath = std::filesystem::current_path() / "Assets/Shaders";
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
                    .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
                    .Constants =
                    {
                        .ShaderRegister = 2,
                        .RegisterSpace = 0,
                        .Num32BitValues = 4,
                    },
                    .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
                }
            };

            const D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc =
            {
                .NumParameters = static_cast<UINT>(rootParameters.size()),
                .pParameters = rootParameters.data(),
                .NumStaticSamplers = 0,
                .pStaticSamplers = nullptr,
                .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            };

            D3DPtr<ID3DBlob> signature;
            D3DPtr<ID3DBlob> error;
            hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, signature.GetAddressOf(), error.GetAddressOf());
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

            hr = gDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(gRasterizationRootSignature.GetAddressOf()));
            if(FAILED(hr))
            {
                assert(false && "Failed to create root signature");
                return false;
            }

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
                .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
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
        }   // Rasterization
        
        // Raytracing
        {
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
                .compilerOptionEntries = compilerOptions.data(),
                .compilerOptionEntryCount = static_cast<uint32_t>(compilerOptions.size()),
            };


            Slang::ComPtr<slang::ISession> session;
            gSlangGlobalSession->createSession(sessionDesc, session.writeRef());
            const std::filesystem::path shaderAbsoluteParentPath = std::filesystem::current_path() / "Assets/Shaders";
            const std::filesystem::path shaderAbsPath = shaderAbsoluteParentPath / "SimpleRaytracing.slang";

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
            module->findEntryPointByName("RayGenMain", rayGenEntryPoint.writeRef());
            componentTypes.push_back(rayGenEntryPoint);
            Slang::ComPtr<slang::IEntryPoint> closestHitEntryPoint;
            module->findEntryPointByName("ClosestHitMain", closestHitEntryPoint.writeRef());
            componentTypes.push_back(closestHitEntryPoint);
            Slang::ComPtr<slang::IEntryPoint> missEntryPoint;
            module->findEntryPointByName("MissMain", missEntryPoint.writeRef());
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

            gRayGenShader = compileShader(program, shaderAbsPath, ShaderType::RAY_GEN, 0);
            if(gRayGenShader == nullptr)
            {
                assert(false && "Failed to compile ray generation shader");
                return false;
            }
            gClosestHitShader = compileShader(program, shaderAbsPath, ShaderType::CLOSEST_HIT, 1);
            if(gClosestHitShader == nullptr)
            {
                assert(false && "Failed to compile closest hit shader");
                return false;
            }
            gMissShader = compileShader(program, shaderAbsPath, ShaderType::MISS, 2);
            if(gMissShader == nullptr)
            {
                assert(false && "Failed to compile miss shader");
                return false;
            }

            // Global root signature
            {
                const D3D12_DESCRIPTOR_RANGE descriptorRange[]
                {
                    {
                        .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
                        .NumDescriptors = 1,
                        .BaseShaderRegister = 0,
                        .RegisterSpace = 0,
                        .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND,
                    },
                };
                const D3D12_ROOT_PARAMETER rootParameters[static_cast<uint32>(GlobalRootSignatureParams::COUNT)] = 
                { 
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                        .DescriptorTable = 
                        {
                            .NumDescriptorRanges = CGS_ARRAYSIZE(descriptorRange),
                            .pDescriptorRanges = descriptorRange,
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    }, 
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
                        .Descriptor = 
                        {
                            .ShaderRegister = 0,
                            .RegisterSpace = 0,
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    }
                };

                const D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc =
                {
                    .NumParameters = CGS_ARRAYSIZE(rootParameters),
                    .pParameters = rootParameters,
                    .NumStaticSamplers = 0,
                    .pStaticSamplers = nullptr,
                    .Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE,
                };

                D3DPtr<ID3DBlob> signature;
                D3DPtr<ID3DBlob> error;
                hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, signature.GetAddressOf(), error.GetAddressOf());
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

                hr = gDevice->CreateRootSignature(1, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(gRaytracingGlobalRootSignature.GetAddressOf()));
                if(FAILED(hr))
                {
                    assert(false && "Failed to create root signature");
                    return false;
                }
            } // Global root signature

            // Local root signature
            {
                const D3D12_ROOT_PARAMETER rootParameters[static_cast<uint32>(LocalRootSignatureParams::COUNT)] = 
                { 
                    {
                        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
                        .Constants =
                        {
                            .ShaderRegister = 0,
                            .RegisterSpace = 0,
                            .Num32BitValues = sizeof(RayGenConstantBuffer) / 4,
                        },
                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    }, 
                };

                const D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc =
                {
                    .NumParameters = CGS_ARRAYSIZE(rootParameters),
                    .pParameters = rootParameters,
                    .NumStaticSamplers = 0,
                    .pStaticSamplers = nullptr,
                    .Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE,
                };

                D3DPtr<ID3DBlob> signature;
                D3DPtr<ID3DBlob> error;
                hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, signature.GetAddressOf(), error.GetAddressOf());
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

                hr = gDevice->CreateRootSignature(1, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(gRaytracingLocalRootSignature.GetAddressOf()));
                if(FAILED(hr))
                {
                    assert(false && "Failed to create root signature");
                    return false;
                }
            } // Local root signature

            std::vector<D3D12_EXPORT_DESC> exportDescs =
            {
                {
                    .Name = L"RayGenMain",
                    .ExportToRename = nullptr,
                    .Flags = D3D12_EXPORT_FLAG_NONE,
                },
                {
                    .Name = L"ClosestHitMain",
                    .ExportToRename = nullptr,
                    .Flags = D3D12_EXPORT_FLAG_NONE,
                },
                {
                    .Name = L"MissMain",
                    .ExportToRename = nullptr,
                    .Flags = D3D12_EXPORT_FLAG_NONE,
                },
            };
            const std::vector<D3D12_DXIL_LIBRARY_DESC> libraries = 
            {
                {
                    .DXILLibrary = 
                    {
                        .pShaderBytecode = gRayGenShader->getBufferPointer(),
                        .BytecodeLength = gRayGenShader->getBufferSize(),
                    },
                    .NumExports = 1,
                    .pExports = &exportDescs[0],
                },
                {
                    .DXILLibrary = 
                    {
                        .pShaderBytecode = gClosestHitShader->getBufferPointer(),
                        .BytecodeLength = gClosestHitShader->getBufferSize(),
                    },
                    .NumExports = 1,
                    .pExports = &exportDescs[1],
                },
                {
                    .DXILLibrary = 
                    {
                        .pShaderBytecode = gMissShader->getBufferPointer(),
                        .BytecodeLength = gMissShader->getBufferSize(),
                    },
                    .NumExports = 1,
                    .pExports = &exportDescs[2],
                },
            };

            const D3D12_HIT_GROUP_DESC hitGroupDesc =
            {
                .HitGroupExport = L"HitGroup",
                .Type = D3D12_HIT_GROUP_TYPE_TRIANGLES,
                .ClosestHitShaderImport = L"ClosestHitMain",
            };

            const D3D12_RAYTRACING_SHADER_CONFIG raytracingShaderConfig =
            {
                .MaxPayloadSizeInBytes = 4 * sizeof(float),     // float4 color
                .MaxAttributeSizeInBytes = 2 * sizeof(float),   // float2 barycentrics
            };

            D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localRootSignatureAssociation =
            {
                .pSubobjectToAssociate = nullptr,
                .NumExports = 1,
                .pExports = &exportDescs[0].Name,
            };

            const D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig =
            {
                .MaxTraceRecursionDepth = 1,
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
                    .pDesc = gRaytracingLocalRootSignature.GetAddressOf(),
                },
                {
                    .Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
                    .pDesc = &localRootSignatureAssociation,
                },
                {
                    .Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,
                    .pDesc = gRaytracingGlobalRootSignature.GetAddressOf(),
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
            hr = gDevice->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(gRaytracingStateObject.GetAddressOf()));
            if(FAILED(hr))
            {
                assert(false && "Failed to create raytracing pipeline state object");
                return false;
            }
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
        gEmissiveBuffer.reset();
        gCameraBuffer.reset();
        gRaytracingOutput.clear();

        for(D3DPtr<ID3D12Resource>& bottomLevelAccelerationStructure : gBottomLevelAccelerationStructures)
        {
            DestroyD3D12Object(bottomLevelAccelerationStructure);
        }

        for(D3DPtr<ID3D12Resource>& topLevelAccelerationStructure : gTopLevelAccelerationStructures)
        {
            DestroyD3D12Object(topLevelAccelerationStructure);
        }
        DestroyD3D12Object(gRaytracingStateObject);
        DestroyD3D12Object(gRaytracingLocalRootSignature);
        DestroyD3D12Object(gRaytracingGlobalRootSignature);

        DestroyD3D12Object(gRayGenShaderTable);
        DestroyD3D12Object(gMissShaderTable);
        DestroyD3D12Object(gHitGroupShaderTable);

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

        if(gMissShader != nullptr)
        {
            gMissShader->Release();
        }
        else
        {
            assert(false && "gMissShader is null");
        }

        if(gClosestHitShader != nullptr)
        {
            gClosestHitShader->Release();
        }
        else
        {
            assert(false && "gClosestHitShader is null");
        }

        if(gRayGenShader != nullptr)
        {
            gRayGenShader->Release();
        }
        else
        {
            assert(false && "gRayGenShader is null");
        }

        DestroyD3D12Object(gFence);
        
        gSceneRenderTargets.clear();

        DestroyD3D12Object(gCbvSrvUavHeap);
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
        outIndexBuffer.Initialize(std::move(indexBufferCreateInfo));
        return true;
    }

    bool
    CreateCornellBoxScene(const eRenderMethod renderMethod, std::vector<std::unique_ptr<Geometry>>& outGeometries) noexcept
    {
        Camera& mainCamera = InitializeCornellBoxCamera();
        
        HRESULT hr = S_OK;

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
            
            ConstantBuffer::CreateInfo cameraBufferCreateInfo;
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
                assert(false && "Failed to create vertex buffer");
                return false;
            }
            
            const D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = 
            {
                .BufferLocation = cameraBufferCreateInfo.Data->GetGPUVirtualAddress(),
                .SizeInBytes = static_cast<uint32>(cameraBufferDesc.Width),
            };
            cameraBufferCreateInfo.View = gCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
            gDevice->CreateConstantBufferView(&cbvDesc, cameraBufferCreateInfo.View);

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

            gCameraBuffer = std::make_unique<ConstantBuffer>(std::move(cameraBufferCreateInfo));
        }

        const float border = 0.1f;
        const uint32 width = gSceneRenderTargets[0].ColorBuffer.GetWidth();
        const uint32 height = gSceneRenderTargets[0].ColorBuffer.GetHeight();
        const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        if(width <= height)
        {
            gRayGenConstantBuffer.CurrentStencil =
            {
                .Left = -1.0f + border,
                .Top = -1.0f + border * aspectRatio,
                .Right = 1.0f - border,
                .Bottom = 1.0f - border * aspectRatio,
            };
        }
        else
        {
            gRayGenConstantBuffer.CurrentStencil =
            {
                .Left = -1.0f + border,
                .Top = -1.0f + border / aspectRatio,
                .Right = 1.0f - border,
                .Bottom = 1.0f - border / aspectRatio,
            };
        }
        
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
            addQuadVertices(vertices, indices, v0, v1, v2, v3);

            VertexBuffer vertexBuffer;
            result = createVertexBuffer(vertexBuffer, vertices, "FloorVertexBuffer");
            floor.SetVertexBuffer(std::move(vertexBuffer));
                
            IndexBuffer indexBuffer;
            result = createIndexBuffer(indexBuffer, indices, "FloorIndexBuffer");
            floor.SetIndexBuffer(std::move(indexBuffer));
            floor.SetColor(WHITE);
        }

        if (result == false)
        {
            assert(false && "Failed to create vertex buffer");
            outGeometries.pop_back();
        }
        
        // Light
        struct EmissiveBuffer final
        {
            float4 Position;
            float3 Color;
        };
        EmissiveBuffer emissiveBuffer;

        outGeometries.push_back(std::make_unique<Geometry>(std::string("Light")));
        Geometry& light = *outGeometries.back();
        {
            vertices.clear();
            indices.clear();

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { -343.0f, 548.8f, 332.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -343.0f, 548.8f, 227.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -213.0f, 548.8f, 227.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { -213.0f, 548.8f, 332.0f };
            addQuadVertices(vertices, indices, v0, v1, v2, v3);

            emissiveBuffer.Position = v0;
            emissiveBuffer.Position += v1;
            emissiveBuffer.Position += v2;
            emissiveBuffer.Position += v3;
            emissiveBuffer.Position /= 4.0f;
            emissiveBuffer.Position.W = 1.0f;

            VertexBuffer vertexBuffer;
            result = createVertexBuffer(vertexBuffer, vertices, "LightVertexBuffer");
            light.SetVertexBuffer(std::move(vertexBuffer));

            IndexBuffer indexBuffer;
            result = createIndexBuffer(indexBuffer, indices, "LightIndexBuffer");
            light.SetIndexBuffer(std::move(indexBuffer));

            light.SetColor(WHITE);
            emissiveBuffer.Color = float3{1.0f, 1.0f, 1.0f};
            light.SetIsEmissive(true);
        }

        if (result == false)
        {
            assert(false && "Failed to create vertex buffer");
            outGeometries.pop_back();
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
            
            ConstantBuffer::CreateInfo emissiveBufferCreateInfo;
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
                assert(false && "Failed to create vertex buffer");
                return false;
            }
            
            const D3D12_CONSTANT_BUFFER_VIEW_DESC emissiveCbvDesc = 
            {
                .BufferLocation = emissiveBufferCreateInfo.Data->GetGPUVirtualAddress(),
                .SizeInBytes = static_cast<uint32>(emissiveBufferDesc.Width),
            };
            emissiveBufferCreateInfo.View = gCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
            gDevice->CreateConstantBufferView(&emissiveCbvDesc, emissiveBufferCreateInfo.View);

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

            gEmissiveBuffer = std::make_unique<ConstantBuffer>(std::move(emissiveBufferCreateInfo));
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
            addQuadVertices(vertices, indices, v0, v1, v2, v3);

            VertexBuffer vertexBuffer;
            result = createVertexBuffer(vertexBuffer, vertices, "CeilingVertexBuffer");
            ceiling.SetVertexBuffer(std::move(vertexBuffer));

            IndexBuffer indexBuffer;
            result = createIndexBuffer(indexBuffer, indices, "CeilingIndexBuffer");
            ceiling.SetIndexBuffer(std::move(indexBuffer));

            ceiling.SetColor(WHITE);
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
            addQuadVertices(vertices, indices, v0, v1, v2, v3);

            VertexBuffer vertexBuffer;
            result = createVertexBuffer(vertexBuffer, vertices, "BackWallVertexBuffer");
            backWall.SetVertexBuffer(std::move(vertexBuffer));

            IndexBuffer indexBuffer;
            result = createIndexBuffer(indexBuffer, indices, "BackWallIndexBuffer");
            backWall.SetIndexBuffer(std::move(indexBuffer));

            backWall.SetColor(WHITE);
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
            addQuadVertices(vertices, indices, v0, v1, v2, v3);

            VertexBuffer vertexBuffer;
            result = createVertexBuffer(vertexBuffer, vertices, "RightWallVertexBuffer");
            rightWall.SetVertexBuffer(std::move(vertexBuffer));

            IndexBuffer indexBuffer;
            result = createIndexBuffer(indexBuffer, indices, "RightWallIndexBuffer");
            rightWall.SetIndexBuffer(std::move(indexBuffer));

            rightWall.SetColor(GREEN);
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
            addQuadVertices(vertices, indices, v0, v1, v2, v3);

            VertexBuffer vertexBuffer;
            result = createVertexBuffer(vertexBuffer, vertices, "LeftWallVertexBuffer");
            leftWall.SetVertexBuffer(std::move(vertexBuffer));

            IndexBuffer indexBuffer;
            result = createIndexBuffer(indexBuffer, indices, "LeftWallIndexBuffer");
            leftWall.SetIndexBuffer(std::move(indexBuffer));

            leftWall.SetColor(RED);
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
            addQuadVertices(vertices, indices, v0, v1, v2, v3);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v4 = { -290.0f, 165.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v5 = { -290.0f, 0.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v6 = { -240.0f, 0.0f, 272.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v7 = { -240.0f, 165.0f, 272.0f };
            addQuadVertices(vertices, indices, v4, v5, v6, v7);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v8 = { -130.0f, 165.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v9 = { -130.0f, 0.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v10 = { -290.0f, 0.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v11 = { -290.0f, 165.0f, 114.0f };
            addQuadVertices(vertices, indices, v8, v9, v10, v11);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v12 = { -82.0f, 165.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v13 = { -82.0f, 0.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v14 = { -130.0f, 0.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v15 = { -130.0f, 165.0f, 65.0f };
            addQuadVertices(vertices, indices, v12, v13, v14, v15);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v16 = { -240.0f, 165.0f, 272.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v17 = { -240.0f, 0.0f, 272.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v18 = { -82.0f, 0.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v19 = { -82.0f, 165.0f, 225.0f };
            addQuadVertices(vertices, indices, v16, v17, v18, v19);

            VertexBuffer vertexBuffer;
            result = createVertexBuffer(vertexBuffer, vertices, "ShortBlockVertexBuffer");
            shortBlock.SetVertexBuffer(std::move(vertexBuffer));

            IndexBuffer indexBuffer;
            result = createIndexBuffer(indexBuffer, indices, "ShortBlockIndexBuffer");
            shortBlock.SetIndexBuffer(std::move(indexBuffer));

            shortBlock.SetColor(WHITE);
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
            addQuadVertices(vertices, indices, v0, v1, v2, v3);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v4 = { -423.0f, 330.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v5 = { -423.0f, 0.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v6 = { -472.0f, 0.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v7 = { -472.0f, 330.0f, 406.0f };
            addQuadVertices(vertices, indices, v4, v5, v6, v7);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v8 = { -472.0f, 330.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v9 = { -472.0f, 0.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v10 = { -314.0f, 0.0f, 456.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v11 = { -314.0f, 330.0f, 456.0f };
            addQuadVertices(vertices, indices, v8, v9, v10, v11);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v12 = { -314.0f, 330.0f, 456.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v13 = { -314.0f, 0.0f, 456.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v14 = { -265.0f, 0.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v15 = { -265.0f, 330.0f, 296.0f };
            addQuadVertices(vertices, indices, v12, v13, v14, v15);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v16 = { -265.0f, 330.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v17 = { -265.0f, 0.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v18 = { -423.0f, 0.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v19 = { -423.0f, 330.0f, 247.0f };
            addQuadVertices(vertices, indices, v16, v17, v18, v19);

            VertexBuffer vertexBuffer;
            result = createVertexBuffer(vertexBuffer, vertices, "TallBlockVertexBuffer");
            tallBlock.SetVertexBuffer(std::move(vertexBuffer));

            IndexBuffer indexBuffer;
            result = createIndexBuffer(indexBuffer, indices, "TallBlockIndexBuffer");
            tallBlock.SetIndexBuffer(std::move(indexBuffer));

            tallBlock.SetColor(WHITE);
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
    Render(uint64& inoutWorkIndex, RenderThreadInfo& inoutRenderThreadInfo, const std::vector<std::unique_ptr<Geometry>>& geometries) noexcept
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


        const D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc =
        {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = 3,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            .NodeMask = 0,
        };
        hr = gDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(gCbvSrvUavHeap.GetAddressOf()));
        if (FAILED(hr))
        {
            assert(false && "Failed to create CBV descriptor heap");
            return false;
        }
        gCbvSrvUavHeap->SetName(TEXT("Main CBV Descriptor Heap"));

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
            colorBufferInfo.IsBackBuffer = true;

            hr = gSwapChain->GetBuffer(frameIndex, IID_PPV_ARGS(colorBufferInfo.ParentCreateInfo.Data.GetAddressOf()));
            if(FAILED(hr))
            {
                assert(false && "Failed to get swap chain buffer");
                return false;
            }

            colorBufferInfo.ParentCreateInfo.View.ptr = rtvStartHandle.ptr + (frameIndex * gRtvIncrementSize);
            colorBufferInfo.ParentCreateInfo.State = D3D12_RESOURCE_STATE_COMMON;    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12#initial-states-for-resources
            colorBufferInfo.ParentCreateInfo.Name = "SwapChainColorBuffer[" + std::to_string(frameIndex) + "]";
            gDevice->CreateRenderTargetView(colorBufferInfo.ParentCreateInfo.Data.Get(), &colorBufferViewDesc, colorBufferInfo.ParentCreateInfo.View);

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

            depthBufferInfo.ParentCreateInfo.View.ptr = dsvStartHandle.ptr + (frameIndex * gDsvIncrementSize);

            gDevice->CreateDepthStencilView(depthBufferInfo.ParentCreateInfo.Data.Get(), nullptr, depthBufferInfo.ParentCreateInfo.View);

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

        const bool result = createShaders();
        if(result == false)
        {
            assert(false && "Failed to create shaders");
            return false;
        }

        D3DPtr<ID3D12StateObjectProperties> stateObjectProperties;
        hr = gRaytracingStateObject->QueryInterface(IID_PPV_ARGS(stateObjectProperties.GetAddressOf()));
        if (FAILED(hr))
        {
            assert(false && "Failed to get raytracing state object properties");
            return false;
        }

        void* rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(TEXT("RayGenMain"));
        void* missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(TEXT("MissMain"));
        void* hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(TEXT("HitGroup"));
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
            struct RootArguments final
            {
                RayGenConstantBuffer ConstantBuffer;
            };

            RootArguments rootArguments =
            {
                .ConstantBuffer = gRayGenConstantBuffer,
            };

            const uint32 shaderRecordsCount = 1;
            const uint32 shaderRecordSize = static_cast<uint32>(Align(shaderIdentifierSize + sizeof(rootArguments), static_cast<size_t>(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT)));
            bufferDesc.Width = shaderRecordsCount * shaderRecordSize;
            
            hr = gDevice->CreateCommittedResource(
                &bufferHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(gRayGenShaderTable.GetAddressOf()));
            if(FAILED(hr))
            {
                assert(false && "Failed to create ray gen shader table");
                return false;
            }
            gRayGenShaderTable->SetName(TEXT("RayGenShaderTable"));

            uint8_t* mappedData = nullptr;
            // We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
            D3D12_RANGE range = { .Begin = 0, .End = 0 };
            hr = gRayGenShaderTable->Map(0, &range, reinterpret_cast<void**>(&mappedData));
            if(FAILED(hr))
            {
                assert(false && "Failed to map ray gen shader table");
                return false;
            }
            
            memcpy(mappedData, rayGenShaderIdentifier, shaderIdentifierSize);
            memcpy(mappedData + shaderIdentifierSize, &rootArguments, sizeof(rootArguments));
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
                IID_PPV_ARGS(gMissShaderTable.GetAddressOf()));
            if(FAILED(hr))
            {
                assert(false && "Failed to create miss shader table");
                return false;
            }

            uint8_t* mappedData = nullptr;
            // We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
            D3D12_RANGE range = { .Begin = 0, .End = 0 };
            hr = gMissShaderTable->Map(0, &range, reinterpret_cast<void**>(&mappedData));
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
                IID_PPV_ARGS(gHitGroupShaderTable.GetAddressOf()));
            if(FAILED(hr))
            {
                assert(false && "Failed to create hit group shader table");
                return false;
            }

            uint8_t* mappedData = nullptr;
            // We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
            D3D12_RANGE range = { .Begin = 0, .End = 0 };
            hr = gHitGroupShaderTable->Map(0, &range, reinterpret_cast<void**>(&mappedData));
            if(FAILED(hr))
            {
                assert(false && "Failed to map hit group shader table");
                return false;
            }

            memcpy(mappedData, hitGroupShaderIdentifier, shaderIdentifierSize);
            mappedData += shaderRecordSize;
        }
    
        // Create the output resource. The dimensions and format should match the swap-chain.
        for(uint32 i = 0; i < BACK_BUFFERS_COUNT; ++i)
        {
            Texture::CreateInfo raytracingOutputCreateInfo = {};
            D3D12_HEAP_PROPERTIES heapProperties = 
            {
                .Type = D3D12_HEAP_TYPE_DEFAULT,
                .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
                .CreationNodeMask = 1,
                .VisibleNodeMask = 1,
            };

            bufferDesc =
            D3D12_RESOURCE_DESC
            {
                .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                .Alignment = 0,
                .Width = cgs::gWidth,
                .Height = cgs::gHeight,
                .DepthOrArraySize = 1,
                .MipLevels = 1,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 },
                .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                .Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            };
            
            raytracingOutputCreateInfo.ParentCreateInfo.State = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            hr = gDevice->CreateCommittedResource(
                &heapProperties, 
                D3D12_HEAP_FLAG_NONE, 
                &bufferDesc, 
                raytracingOutputCreateInfo.ParentCreateInfo.State, 
                nullptr, 
                IID_PPV_ARGS(raytracingOutputCreateInfo.ParentCreateInfo.Data.GetAddressOf()));
            if(FAILED(hr))
            {
                assert(false && "Failed to create raytracing output resource");
                return false;
            }
            const std::wstring resourceName = L"RaytracingOutputResource[" + std::to_wstring(i) + L"]";
            raytracingOutputCreateInfo.ParentCreateInfo.Data->SetName(resourceName.c_str());

            const uint32 descriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            raytracingOutputCreateInfo.ParentCreateInfo.View = gCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
            raytracingOutputCreateInfo.ParentCreateInfo.View.ptr += descriptorSize * i;
            
            raytracingOutputCreateInfo.UavView.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            gDevice->CreateUnorderedAccessView(raytracingOutputCreateInfo.ParentCreateInfo.Data.Get(), nullptr, &raytracingOutputCreateInfo.UavView, raytracingOutputCreateInfo.ParentCreateInfo.View);
            gRaytracingOutput[i] = std::make_unique<Texture>(std::move(raytracingOutputCreateInfo));
            gRaytracingOutputResourceUAVGpuDescriptor[i] = gCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
            gRaytracingOutputResourceUAVGpuDescriptor[i].ptr += descriptorSize * i;
        }

        gGlobalRenderContext.RenderDeviceType = eRenderDeviceType::D3D12;
        return true;
    }

    void
    rasterize(ID3D12GraphicsCommandList& graphicsCommandList, SceneRenderTarget& sceneRenderTarget, RenderWork& renderWork) noexcept
    {
        graphicsCommandList.SetGraphicsRootSignature(gRasterizationRootSignature.Get());

        ID3D12DescriptorHeap* heaps[] = { gCbvSrvUavHeap.Get() };
        graphicsCommandList.SetDescriptorHeaps(CGS_ARRAYSIZE(heaps), heaps);

        graphicsCommandList.SetGraphicsRootConstantBufferView(0, gCameraBuffer->GetGPUVirtualAddress());
        graphicsCommandList.SetGraphicsRootConstantBufferView(1, gEmissiveBuffer->GetGPUVirtualAddress());
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

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { sceneRenderTarget.ColorBuffer.GetView(), };
        graphicsCommandList.OMSetRenderTargets(CGS_ARRAYSIZE(rtvHandles), rtvHandles, FALSE, &sceneRenderTarget.DepthBuffer.GetView());

        sceneRenderTarget.ColorBuffer.Transition(graphicsCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        constexpr float BLACK_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        graphicsCommandList.ClearRenderTargetView(
            sceneRenderTarget.ColorBuffer.GetView(),
            BLACK_COLOR,
            0,
            nullptr
        );
        graphicsCommandList.ClearDepthStencilView(
            sceneRenderTarget.DepthBuffer.GetView(),
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

            const float4& pushConstantColor = geometry->GetColor();
            graphicsCommandList.SetGraphicsRoot32BitConstants(2, 4, &pushConstantColor, 0);

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
        graphicsCommandList.SetComputeRootSignature(gRaytracingGlobalRootSignature.Get());

        // Bind the heaps, acceleration structure and dispatch rays.
        graphicsCommandList.SetDescriptorHeaps(1, gCbvSrvUavHeap.GetAddressOf());
        graphicsCommandList.SetComputeRootDescriptorTable(0, gRaytracingOutputResourceUAVGpuDescriptor[renderWork.FrameIndex]);
        const D3D12_DISPATCH_RAYS_DESC dispatchDesc = 
        {
            .RayGenerationShaderRecord =
            {
                .StartAddress = gRayGenShaderTable->GetGPUVirtualAddress(),
                .SizeInBytes = gRayGenShaderTable->GetDesc().Width,
            },
            .MissShaderTable = 
            {
                .StartAddress = gMissShaderTable->GetGPUVirtualAddress(),
                .SizeInBytes = gMissShaderTable->GetDesc().Width,
                .StrideInBytes = gMissShaderTable->GetDesc().Width,
            },
            .HitGroupTable = 
            {
                .StartAddress = gHitGroupShaderTable->GetGPUVirtualAddress(),
                .SizeInBytes = gHitGroupShaderTable->GetDesc().Width,
                .StrideInBytes = gHitGroupShaderTable->GetDesc().Width,
            },
            .Width = sceneRenderTarget.ColorBuffer.GetWidth(),
            .Height = sceneRenderTarget.ColorBuffer.GetHeight(),
            .Depth = 1,
        };
        graphicsCommandList.SetPipelineState1(gRaytracingStateObject.Get());

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
            graphicsCommandList.DispatchRays(&dispatchDesc);
        }

        sceneRenderTarget.ColorBuffer.Transition(graphicsCommandList, D3D12_RESOURCE_STATE_COPY_DEST);
        gRaytracingOutput[renderWork.FrameIndex]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_COPY_SOURCE);

        graphicsCommandList.CopyResource(sceneRenderTarget.ColorBuffer.GetResource().Get(), gRaytracingOutput[renderWork.FrameIndex]->GetResource().Get());

        gRaytracingOutput[renderWork.FrameIndex]->Transition(graphicsCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
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