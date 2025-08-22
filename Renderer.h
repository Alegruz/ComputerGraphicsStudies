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

    template<eCoordinateSpace SPACE>
    class TriangleMesh final
    {
    public:
        CGS_INLINE constexpr TriangleMesh() = default;
        CGS_INLINE constexpr TriangleMesh(const Coordinate<SPACE>& v0, const Coordinate<SPACE>& v1, const Coordinate<SPACE>& v2) noexcept
            : mVertices{ v0, v1, v2 } {}
        CGS_INLINE constexpr TriangleMesh(const TriangleMesh& mesh) noexcept
            : mVertices{ mesh.mVertices[0], mesh.mVertices[1], mesh.mVertices[2] } {}
        CGS_INLINE constexpr TriangleMesh(TriangleMesh&&) noexcept = default;
        CGS_INLINE ~TriangleMesh() noexcept = default;

        CGS_INLINE constexpr TriangleMesh& operator=(const TriangleMesh& mesh) noexcept
        {
            if (this != &mesh)
            {
                mVertices[0] = mesh.mVertices[0];
                mVertices[1] = mesh.mVertices[1];
                mVertices[2] = mesh.mVertices[2];
            }
            return *this;
        }
        CGS_INLINE constexpr TriangleMesh& operator=(TriangleMesh&&) noexcept = default;
        CGS_INLINE constexpr const Coordinate<SPACE>& GetVertex(size_t index) const noexcept
        {
            return mVertices[index];
        }
        CGS_INLINE constexpr void SetVertex(size_t index, const Coordinate<SPACE>& vertex) noexcept
        {
            if (index < 3)
            {
                mVertices[index] = vertex;
            }
        }

    private:
        Coordinate<SPACE> mVertices[3];
    };

    template<eCoordinateSpace SPACE>
    CGS_INLINE constexpr Coordinate<SPACE> ComputeBarycentricCoordinates(const TriangleMesh<SPACE>& mesh, const float3& point) noexcept
    {
        const float3& v0 = mesh.GetVertex(0);
        const float3& v1 = mesh.GetVertex(1);
        const float3& v2 = mesh.GetVertex(2);

        return ComputeBarycentricCoordinates(v0, v1, v2, point);
    }

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
    Rasterize(Texture& outTexture, const std::vector<TriangleMesh<SPACE>>& meshes) noexcept;
}