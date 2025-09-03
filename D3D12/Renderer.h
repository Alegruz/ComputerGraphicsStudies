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

#define CGS_DESTROY_D3D12_OBJECT_OR_NULL(object)    \
    if (object != nullptr)                  \
    {                                       \
        [[maybe_unused]] const uint64 refCount = (object)->Release(); \
        assert(refCount == 0 && #object " was not released properly"); \
    }                                       

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
            CGS_DESTROY_D3D12_OBJECT_OR_NULL(mData);
        }

        RenderResource&
        operator=(const RenderResource&) = delete;
        CGS_INLINE constexpr
        RenderResource& operator=(RenderResource&& other) noexcept
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

        CGS_INLINE void
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

        CGS_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE&
        GetView() const noexcept
        {
            return mView;
        }

        void 
        Transition(ID3D12GraphicsCommandList& commandList, const D3D12_RESOURCE_STATES newState) noexcept;

        [[nodiscard]] CGS_INLINE bool
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

    private:
        D3DPtr<ID3D12Resource> mData;
        D3D12_CPU_DESCRIPTOR_HANDLE mView;
        D3D12_RESOURCE_STATES mState;
        std::string mName;
    };

    class Texture final : public RenderResource
    {
    public:
        CGS_INLINE constexpr 
        Texture() noexcept
            : RenderResource()
        {
        }

        CGS_INLINE CGS_CONSTEXPR_WITH_ASSERT 
        Texture(CreateInfo&& createInfo) noexcept
            : RenderResource(std::move(createInfo))
        {
        }

        Texture(const Texture&) = delete;
        CGS_INLINE constexpr
        Texture(Texture&& other) noexcept = default;

        CGS_INLINE 
        ~Texture() noexcept = default;

        Texture&
        operator=(const Texture&) = delete;
        CGS_INLINE constexpr
        Texture& operator=(Texture&& other) noexcept = default;
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

    private:
        D3D12_VERTEX_BUFFER_VIEW    mView;
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
        SetVertexBuffer(VertexBuffer&& vertexBuffer) noexcept { mVertexBuffer = std::move(vertexBuffer); }
        CGS_INLINE constexpr void
        SetIndexBuffer(IndexBuffer&& indexBuffer) noexcept { mIndexBuffer = std::move(indexBuffer); }
        CGS_INLINE constexpr void
        SetName(const std::string& name) noexcept { mName = name; }

        [[nodiscard]] CGS_INLINE constexpr bool
        IsEmissive() const noexcept { return mIsEmissive; }
        [[nodiscard]] CGS_INLINE constexpr const std::string&
        GetName() const noexcept { return mName; }

    private:
        bool mIsEmissive;
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
    DestroyRenderer() noexcept;

    bool
    InitializeRenderer(const RendererCreateInfo& createInfo) noexcept;
}   // namespace cgs
#endif  // defined(CGS_GRAPHICS_API_D3D12)