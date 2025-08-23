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
        CGS_INLINE constexpr VertexBuffer() noexcept = default;
        CGS_INLINE constexpr VertexBuffer(CreateInfo&& createInfo) noexcept
            : mHandnessType(createInfo.HandnessType)
            , mWindingType(createInfo.WindingType)
            , mStrideInBytes(createInfo.StrideInBytes)
            , mData(std::move(createInfo.Data))
        {
        }
        CGS_INLINE ~VertexBuffer() noexcept = default;

        CGS_INLINE constexpr void SetHandnessType(const eHandnessType handnessType) noexcept { mHandnessType = handnessType; }
        CGS_INLINE constexpr void SetWindingType(const eWindingType windingType) noexcept { mWindingType = windingType; }
        template<typename T>
        [[nodiscard]] bool AddVertex(const T& vertex) noexcept;
        template<typename T>
        void GetVertexOrNull(const T*& outVertex, const uint16 index) const noexcept;

    private:
        eHandnessType mHandnessType;
        eWindingType mWindingType;
        uint32 mStrideInBytes;
        std::vector<byte> mData;
    };

    class Geometry final
    {
    public:
        CGS_INLINE constexpr Geometry() noexcept = default;
        CGS_INLINE ~Geometry() noexcept = default;

        CGS_INLINE constexpr void SetVertexBuffer(VertexBuffer&& vertexBuffer) noexcept { mVertexBuffer = std::move(vertexBuffer); }
        CGS_INLINE constexpr void SetIndices(std::vector<uint16>&& indices) noexcept { mIndices = std::move(indices); }

        CGS_INLINE constexpr const VertexBuffer& GetVertexBuffer() const noexcept { return mVertexBuffer; }
        CGS_INLINE constexpr const std::vector<uint16>& GetIndices() const noexcept { return mIndices; }

    private:
        VertexBuffer mVertexBuffer;
        std::vector<uint16> mIndices;
    };

    class Texture final
    {
    public:
        CGS_INLINE constexpr Texture() noexcept = default;
        CGS_INLINE constexpr Texture(const uint32 width, const uint32 height) noexcept
            : Texture(width, height, 0) {
        }
        CGS_INLINE constexpr Texture(const uint32 width, const uint32 height, const byte initialValue) noexcept
            : mWidth(width), mHeight(height), mData(width * height * 4, initialValue) {
        }
        CGS_INLINE constexpr Texture(const Texture&) noexcept = default;
        CGS_INLINE constexpr Texture(Texture&&) noexcept = default;
        CGS_INLINE ~Texture() noexcept = default;

        CGS_INLINE constexpr Texture& operator=(const Texture&) noexcept = default;
        CGS_INLINE constexpr Texture& operator=(Texture&&) noexcept = default;

        CGS_INLINE constexpr uint32 GetWidth() const noexcept { return mWidth; }
        CGS_INLINE constexpr uint32 GetHeight() const noexcept { return mHeight; }
        CGS_INLINE constexpr const byte* GetData() const noexcept { return mData.data(); }
        CGS_INLINE constexpr Rgba8 GetFragment(const uint32 x, const uint32 y) const noexcept
        {
            if (x < mWidth && y < mHeight)
            {
                const size_t index = (y * mWidth + x) * 4;
                return Rgba8(mData[index], mData[index + 1], mData[index + 2], mData[index + 3]);
            }
            return Rgba8(); // Out of bounds, return transparent black
        }

        CGS_INLINE constexpr void Clear() noexcept { std::fill(mData.begin(), mData.end(), static_cast<byte>(0)); }
        CGS_INLINE constexpr bool SetFragmentValue(const uint32 x, const uint32 y, const byte r, const byte g, const byte b, const byte a) noexcept
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

    template<eCoordinateSpace SPACE, eRasterizationMethod METHOD = eRasterizationMethod::DEFAULT>
    void
    Rasterize(Texture& outTexture, const std::vector<Geometry>& geometries) noexcept;
}