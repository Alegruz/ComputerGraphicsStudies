#include "pch.hpp"

#include "Common/Renderer.cpp"

#if defined(CGS_GRAPHICS_API_D3D12)
#include "D3D12/Renderer.h"

namespace cgs
{
    static GlobalRenderContext gGlobalRenderContext;

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

    private:
        T* mPtr;
    };

    static D3DPtr<IDXGIFactory6> gFactory;
    static D3DPtr<IDXGIAdapter> gAdapter;
    static D3DPtr<ID3D12Device> gDevice;
    static D3DPtr<IDXGISwapChain1> gSwapChain;

    void
    CreateCornellBoxScene(std::vector<Geometry>&) noexcept
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

        hr = D3D12CreateDevice(gAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(gDevice.GetAddressOf()));
        if (FAILED(hr))
        {
            assert(false && "Failed to create D3D12 Device");
            return false;
        }

        const DXGI_SWAP_CHAIN_DESC1 swapChainDesc = 
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
        hr = gFactory->CreateSwapChainForHwnd(gDevice.Get(), createInfo.Window, &swapChainDesc, nullptr, nullptr, gSwapChain.GetAddressOf());
        if (FAILED(hr))
        {
            assert(false && "Failed to create swap chain");
            return false;
        }

        gGlobalRenderContext.RenderDeviceType = eRenderDeviceType::D3D12;
        return true;
    }
}
#else   // NOT defined(CGS_GRAPHICS_API_D3D12)
    bool
    InitializeRenderer(const RendererCreateInfo&) noexcept
    {
        return false;
    }
#endif  // NOT defined(CGS_GRAPHICS_API_D3D12)