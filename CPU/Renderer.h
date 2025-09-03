#pragma once

#include "Common/Renderer.h"

#if defined(CGS_GRAPHICS_API_CPU)
namespace cgs
{
    class RenderResource
    {
    public:
        enum class eFormat : uint8
        {
            BGRA8_UNORM = 0,
            RGBA8_UNORM,
            RGB32_FLOAT,
            RGBA32_FLOAT,
            D8_UNORM,
            D32_UNORM,
            COUNT,
            DEFAULT = 0,
        };

        struct CreateInfo final
        {
            std::vector<eFormat>&& Formats;
            std::vector<byte>&& DataOrEmpty;
            uint32 ElementsCount = 0;
            std::string&& Name;
        };

    public:
        static constexpr uint32 FORMAT_STRIDE[] =
        {
            1 * 4,  // BGRA8_UNORM
            1 * 4,  // RGBA8_UNORM
            4 * 3,  // RGB32_FLOAT
            4 * 4,  // RGBA32_FLOAT
            1 * 1,  // D8_UNORM
            1 * 4,  // D32_UNORM
        };
        static_assert(CGS_ARRAYSIZE(FORMAT_STRIDE) == static_cast<uint32>(eFormat::COUNT));

    public:
        CGS_INLINE constexpr
        RenderResource() noexcept = default;
        CGS_INLINE constexpr
        RenderResource(CreateInfo&& createInfo) noexcept
            : mStrideInBytes()
            , mFormats(std::move(createInfo.Formats))
            , mData(std::move(createInfo.DataOrEmpty))
            , mName(std::move(createInfo.Name))
        {
            for (const eFormat format : mFormats)
            {
                mStrideInBytes += FORMAT_STRIDE[static_cast<uint32>(format)];
            }

            if (mData.size() < createInfo.ElementsCount * mStrideInBytes)
            {
                mData.resize(createInfo.ElementsCount * mStrideInBytes, 0);
            }
        }
        CGS_INLINE constexpr
        RenderResource(const RenderResource&) noexcept = default;
        CGS_INLINE constexpr
        RenderResource(RenderResource&&) noexcept = default;
        CGS_INLINE
        ~RenderResource() noexcept = default;

        CGS_INLINE constexpr RenderResource&
        operator=(const RenderResource&) noexcept = default;
        CGS_INLINE constexpr RenderResource&
        operator=(RenderResource&&) noexcept = default;

        CGS_INLINE constexpr void
        SetName(const std::string& name) noexcept { mName = name; }

        CGS_INLINE constexpr const byte*
        GetData() const noexcept { return mData.data(); }
        template<typename T>
        void 
        GetElementOrNull(const T*& outElementOrNull, const uint32 index) const noexcept;
        CGS_INLINE constexpr const std::string&
        GetName() const noexcept { return mName; }

    protected:
        uint32 mStrideInBytes;
        std::vector<eFormat> mFormats;
        std::vector<byte> mData;
        std::string mName;
    };
    
    bool
    Convert(void*& dst, const RenderResource::eFormat dstFormat, const void* src, const RenderResource::eFormat srcFormat) noexcept
    {
        if(dstFormat == srcFormat)
        {
            memcpy(dst, src, RenderResource::FORMAT_STRIDE[static_cast<uint32>(srcFormat)]);
            return true;
        }

        if(dstFormat == RenderResource::eFormat::BGRA8_UNORM && srcFormat == RenderResource::eFormat::RGBA8_UNORM)
        {
            const Rgba8& rgbaValue = *reinterpret_cast<const Rgba8*>(src);
            const Bgra8 convertedValue =
            {
                .B = rgbaValue.B,
                .G = rgbaValue.G,
                .R = rgbaValue.R,
                .A = rgbaValue.A
            };

            *reinterpret_cast<Bgra8*>(dst) = convertedValue;
            return true;
        }
        
        if(dstFormat == RenderResource::eFormat::RGBA8_UNORM && srcFormat == RenderResource::eFormat::BGRA8_UNORM)
        {
            const Bgra8& bgraValue = *reinterpret_cast<const Bgra8*>(src);
            const Rgba8 convertedValue =
            {
                .R = bgraValue.R,
                .G = bgraValue.G,
                .B = bgraValue.B,
                .A = bgraValue.A
            };

            *reinterpret_cast<Rgba8*>(dst) = convertedValue;
            return true;
        }

        return false;
    }

    // TODO(alegruz): Force handness and winding to be LHS and CCW?
    class VertexBuffer final : public RenderResource
    {
    public:
        CGS_INLINE constexpr
        VertexBuffer() noexcept = default;
        CGS_INLINE constexpr
        VertexBuffer(CreateInfo&& createInfo) noexcept
            : RenderResource(std::move(createInfo))
        {
        }
        CGS_INLINE 
        ~VertexBuffer() noexcept = default;

        template<typename T>
        [[nodiscard]] constexpr bool
        AddVertex(const T& vertex) noexcept;

        template<typename T>
        CGS_INLINE void 
        GetVertexOrNull(const T*& outVertex, const uint16 index) const noexcept { GetElementOrNull(outVertex, static_cast<uint32>(index)); }
        CGS_INLINE constexpr uint32
        GetVertexCount() const noexcept { if (mStrideInBytes > 0) { return static_cast<uint32>(mData.size()) / mStrideInBytes; } return 0; }
    };

    class Geometry final
    {
    public:
        CGS_INLINE constexpr 
        Geometry() noexcept: mIsEmissive(false), mVertexBuffer(), mIndices(), mColor(BLACK), mName() {}
        CGS_INLINE constexpr
        Geometry(const std::string& name) noexcept: mIsEmissive(false), mVertexBuffer(), mIndices(), mColor(BLACK), mName(name) {}
        CGS_INLINE
        ~Geometry() noexcept = default;

        CGS_INLINE constexpr void
        SetIsEmissive(const bool isEmissive) noexcept { mIsEmissive = isEmissive; }
        CGS_INLINE void
        SetVertexBuffer(VertexBuffer&& vertexBuffer) noexcept { mVertexBuffer = std::move(vertexBuffer); }
        CGS_INLINE constexpr void 
        SetIndices(std::vector<uint16>&& indices) noexcept { mIndices = std::move(indices); }
        CGS_INLINE constexpr void 
        SetColor(const Rgba8& color) noexcept { mColor = color; }

        [[nodiscard]] CGS_INLINE constexpr bool
        IsEmissive() const noexcept { return mIsEmissive; }
        [[nodiscard]] CGS_INLINE constexpr const VertexBuffer&
        GetVertexBuffer() const noexcept { return mVertexBuffer; }
        [[nodiscard]] CGS_INLINE constexpr const std::vector<uint16>&
        GetIndices() const noexcept { return mIndices; }
        [[nodiscard]] CGS_INLINE constexpr const Rgba8&
        GetColor() const noexcept { return mColor; }
        [[nodiscard]] CGS_INLINE constexpr const std::string&
        GetName() const noexcept { return mName; }

    private:
        bool mIsEmissive;
        VertexBuffer mVertexBuffer;
        std::vector<uint16> mIndices;
        Rgba8 mColor;
        std::string mName;
    };

    class Texture final : public RenderResource
    {
    public:
        struct CreateInfo final
        {
            eFormat Format;
            uint32 Width;
            uint32 Height;
            uint32 Depth;
            std::string&& Name;
        };

    public:
        CGS_INLINE constexpr
        Texture() noexcept = default;
        CGS_INLINE constexpr
        Texture(CreateInfo&& createInfo) noexcept
            : RenderResource
            (
                RenderResource::CreateInfo
                {
                    .Formats = std::vector<eFormat>{ createInfo.Format },
                    .DataOrEmpty = std::vector<byte>(),
                    .ElementsCount = createInfo.Width * createInfo.Height * createInfo.Depth,
                    .Name = std::move(createInfo.Name),
                }
            )
            , mWidth(createInfo.Width)
            , mHeight(createInfo.Height)
            , mDepth(createInfo.Depth)
        {
        }
        CGS_INLINE constexpr 
        Texture(const Texture&) noexcept = default;
        CGS_INLINE constexpr 
        Texture(Texture&&) noexcept = default;
        CGS_INLINE 
        ~Texture() noexcept = default;

        CGS_INLINE constexpr Texture& 
        operator=(const Texture&) noexcept = default;
        CGS_INLINE constexpr Texture& 
        operator=(Texture&&) noexcept = default;

        CGS_INLINE constexpr uint32
        GetWidth() const noexcept { return mWidth; }
        CGS_INLINE constexpr uint32 
        GetHeight() const noexcept { return mHeight; }
        CGS_INLINE constexpr uint32
        GetDepth() const noexcept { return mDepth; }
        template<typename T>
        CGS_INLINE constexpr bool
        GetFragment(T& outFragment, const eFormat format, const uint32 x, const uint32 y) const noexcept { return GetFragment(outFragment, format, x, y, 0); }
        template<typename T>
        CGS_INLINE constexpr bool
        GetFragment(T& outFragment, const eFormat format, const uint32 x, const uint32 y, const uint32 z) const noexcept
        {
            if(sizeof(T) != mStrideInBytes)
            {
                assert(false && "GetFragment: type size mismatch");
                return false;
            }

            if (x < mWidth && y < mHeight && z < mDepth)
            {
                size_t index = (z * mWidth * mHeight + y * mWidth + x) * mStrideInBytes;
                // outFragment = *reinterpret_cast<const T*>(&mData[index]);
                size_t offset = 0;
                for(uint32 i = 0; i < mFormats.size(); ++i)
                {
                    void* dst = &outFragment + offset;
                    Convert(dst, format, &mData[index + offset], mFormats[i]);
                    offset += RenderResource::FORMAT_STRIDE[static_cast<uint32>(mFormats[i])];
                }
                return true;
            }
            assert(false && "GetFragment: coordinates out of bounds");
            return false;
        }

        CGS_INLINE constexpr void 
        Clear() noexcept { std::fill(mData.begin(), mData.end(), static_cast<byte>(0)); }
        template<typename T>
        CGS_INLINE constexpr void
        Clear(const T& value) noexcept 
        { 
            for (uint32 z = 0; z < mDepth; ++z) 
            { 
                for (uint32 y = 0; y < mHeight; ++y) 
                { 
                    for (uint32 x = 0; x < mWidth; ++x) 
                    { 
                        size_t index = (z * mWidth * mHeight + y * mWidth + x) * mStrideInBytes;
                        // *reinterpret_cast<T*>(&mData[index]) = value;
                        size_t offset = 0;
                        for(uint32 i = 0; i < mFormats.size(); ++i)
                        {
                            const size_t stride = RenderResource::FORMAT_STRIDE[static_cast<uint32>(mFormats[i])];
                            memcpy(&mData[index + offset], &value + offset, stride);
                            offset += stride;
                        }
                    } 
                } 
            } 
        }
        template<typename T>
        CGS_INLINE constexpr bool
        SetFragmentValue(const uint32 x, const uint32 y, const T& value, const eFormat format) noexcept
        {
            return SetFragmentValue<T>(x, y, 0, value, format);
        }
        template<typename T>
        CGS_INLINE constexpr bool
        SetFragmentValue(const uint32 x, const uint32 y, const uint32 z, const T& value, const eFormat format) noexcept
        {
            if (sizeof(T) != mStrideInBytes)
            {
                assert(false && "SetFragmentValue: type size mismatch");
                return false;
            }

            if (x < mWidth && y < mHeight && z < mDepth)
            {
                size_t index = (z * mWidth * mHeight + y * mWidth + x) * mStrideInBytes;
                // *reinterpret_cast<T*>(&mData[index]) = value;
                size_t offset = 0;
                for(uint32 i = 0; i < mFormats.size(); ++i)
                {
                    void* dst = &mData[index + offset];
                    Convert(dst, mFormats[i], &value + offset, format);
                    offset += RenderResource::FORMAT_STRIDE[static_cast<uint32>(mFormats[i])];
                }
                return true;
            }
            return false;
        }

    private:
        uint32 mWidth;
        uint32 mHeight;
        uint32 mDepth;
    };

    using PFN_Present = void(*)(const Texture& backBuffer);

    extern PFN_Present gPresentFunc;
    
    struct RendererCreateInfo final
    {
        uint32 Width;
        uint32 Height;
    };

    struct CpuRenderWork final
    {
        Texture& OutTexture;
        Texture& OutDepthBuffer;
        RenderWork& Work;
    };

    struct SubRenderWork final
    {
        CpuRenderWork& ParentRenderWork;
        
        const Geometry& CurrentGeometry;
        const Geometry& EmissiveGeometry;

        const CornellBoxVertexShaderOutput V0;
        const CornellBoxVertexShaderOutput V1;
        const CornellBoxVertexShaderOutput V2;

        uint32 MinX = 0;
        uint32 MaxX = 0;
        uint32 MinY = 0;
        uint32 MaxY = 0;

        uint64 WorkIndex = 0;
    };

    struct SubRenderThreadInfo final
    {
        std::shared_ptr<ThreadHandle> CurrentThreadHandle;
        eRenderMethod RenderMethod;

        std::mutex RenderWorksMutex;
        std::queue<SubRenderWork> SubRenderWorks;
        std::atomic<uint64> LastCompleteWorkIndex;
        std::atomic<bool> IsActive;

        CGS_INLINE SubRenderThreadInfo() noexcept
            : CurrentThreadHandle(nullptr)
            , RenderMethod(eRenderMethod::DEFAULT)
            , LastCompleteWorkIndex(std::numeric_limits<uint64>::max())
            , IsActive(true)
        {
        }
        SubRenderThreadInfo(const SubRenderThreadInfo&) = delete;
        CGS_INLINE SubRenderThreadInfo(SubRenderThreadInfo&& other) noexcept
        {
            *this = std::move(other);
        }
        CGS_INLINE ~SubRenderThreadInfo() noexcept = default;

        SubRenderThreadInfo& operator=(const SubRenderThreadInfo&) = delete;
        CGS_INLINE SubRenderThreadInfo& operator=(SubRenderThreadInfo&& other) noexcept
        {
            if (this != &other)
            {
                std::lock_guard<std::mutex> lockGuard(other.RenderWorksMutex);
                CurrentThreadHandle = std::move(other.CurrentThreadHandle);
                RenderMethod = other.RenderMethod;
                SubRenderWorks = std::move(other.SubRenderWorks);
                LastCompleteWorkIndex = other.LastCompleteWorkIndex.load();
                IsActive = other.IsActive.load();
            }
            return *this;
        }
    };

    constexpr void
    DestroyRenderer() noexcept;

    bool
    InitializeRenderer(const RendererCreateInfo& createInfo) noexcept;

    template<eRasterizationMethod METHOD = eRasterizationMethod::DEFAULT>
    void
    Rasterize(CpuRenderWork& renderWork) noexcept;

    void
    SubRasterize(SubRenderWork& work) noexcept;
}   // namespace cgs
#endif  // defined(CGS_GRAPHICS_API_CPU)