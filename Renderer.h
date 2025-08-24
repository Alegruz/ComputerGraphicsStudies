#pragma once

namespace cgs
{
    struct Rgba8 final
    {
        byte R = 0;
        byte G = 0;
        byte B = 0;
        byte A = 255;
    };

    static constexpr Rgba8 WHITE{ 255, 255, 255, 255 };
    static constexpr Rgba8 BLACK{ 0, 0, 0, 255 };
    static constexpr Rgba8 RED{ 255, 0, 0, 255 };
    static constexpr Rgba8 GREEN{ 0, 255, 0, 255 };
    static constexpr Rgba8 BLUE{ 0, 0, 255, 255 };

    enum class eHandnessType : uint8
    {
        LEFT,
        RIGHT,
    };

    enum class eWindingType : uint8
    {
        COUNTER_CLOCKWISE,
        CLOCKWISE,
    };

    struct VertexPN final
    {
        Coordinate<eCoordinateSpace::WORLD> Position;
        Coordinate<eCoordinateSpace::WORLD> Normal;
    };

    // TODO(alegruz): Force handness and winding to be LHS and CCW?
    class VertexBuffer final
    {
    public:
        struct CreateInfo final
        {
            eHandnessType HandnessType = eHandnessType::LEFT;
            eWindingType WindingType = eWindingType::COUNTER_CLOCKWISE;
            uint32 StrideInBytes = 0;
            std::vector<byte>&& Data;
        };

    public:
        CGS_INLINE constexpr
        VertexBuffer() noexcept = default;
        CGS_INLINE constexpr
        VertexBuffer(CreateInfo&& createInfo) noexcept
            : mHandnessType(createInfo.HandnessType)
            , mWindingType(createInfo.WindingType)
            , mStrideInBytes(createInfo.StrideInBytes)
            , mData(std::move(createInfo.Data))
        {
        }
        CGS_INLINE 
        ~VertexBuffer() noexcept = default;

        CGS_INLINE constexpr void
        SetHandnessType(const eHandnessType handnessType) noexcept { mHandnessType = handnessType; }
        CGS_INLINE constexpr void 
        SetWindingType(const eWindingType windingType) noexcept { mWindingType = windingType; }
        template<typename T>
        [[nodiscard]] constexpr bool
        AddVertex(const T& vertex) noexcept;

        CGS_INLINE constexpr eHandnessType 
        GetHandnessType() const noexcept { return mHandnessType; }
        CGS_INLINE constexpr eWindingType 
        GetWindingType() const noexcept { return mWindingType; }
        template<typename T>
        void 
        GetVertexOrNull(const T*& outVertex, const uint16 index) const noexcept;
        CGS_INLINE constexpr uint32
        GetVertexCount() const noexcept { if (mStrideInBytes > 0) { return static_cast<uint32>(mData.size()) / mStrideInBytes; } return 0; }

    private:
        eHandnessType mHandnessType;
        eWindingType mWindingType;
        uint32 mStrideInBytes;
        std::vector<byte> mData;
    };

    class Geometry final
    {
    public:
        CGS_INLINE constexpr 
        Geometry() noexcept: mIsEmissive(false), mVertexBuffer(), mIndices(), mColor(BLACK) {}
        CGS_INLINE
        ~Geometry() noexcept = default;

        CGS_INLINE constexpr void
        SetIsEmissive(const bool isEmissive) noexcept { mIsEmissive = isEmissive; }
        CGS_INLINE constexpr void
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

    private:
        bool mIsEmissive;
        VertexBuffer mVertexBuffer;
        std::vector<uint16> mIndices;
        Rgba8 mColor;
    };

    class Texture final
    {
    public:
        CGS_INLINE constexpr 
        Texture() noexcept = default;
        CGS_INLINE constexpr
        Texture(const uint32 width, const uint32 height) noexcept
            : Texture(width, height, 0) {}
        CGS_INLINE constexpr 
        Texture(const uint32 width, const uint32 height, const byte initialValue) noexcept
            : mWidth(width), mHeight(height), mData(width * height * 4, initialValue) {}
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
        CGS_INLINE constexpr const byte*
        GetData() const noexcept { return mData.data(); }
        CGS_INLINE constexpr Rgba8
        GetFragment(const uint32 x, const uint32 y) const noexcept
        {
            if (x < mWidth && y < mHeight)
            {
                const size_t index = (y * mWidth + x) * 4;
                return Rgba8{ mData[index], mData[index + 1], mData[index + 2], mData[index + 3] };
            }
            assert(false && "GetFragment: coordinates out of bounds");
            return Rgba8(); // Out of bounds, return transparent black
        }

        CGS_INLINE constexpr void 
        Clear() noexcept { std::fill(mData.begin(), mData.end(), static_cast<byte>(0)); }
        CGS_INLINE constexpr bool
        SetFragmentValue(const uint32 x, const uint32 y, const byte r, const byte g, const byte b, const byte a) noexcept
        {
            if (x < mWidth && y < mHeight)
            {
                const size_t index = (y * mWidth + x) * 4;
                mData[index + 0] = r;
                mData[index + 1] = g;
                mData[index + 2] = b;
                mData[index + 3] = a;
                return true;
            }
            return false;
        }

    private:
        uint32 mWidth = 0;
        uint32 mHeight = 0;
        std::vector<byte> mData;
    };

    extern cgs::Texture gBackBuffer;

    enum class eRasterizationMethod : uint8
    {
        DEFAULT,
        BARYCENTRIC = DEFAULT,
    };

    template<eRasterizationMethod METHOD = eRasterizationMethod::DEFAULT>
    void
    Rasterize(Texture& outTexture, const std::vector<Geometry>& geometries) noexcept;

    class Camera final
    {
    public:
        struct CreateInfo final
        {
            Coordinate<eCoordinateSpace::WORLD> Position;
            Coordinate<eCoordinateSpace::WORLD> Front;
            Coordinate<eCoordinateSpace::WORLD> Up;
        };

    public:
        CGS_INLINE constexpr 
        Camera() noexcept
            : mPosition()
            , mFront()
            , mUp()
        {
        }
        CGS_INLINE constexpr 
        Camera(CreateInfo&& createInfo) noexcept
            : mPosition(createInfo.Position)
            , mFront(createInfo.Front)
            , mUp(createInfo.Up)
        {
        }
        CGS_INLINE constexpr
        Camera(const Camera&) noexcept = default;
        CGS_INLINE constexpr
        Camera(Camera&&) noexcept = default;
        CGS_INLINE
        ~Camera() noexcept = default;

        CGS_INLINE constexpr Camera& 
        operator=(const Camera&) noexcept = default;
        CGS_INLINE constexpr Camera& 
        operator=(Camera&&) noexcept = default;

        CGS_INLINE constexpr Coordinate<eCoordinateSpace::WORLD>
        GetPosition() const noexcept { return mPosition; }
        CGS_INLINE constexpr Coordinate<eCoordinateSpace::WORLD>
        GetFront() const noexcept { return mFront; }
        CGS_INLINE constexpr Coordinate<eCoordinateSpace::WORLD>
        GetUp() const noexcept { return mUp; }

    private:
        Coordinate<eCoordinateSpace::WORLD> mPosition;
        Coordinate<eCoordinateSpace::WORLD> mFront;
        Coordinate<eCoordinateSpace::WORLD> mUp;
    };

    void
    CreateCornellBoxScene(std::vector<Geometry>& outGeometries) noexcept;

    struct CornellBoxVertexShaderOutput final
    {
        Coordinate<eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE> NdcPosition;
        Coordinate<eCoordinateSpace::WORLD> WsPosition;
        Coordinate<eCoordinateSpace::WORLD> Normal;
    };

    CornellBoxVertexShaderOutput
    CornellBoxVertexShader(const VertexPN& input) noexcept;

    struct CornellBoxFragmentShaderInput final
    {
        CornellBoxVertexShaderOutput VSOutput;
        Rgba8 Color;
        const Geometry& EmissiveGeometry;
    };
    
    Rgba8
    CornellBoxFragmentShader(const CornellBoxFragmentShaderInput& input) noexcept;

    struct RenderInfo final
    {
        Texture& outBackBuffer;
        const std::vector<Geometry>& geometries;
    };

    void*
    Render(void* arg) noexcept;
}