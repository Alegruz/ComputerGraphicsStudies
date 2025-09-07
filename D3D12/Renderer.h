#pragma once

#include "Common/Renderer.h"

#if defined(CGS_GRAPHICS_API_D3D12)
namespace cgs
{
    template<typename T>
    class D3DPtr final
    {
    public:
        CGS_INLINE explicit constexpr 
        D3DPtr() noexcept
            : mPtr(nullptr)
        {
        }

        CGS_INLINE constexpr
        D3DPtr(std::nullptr_t) noexcept
            : mPtr(nullptr)
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

        CGS_INLINE constexpr D3DPtr& 
        operator=(const D3DPtr& other) noexcept
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

        CGS_INLINE constexpr D3DPtr& 
        operator=(D3DPtr&& other) noexcept
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

        CGS_INLINE constexpr D3DPtr&
        operator=(std::nullptr_t) noexcept
        {
            mPtr = nullptr;
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

        CGS_INLINE constexpr const T*&
        operator->() const noexcept
        {
            return mPtr;
        }

        CGS_INLINE constexpr T*&
        operator->() noexcept
        {
            return mPtr;
        }

        CGS_INLINE constexpr T&
        operator*() noexcept
        {
            return *mPtr;
        }

        CGS_INLINE constexpr const T&
        operator*() const noexcept
        {
            return *mPtr;
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

    template<typename T>
    concept IUnknownDerived = std::is_base_of_v<IUnknown, T>;

    template<typename T>
    concept D3D12ObjectDerived = std::is_base_of_v<ID3D12Object, T>;
    
    template<IUnknownDerived T>
    static void
    checkRefCountValue(D3DPtr<T>& object) noexcept
    {
        if(object == nullptr)
        {
            return;
        }
        [[maybe_unused]] volatile const uint32 refCount = object->AddRef() - 1;
        object->Release();
    }

    template<IUnknownDerived T>
    bool
    DestroyD3D12ObjectOrNull(D3DPtr<T>& objectOrNull, const bool assertOnRemainingRefCountValues = true) noexcept
    {
        if (objectOrNull != nullptr)
        {
            [[maybe_unused]] const uint64 refCount = objectOrNull->Release();
            if constexpr (std::is_base_of_v<ID3D12Device, T> == false 
                || std::is_same_v<T, ID3D12Device> == true
                || std::is_same_v<T, ID3D12Device1> == true
                || std::is_same_v<T, ID3D12Device2> == true 
                || std::is_same_v<T, ID3D12Device3> == true 
                || std::is_same_v<T, ID3D12Device4> == true)
            {
                if(assertOnRemainingRefCountValues == true)
                {
                    assert(refCount == 0 && "Object was not released properly");
                }
            }
            objectOrNull = nullptr;
            return true;
        }

        return false;
    }

    template<IUnknownDerived T>
    void
    DestroyD3D12Object(D3DPtr<T>& objectOrNull, const bool assertOnRemainingRefCountValues = true) noexcept
    {
        const bool result = DestroyD3D12ObjectOrNull(objectOrNull, assertOnRemainingRefCountValues);
        if(result == false)
        {
            assert(false && "Object is null");
        }
    }

    template<typename T>
    concept DXGIObjectDerived = std::is_base_of_v<IDXGIObject, T>;

    template<IUnknownDerived T>
    void
    DestroyDXGIObject(D3DPtr<T>& objectOrNull, const bool assertOnRemainingRefCountValues = true) noexcept
    {
        DestroyD3D12Object(objectOrNull, assertOnRemainingRefCountValues);
    }

    class RenderResource
    {
    public:
        struct CreateInfo final
        {
            D3DPtr<ID3D12Resource> Data;
            D3D12_CPU_DESCRIPTOR_HANDLE View;
            D3D12_RESOURCE_STATES State;
            std::string Name;
        };

    public:
        CGS_INLINE constexpr 
        RenderResource() noexcept
            : mData()
            , mView()
            , mState(D3D12_RESOURCE_STATE_COMMON)
            , mName()
        {
        }

        CGS_INLINE CGS_CONSTEXPR_WITH_ASSERT 
        RenderResource(CreateInfo&& createInfo) noexcept
            : mData(std::move(createInfo.Data))
            , mView(createInfo.View)
            , mState(createInfo.State)
            , mName(std::move(createInfo.Name))
        {
            const bool result = SetName(mName);
            if(result == false)
            {
                assert(false && "Failed to set resource name");
            }
        }

        RenderResource(const RenderResource&) = delete;
        CGS_INLINE CGS_CONSTEXPR_WITH_ASSERT
        RenderResource(RenderResource&& other) noexcept
            : mData(std::move(other.mData))
            , mView(other.mView)
            , mState(other.mState)
            , mName(std::move(other.mName))
        {
            const bool result = SetName(mName);
            if(result == false)
            {
                assert(false && "Failed to set resource name");
            }
        }
        
        CGS_INLINE virtual
        ~RenderResource() noexcept
        {
            DestroyD3D12ObjectOrNull(mData);
        }

        RenderResource&
        operator=(const RenderResource&) = delete;
        CGS_INLINE constexpr RenderResource& 
        operator=(RenderResource&& other) noexcept
        {
            if (this != &other)
            {
                mData = std::move(other.mData);
                mView = other.mView;
                mState = other.mState;
                mName = std::move(other.mName);
                const bool result = SetName(mName);
                if(result == false)
                {
                    assert(false && "Failed to set resource name");
                }
            }
            return *this;
        }

        virtual CGS_INLINE void
        Initialize(CreateInfo&& createInfo) noexcept
        {
            mData = std::move(createInfo.Data);
            mView = createInfo.View;
            mState = createInfo.State;
            mName = std::move(createInfo.Name);

            const bool result = SetName(mName);
            if(result == false)
            {
                assert(false && "Failed to set resource name");
            }
        }

        CGS_INLINE const D3DPtr<ID3D12Resource>&
        GetResource() const noexcept
        {
            return mData;
        }

        CGS_INLINE const D3D12_GPU_VIRTUAL_ADDRESS
        GetGPUVirtualAddress() noexcept
        {
            return mData->GetGPUVirtualAddress();
        }

        CGS_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE&
        GetView() const noexcept
        {
            return mView;
        }

        void 
        Transition(ID3D12GraphicsCommandList& commandList, const D3D12_RESOURCE_STATES newState) noexcept;

        [[nodiscard]] CGS_INLINE CGS_CONSTEXPR_WITH_ASSERT bool
        SetName(const std::string& name) noexcept
        {
            mName = name;

            if(mData == nullptr)
            {
                assert(false && "SetName: resource is null");
                return false;
            }

            HRESULT hr = mData->SetName(std::wstring(mName.begin(), mName.end()).c_str());
            if (FAILED(hr))
            {
                assert(false && "Failed to set resource name");
                return false;
            }
            return true;
        }

    protected:
        D3DPtr<ID3D12Resource> mData;
        D3D12_CPU_DESCRIPTOR_HANDLE mView;
        D3D12_RESOURCE_STATES mState;
        std::string mName;
    };

    class Texture final : public RenderResource
    {
    public:
        struct CreateInfo final
        {
            RenderResource::CreateInfo ParentCreateInfo;
            bool IsBackBuffer = false;
        };

    public:
        CGS_INLINE constexpr 
        Texture() noexcept
            : RenderResource()
            , mIsBackBuffer(false)
        {
        }

        CGS_INLINE CGS_CONSTEXPR_WITH_ASSERT 
        Texture(CreateInfo&& createInfo) noexcept
            : RenderResource(std::move(createInfo.ParentCreateInfo))
            , mIsBackBuffer(createInfo.IsBackBuffer)
        {
            const D3D12_RESOURCE_DESC desc = mData->GetDesc();
            mWidth = static_cast<uint32>(desc.Width);
            mHeight = static_cast<uint32>(desc.Height);
        }

        Texture(const Texture&) = delete;
        CGS_INLINE constexpr
        Texture(Texture&& other) noexcept = default;

        CGS_INLINE 
        ~Texture() noexcept
        {
            if(mIsBackBuffer)
            {
                if (mData != nullptr)
                {
                    mData->Release();
                    mData = nullptr;
                }
            }
        }

        Texture&
        operator=(const Texture&) = delete;
        CGS_INLINE constexpr
        Texture& operator=(Texture&& other) noexcept = default;

        CGS_INLINE void
        Initialize(CreateInfo&& createInfo) noexcept
        {
            RenderResource::Initialize(std::move(createInfo.ParentCreateInfo));
            mIsBackBuffer = createInfo.IsBackBuffer;
            const D3D12_RESOURCE_DESC desc = mData->GetDesc();
            mWidth = static_cast<uint32>(desc.Width);
            mHeight = static_cast<uint32>(desc.Height);
        }

        CGS_INLINE constexpr uint32
        GetWidth() const noexcept { return mWidth; }
        CGS_INLINE constexpr uint32
        GetHeight() const noexcept { return mHeight; }

    protected:
        CGS_INLINE void
        Initialize(RenderResource::CreateInfo&&) noexcept override {}

    private:
        bool mIsBackBuffer;
        uint32 mWidth;
        uint32 mHeight;
    };

    class IndexBuffer final : public RenderResource
    {
    public:
        struct CreateInfo final
        {
            RenderResource::CreateInfo  ParentCreateInfo;
            D3D12_INDEX_BUFFER_VIEW     View;
        };

    public:
        CGS_INLINE constexpr 
        IndexBuffer() noexcept
            : RenderResource()
        {
        }

        CGS_INLINE CGS_CONSTEXPR_WITH_ASSERT 
        IndexBuffer(CreateInfo&& createInfo) noexcept
            : RenderResource(std::move(createInfo.ParentCreateInfo))
            , mView(createInfo.View)
        {
        }

        IndexBuffer(const IndexBuffer&) = delete;
        CGS_INLINE CGS_CONSTEXPR_WITH_ASSERT
        IndexBuffer(IndexBuffer&& other) noexcept
            : RenderResource(std::move(other))
            , mView(other.mView)
        {}

        CGS_INLINE 
        ~IndexBuffer() noexcept = default;

        IndexBuffer&
        operator=(const IndexBuffer&) = delete;
        CGS_INLINE constexpr
        IndexBuffer& operator=(IndexBuffer&& other) noexcept
        {
            if (this != &other)
            {
                RenderResource::operator=(std::move(other));
                mView = other.mView;
            }
            return *this;
        }

        CGS_INLINE void
        Initialize(CreateInfo&& createInfo) noexcept
        {
            RenderResource::Initialize(std::move(createInfo.ParentCreateInfo));
            mView = createInfo.View;
        }

        CGS_INLINE constexpr const D3D12_INDEX_BUFFER_VIEW&
        GetIndexBufferView() const noexcept { return mView; }

    protected:
        CGS_INLINE void
        Initialize(RenderResource::CreateInfo&&) noexcept override {}

    private:
        D3D12_INDEX_BUFFER_VIEW    mView;
    };

    class VertexBuffer final : public RenderResource
    {
    public:
        struct CreateInfo final
        {
            RenderResource::CreateInfo  ParentCreateInfo;
            D3D12_VERTEX_BUFFER_VIEW    View;
        };

    public:
        CGS_INLINE constexpr 
        VertexBuffer() noexcept
            : RenderResource()
        {
        }

        CGS_INLINE CGS_CONSTEXPR_WITH_ASSERT 
        VertexBuffer(CreateInfo&& createInfo) noexcept
            : RenderResource(std::move(createInfo.ParentCreateInfo))
            , mView(createInfo.View)
        {
        }

        VertexBuffer(const VertexBuffer&) = delete;
        CGS_INLINE CGS_CONSTEXPR_WITH_ASSERT
        VertexBuffer(VertexBuffer&& other) noexcept
            : RenderResource(std::move(other))
            , mView(other.mView)
        {}

        CGS_INLINE 
        ~VertexBuffer() noexcept = default;

        VertexBuffer&
        operator=(const VertexBuffer&) = delete;
        CGS_INLINE constexpr
        VertexBuffer& operator=(VertexBuffer&& other) noexcept
        {
            if (this != &other)
            {
                RenderResource::operator=(std::move(other));
                mView = other.mView;
            }
            return *this;
        }

        CGS_INLINE void
        Initialize(CreateInfo&& createInfo) noexcept
        {
            RenderResource::Initialize(std::move(createInfo.ParentCreateInfo));
            mView = createInfo.View;
        }

        CGS_INLINE constexpr const D3D12_VERTEX_BUFFER_VIEW&
        GetVertexBufferView() const noexcept { return mView; }

    protected:
        CGS_INLINE void
        Initialize(RenderResource::CreateInfo&&) noexcept override {}

    private:
        D3D12_VERTEX_BUFFER_VIEW    mView;
    };

    class ConstantBuffer final : public RenderResource
    {
    public:
        CGS_INLINE constexpr 
        ConstantBuffer() noexcept
            : RenderResource()
        {
        }

        CGS_INLINE CGS_CONSTEXPR_WITH_ASSERT 
        ConstantBuffer(CreateInfo&& createInfo) noexcept
            : RenderResource(std::move(createInfo))
        {
        }

        ConstantBuffer(const ConstantBuffer&) = delete;
        CGS_INLINE CGS_CONSTEXPR_WITH_ASSERT
        ConstantBuffer(ConstantBuffer&& other) noexcept
            : RenderResource(std::move(other))
        {}

        CGS_INLINE 
        ~ConstantBuffer() noexcept = default;

        ConstantBuffer&
        operator=(const ConstantBuffer&) = delete;
        CGS_INLINE constexpr ConstantBuffer& 
        operator=(ConstantBuffer&& other) noexcept
        {
            if (this != &other)
            {
                RenderResource::operator=(std::move(other));
            }
            return *this;
        }
    };

    class Geometry final
    {
    public:
        CGS_INLINE constexpr 
        Geometry() noexcept: mIsEmissive(false), mName() {}
        CGS_INLINE constexpr
        Geometry(const std::string& name) noexcept: mIsEmissive(false), mName(name) {}
        CGS_INLINE
        ~Geometry() noexcept = default;

        CGS_INLINE constexpr void
        SetIsEmissive(const bool isEmissive) noexcept { mIsEmissive = isEmissive; }
        CGS_INLINE constexpr void
        SetColor(const float4& color) noexcept { mColor = color; }
        CGS_INLINE constexpr void
        SetColor(const Rgba8& color) noexcept { mColor = float4{ static_cast<float>(color.R) / 255.0f, static_cast<float>(color.G) / 255.0f, static_cast<float>(color.B) / 255.0f, static_cast<float>(color.A) / 255.0f }; }
        CGS_INLINE constexpr void
        SetVertexBuffer(VertexBuffer&& vertexBuffer) noexcept { mVertexBuffer = std::move(vertexBuffer); }
        CGS_INLINE constexpr void
        SetIndexBuffer(IndexBuffer&& indexBuffer) noexcept { mIndexBuffer = std::move(indexBuffer); }
        CGS_INLINE constexpr void
        SetName(const std::string& name) noexcept { mName = name; }

        [[nodiscard]] CGS_INLINE constexpr bool
        IsEmissive() const noexcept { return mIsEmissive; }
        [[nodiscard]] CGS_INLINE constexpr const float4&
        GetColor() const noexcept { return mColor; }
        [[nodiscard]] CGS_INLINE constexpr const VertexBuffer&
        GetVertexBuffer() const noexcept { return mVertexBuffer; }
        [[nodiscard]] CGS_INLINE constexpr const IndexBuffer&
        GetIndexBuffer() const noexcept { return mIndexBuffer; }
        [[nodiscard]] CGS_INLINE constexpr const std::string&
        GetName() const noexcept { return mName; }

    private:
        bool mIsEmissive;
        float4 mColor;
        VertexBuffer mVertexBuffer;
        IndexBuffer mIndexBuffer;
        std::string mName;
    };

    struct RendererCreateInfo final
    {
        uint32 Width;
        uint32 Height;
        HWND Window;
    };

    void
    DestroyRenderer(std::vector<std::unique_ptr<Geometry>>& geometries) noexcept;

    bool
    InitializeRenderer(const RendererCreateInfo& createInfo) noexcept;
}   // namespace cgs
#endif  // defined(CGS_GRAPHICS_API_D3D12)