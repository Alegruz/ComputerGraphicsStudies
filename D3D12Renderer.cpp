#include "pch.hpp"

#include "Renderer.hpp"

#if defined(CGS_GRAPHICS_API_D3D12)
namespace cgs
{
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

    template <>
    bool
    InitializeRenderer<eRenderDeviceType::D3D12>() noexcept
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

        return true;
    }
}
#else   // NOT defined(CGS_GRAPHICS_API_D3D12)
    template <>
    bool
    InitializeRenderer<eRenderDeviceType::D3D12>() noexcept
    {
        return false;
    }
#endif  // NOT defined(CGS_GRAPHICS_API_D3D12)