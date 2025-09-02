#include "pch.hpp"

#include "Common/Renderer.cpp"

#if defined(CGS_GRAPHICS_API_D3D12)
#include "D3D12/Renderer.h"

namespace cgs
{
    static GlobalRenderContext gGlobalRenderContext;

    using TDXGIGetDebugInterface = HRESULT (*)(REFIID, void**);

#define CGS_DESTROY_D3D12_OBJECT(object)    \
    if (object != nullptr)                  \
    {                                       \
        [[maybe_unused]] const uint64 refCount = (object)->Release(); \
        assert(refCount == 0 && #object " was not released properly"); \
    }                                       \
    else                                    \
    {                                       \
        assert(false && #object " is null"); \
    }

#define CGS_DESTROY_DXGI_OBJECT(object) CGS_DESTROY_D3D12_OBJECT(object)

    template<typename T>
    class D3DPtr final
    {
    public:
        CGS_INLINE explicit constexpr 
        D3DPtr() noexcept
            : mPtr()
        {
        }

        CGS_INLINE constexpr
        D3DPtr(const D3DPtr& other) noexcept
            : mPtr(other.mPtr)
        {
            if (mPtr)
            {
                mPtr->AddRef();
            }
        }

        CGS_INLINE constexpr
        D3DPtr(D3DPtr&& other) noexcept
            : mPtr(other.mPtr)
        {
            other.mPtr = nullptr;
        }

        CGS_INLINE 
        ~D3DPtr()
        {
            if (mPtr)
            {
                mPtr->Release();
            }
        }

        CGS_INLINE constexpr
        D3DPtr& operator=(const D3DPtr& other) noexcept
        {
            if (this != &other)
            {
                if (mPtr)
                {
                    mPtr->Release();
                }
                mPtr = other.mPtr;

                if(mPtr)
                {
                    mPtr->AddRef();
                }
            }
            return *this;
        }

        CGS_INLINE constexpr
        D3DPtr& operator=(D3DPtr&& other) noexcept
        {
            if (this != &other)
            {
                if (mPtr)
                {
                    mPtr->Release();
                }
                mPtr = other.mPtr;
                other.mPtr = nullptr;
            }
            return *this;
        }

        CGS_INLINE T* 
        Get() const noexcept
        {
            return mPtr;
        }

        CGS_INLINE T** 
        GetAddressOf() noexcept
        {
            return &mPtr;
        }

        CGS_INLINE constexpr const T*
        operator->() const noexcept
        {
            return mPtr;
        }

        CGS_INLINE constexpr T*
        operator->() noexcept
        {
            return mPtr;
        }

        CGS_INLINE constexpr bool
        operator==(std::nullptr_t) const noexcept
        {
            return mPtr == nullptr;
        }

        CGS_INLINE constexpr bool
        operator!=(std::nullptr_t) const noexcept
        {
            return mPtr != nullptr;
        }

    private:
        T* mPtr;
    };

    class Texture final
    {
    public:
        struct CreateInfo final
        {
            D3DPtr<ID3D12Resource> Data;
            D3D12_CPU_DESCRIPTOR_HANDLE View;
        };

    public:
        CGS_INLINE constexpr 
        Texture() noexcept
            : mData()
            , mView()
        {
        }

        CGS_INLINE constexpr 
        Texture(CreateInfo&& createInfo) noexcept
            : mData(std::move(createInfo.Data))
            , mView(createInfo.View)
        {
        }

        CGS_INLINE 
        ~Texture() noexcept
        {
            CGS_DESTROY_D3D12_OBJECT(mData);
        }

        CGS_INLINE void
        Initialize(CreateInfo&& createInfo) noexcept
        {
            mData = std::move(createInfo.Data);
            mView = createInfo.View;
        }

    private:
        D3DPtr<ID3D12Resource> mData;
        D3D12_CPU_DESCRIPTOR_HANDLE mView;
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
    
    // DXGI
#if defined(CGS_DEBUG)
    static D3DPtr<IDXGIDebug> gDxgiDebug;
    static D3DPtr<IDXGIInfoQueue> gInfoQueue;
#endif  // defined(CGS_DEBUG)
    static D3DPtr<IDXGIFactory6> gFactory;
    static D3DPtr<IDXGIAdapter> gAdapter;
    static D3DPtr<IDXGISwapChain1> gSwapChain;
    
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
    DestroyRenderer() noexcept
    {
        CGS_DESTROY_D3D12_OBJECT(gGraphicsCommandList);
        CGS_DESTROY_D3D12_OBJECT(gGraphicsCommandAllocator);
        CGS_DESTROY_D3D12_OBJECT(gGraphicsCommandQueue);

        CGS_DESTROY_DXGI_OBJECT(gSwapChain);

        CGS_DESTROY_D3D12_OBJECT(gDevice);
        CGS_DESTROY_D3D12_OBJECT(gD3D12Debug);

        CGS_DESTROY_DXGI_OBJECT(gAdapter);
        CGS_DESTROY_DXGI_OBJECT(gFactory);

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
    }

    void
    CreateCornellBoxScene(std::vector<std::unique_ptr<Geometry>>&) noexcept
    {
        InitializeCornellBoxCamera();
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
                RenderWork renderWork = std::move(renderThreadInfo.RenderWorksPerFrame.front());
                renderThreadInfo.RenderWorksPerFrame.pop();
                uniqueLock.unlock();

                // Process the render work
                assert(false && "Render work not implemented");
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
        hr = gFactory->CreateSwapChainForHwnd(gGraphicsCommandQueue.Get(), createInfo.Window, &swapChainDesc, nullptr, nullptr, gSwapChain.GetAddressOf());
        if (FAILED(hr))
        {
            assert(false && "Failed to create swap chain");
            return false;
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

            gDevice->CreateRenderTargetView(colorBufferInfo.Data.Get(), &colorBufferViewDesc, colorBufferInfo.View);

            gSceneRenderTargets[frameIndex].ColorBuffer.Initialize(std::move(colorBufferInfo));

            Texture::CreateInfo depthBufferInfo;
            hr = gDevice->CreateCommittedResource(
                &depthBufferHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &depthBufferDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
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

        gGlobalRenderContext.RenderDeviceType = eRenderDeviceType::D3D12;
        return true;
    }
}
#endif  // defined(CGS_GRAPHICS_API_D3D12)