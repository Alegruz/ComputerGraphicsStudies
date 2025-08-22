#pragma once

#include <cstdint>

namespace cgs
{
    enum class eCoordinateSpace : uint8_t
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

        CGS_INLINE constexpr float& operator[](size_t index) noexcept
        {
            return *(&X + index);
        }

        CGS_INLINE constexpr float operator[](size_t index) const noexcept
        {
            return *(&X + index);
        }
    };

    CGS_INLINE constexpr float3 operator+(const float3& lhs, const float3& rhs) noexcept
    {
        return float3{ lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z };
    }

    CGS_INLINE constexpr float3 operator-(const float3& lhs, const float3& rhs) noexcept
    {
        return float3{ lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z };
    }

    CGS_INLINE constexpr float Dot(const float3& lhs, const float3& rhs) noexcept
    {
        return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z;
    }

    CGS_INLINE constexpr float3 Cross(const float3& lhs, const float3& rhs) noexcept
    {
        return float3{
            lhs.Y * rhs.Z - lhs.Z * rhs.Y,
            lhs.Z * rhs.X - lhs.X * rhs.Z,
            lhs.X * rhs.Y - lhs.Y * rhs.X
        };
    }

    CGS_INLINE constexpr float3 ComputeBarycentricCoordinates(const float3& v0, const float3& v1, const float3& v2, const float3& point) noexcept
    {
        const float area = 0.5f * Dot(Cross(v1 - v0, v2 - v0), float3{ 0.0f, 0.0f, 1.0f });
        const float alpha = 0.5f * Dot(Cross(v2 - v1, point - v1), float3{ 0.0f, 0.0f, 1.0f }) / area;
        const float beta = 0.5f * Dot(Cross(v0 - v2, point - v2), float3{ 0.0f, 0.0f, 1.0f }) / area;
        const float gamma = 1.0f - alpha - beta;

        return float3{ alpha, beta, gamma };
    }

    template<eCoordinateSpace SPACE>
    using Coordinate = float3;
}