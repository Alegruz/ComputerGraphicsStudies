#pragma once

namespace cgs
{
    enum class eCoordinateSpace : uint8
    {
        DEFAULT,
        OBJECT = DEFAULT,
        LOCAL = DEFAULT,
        WORLD,
        VIEW,
        PERSPECTIVE,
        SCREEN,
        NORMALIZED_DEVICE_COORDINATE,
    };

    struct float3 final
    {
        float X = 0.0f;
        float Y = 0.0f;
        float Z = 0.0f;

        CGS_INLINE constexpr float& 
        operator[](size_t index) noexcept
        {
            return *(&X + index);
        }

        CGS_INLINE constexpr float 
        operator[](size_t index) const noexcept
        {
            return *(&X + index);
        }
    };

    CGS_INLINE constexpr float3 
    operator+(const float3& lhs, const float3& rhs) noexcept
    {
        return float3{ lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z };
    }

    CGS_INLINE constexpr float3 
    operator-(const float3& lhs, const float3& rhs) noexcept
    {
        return float3{ lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z };
    }

    CGS_INLINE constexpr float3
    operator*(const float3& lhs, const float rhs) noexcept
    {
        return float3{ lhs.X * rhs, lhs.Y * rhs, lhs.Z * rhs };
    }

    CGS_INLINE constexpr float3
    operator*(const float lhs, const float3& rhs) noexcept
    {
        return rhs * lhs;
    }

    CGS_INLINE constexpr float3
    operator/(const float3& lhs, const float rhs) noexcept
    {
        return float3{ lhs.X / rhs, lhs.Y / rhs, lhs.Z / rhs };
    }

    CGS_INLINE constexpr float 
    Dot(const float3& lhs, const float3& rhs) noexcept
    {
        return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z;
    }

    CGS_INLINE constexpr float3 
    Cross(const float3& lhs, const float3& rhs) noexcept
    {
        return float3{
            lhs.Y * rhs.Z - lhs.Z * rhs.Y,
            lhs.Z * rhs.X - lhs.X * rhs.Z,
            lhs.X * rhs.Y - lhs.Y * rhs.X
        };
    }

    CGS_INLINE constexpr float
    GetLengthSquared(const float3& vec) noexcept
    {
        return Dot(vec, vec);
    }

    CGS_INLINE float
    GetLength(const float3& vec) noexcept
    {
        return std::sqrt(GetLengthSquared(vec));
    }

    CGS_INLINE float3
    Normalize(const float3& vec) noexcept
    {
        const float length = GetLength(vec);
        if (length > 0.0f)
        {
            return vec / length;
        }
        return vec;
    }

    CGS_INLINE constexpr float3 
    ComputeBarycentricCoordinates(const float3& v0, const float3& v1, const float3& v2, const float3& point) noexcept
    {
        const float area = 0.5f * Dot(Cross(v1 - v0, v2 - v0), float3{ 0.0f, 0.0f, 1.0f });
        const float alpha = 0.5f * Dot(Cross(v2 - v1, point - v1), float3{ 0.0f, 0.0f, 1.0f }) / area;
        const float beta = 0.5f * Dot(Cross(v0 - v2, point - v2), float3{ 0.0f, 0.0f, 1.0f }) / area;
        const float gamma = 1.0f - alpha - beta;

        return float3{ alpha, beta, gamma };
    }

    struct float4 final
    {
        float X;
        float Y;
        float Z;
        float W;

        CGS_INLINE constexpr
        float4(const float x, const float y, const float z, const float w) noexcept
            : X(x), Y(y), Z(z), W(w)
        {
        }
        CGS_INLINE constexpr
        float4(const float3& vec, const float w) noexcept
            : float4(vec.X, vec.Y, vec.Z, w)
        {
        }
        CGS_INLINE constexpr
        float4() noexcept
            : float4(0.0f, 0.0f, 0.0f, 0.0f)
        {
        }
        CGS_INLINE constexpr
        float4(const float4&) noexcept = default;
        CGS_INLINE constexpr
        float4(float4&&) noexcept = default;   
        CGS_INLINE ~float4() noexcept = default;

        CGS_INLINE constexpr float4& 
        operator=(const float4&) noexcept = default;
        CGS_INLINE constexpr float4& 
        operator=(float4&&) noexcept = default;

        CGS_INLINE constexpr float& 
        operator[](size_t index) noexcept
        {
            return *(&X + index);
        }

        CGS_INLINE constexpr float 
        operator[](size_t index) const noexcept
        {
            return *(&X + index);
        }

        CGS_INLINE const float3&
        GetXYZ() const noexcept
        {
            return *reinterpret_cast<const float3*>(&X);
        }
    };

    CGS_INLINE constexpr float4 
    operator+(const float4& lhs, const float4& rhs) noexcept
    {
        return float4( lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z, lhs.W + rhs.W );
    }

    CGS_INLINE constexpr float4 
    operator-(const float4& lhs, const float4& rhs) noexcept
    {
        return float4( lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z, lhs.W - rhs.W );
    }

    CGS_INLINE constexpr float4
    operator*(const float4& lhs, const float rhs) noexcept
    {
        return float4( lhs.X * rhs, lhs.Y * rhs, lhs.Z * rhs, lhs.W * rhs );
    }

    CGS_INLINE constexpr float4
    operator*(const float lhs, const float4& rhs) noexcept
    {
        return rhs * lhs;
    }

    CGS_INLINE constexpr float4
    operator/(const float4& lhs, const float rhs) noexcept
    {
        return float4( lhs.X / rhs, lhs.Y / rhs, lhs.Z / rhs, lhs.W / rhs );
    }

    CGS_INLINE constexpr float 
    Dot(const float4& lhs, const float4& rhs) noexcept
    {
        return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z + lhs.W * rhs.W;
    }

    CGS_INLINE constexpr float4
    Cross(const float4& lhs, const float4& rhs) noexcept
    {
        return float4{
            lhs.Y * rhs.Z - lhs.Z * rhs.Y,
            lhs.Z * rhs.X - lhs.X * rhs.Z,
            lhs.X * rhs.Y - lhs.Y * rhs.X,
            0.0f
        };
    }

    CGS_INLINE constexpr float
    GetLengthSquared(const float4& vec) noexcept
    {
        return Dot(vec, vec);
    }

    CGS_INLINE float
    GetLength(const float4& vec) noexcept
    {
        return std::sqrt(GetLengthSquared(vec));
    }

    CGS_INLINE float4
    Normalize(const float4& vec) noexcept
    {
        const float length = GetLength(vec);
        if (length > 0.0f)
        {
            return vec / length;
        }
        return vec;
    }

    CGS_INLINE constexpr float
    SumOfElements(const float4& vec) noexcept
    {
        return vec.X + vec.Y + vec.Z + vec.W;
    }

    // Row-major
    struct float4x4 final
    {
        float4 Data[4] = { { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } };

        void
        Translate(const float3& translation) noexcept
        {
            Data[0].W += translation.X;
            Data[1].W += translation.Y;
            Data[2].W += translation.Z;
        }
    };

    CGS_INLINE constexpr float4 
    operator*(const float4x4& mat, const float4& vec) noexcept
    {
        return float4
        {
            mat.Data[0].X * vec.X + mat.Data[0].Y * vec.Y + mat.Data[0].Z * vec.Z + mat.Data[0].W * vec.W,
            mat.Data[1].X * vec.X + mat.Data[1].Y * vec.Y + mat.Data[1].Z * vec.Z + mat.Data[1].W * vec.W,
            mat.Data[2].X * vec.X + mat.Data[2].Y * vec.Y + mat.Data[2].Z * vec.Z + mat.Data[2].W * vec.W,
            mat.Data[3].X * vec.X + mat.Data[3].Y * vec.Y + mat.Data[3].Z * vec.Z + mat.Data[3].W * vec.W,
        };
    }

    template<eCoordinateSpace SPACE>
    using Coordinate = float3;
    template<eCoordinateSpace SPACE>
    using Direction = float3;
    template<eCoordinateSpace SPACE>
    using HomogenousCoordinate = float4;
}