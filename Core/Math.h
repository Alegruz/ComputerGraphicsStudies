#pragma once

#include <cmath>

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

    CGS_INLINE constexpr float3&
    operator+=(float3& lhs, const float3& rhs) noexcept
    {
        lhs.X += rhs.X;
        lhs.Y += rhs.Y;
        lhs.Z += rhs.Z;
        return lhs;
    }

    CGS_INLINE constexpr float3&
    operator-=(float3& lhs, const float3& rhs) noexcept
    {
        lhs.X -= rhs.X;
        lhs.Y -= rhs.Y;
        lhs.Z -= rhs.Z;
        return lhs;
    }

    CGS_INLINE constexpr float3&
    operator*=(float3& lhs, const float rhs) noexcept
    {
        lhs.X *= rhs;
        lhs.Y *= rhs;
        lhs.Z *= rhs;
        return lhs;
    }

    CGS_INLINE constexpr float3&
    operator/=(float3& lhs, const float rhs) noexcept
    {
        lhs.X /= rhs;
        lhs.Y /= rhs;
        lhs.Z /= rhs;
        return lhs;
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
        CGS_INLINE constexpr float4&
        operator=(const float3& other) noexcept
        {
            if(static_cast<const void*>(this) != static_cast<const void*>(&other))
            {
                X = other.X;
                Y = other.Y;
                Z = other.Z;
                W = 0.0f;
            }

            return *this;
        }
        CGS_INLINE constexpr float4&
        operator=(float3&& other) noexcept
        {
            if(static_cast<const void*>(this) != static_cast<const void*>(&other))
            {
                X = other.X;
                Y = other.Y;
                Z = other.Z;
                W = 0.0f;
            }

            return *this;
        }

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

    CGS_INLINE constexpr float4&
    operator+=(float4& lhs, const float4& rhs) noexcept
    {
        lhs.X += rhs.X;
        lhs.Y += rhs.Y;
        lhs.Z += rhs.Z;
        lhs.W += rhs.W;
        return lhs;
    }

    CGS_INLINE constexpr float4&
    operator+=(float4& lhs, const float3& rhs) noexcept
    {
        lhs.X += rhs.X;
        lhs.Y += rhs.Y;
        lhs.Z += rhs.Z;
        return lhs;
    }

    CGS_INLINE constexpr float4&
    operator-=(float4& lhs, const float4& rhs) noexcept
    {
        lhs.X -= rhs.X;
        lhs.Y -= rhs.Y;
        lhs.Z -= rhs.Z;
        lhs.W -= rhs.W;
        return lhs;
    }

    CGS_INLINE constexpr float4&
    operator*=(float4& lhs, const float rhs) noexcept
    {
        lhs.X *= rhs;
        lhs.Y *= rhs;
        lhs.Z *= rhs;
        lhs.W *= rhs;
        return lhs;
    }

    CGS_INLINE constexpr float4&
    operator/=(float4& lhs, const float rhs) noexcept
    {
        lhs.X /= rhs;
        lhs.Y /= rhs;
        lhs.Z /= rhs;
        lhs.W /= rhs;
        return lhs;
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
        float4 Data[4];

        CGS_INLINE constexpr
        float4x4() noexcept
            : Data{ { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } }
        {
        }

        CGS_INLINE constexpr
        float4x4(const float4& col0, const float4& col1, const float4& col2, const float4& col3) noexcept
            : Data{ col0, col1, col2, col3 }
        {
        }

        CGS_INLINE constexpr
        float4x4(const float4x4& other) noexcept
            : Data{ other.Data[0],
                    other.Data[1],
                    other.Data[2],
                    other.Data[3] }
        {
        }

        CGS_INLINE constexpr
        float4x4(float4x4&& other) noexcept
            : Data{ std::move(other.Data[0]),
                    std::move(other.Data[1]),
                    std::move(other.Data[2]),
                    std::move(other.Data[3]) }
        {
        }

        CGS_INLINE constexpr void
        Translate(const float3& translation) noexcept
        {
            Data[0].W += translation.X;
            Data[1].W += translation.Y;
            Data[2].W += translation.Z;
        }

        CGS_INLINE constexpr float4x4&
        operator=(const float4x4& other) noexcept
        {
            if(this != &other)
            {
                Data[0] = other.Data[0];
                Data[1] = other.Data[1];
                Data[2] = other.Data[2];
                Data[3] = other.Data[3];
            }
            return *this;
        }

        CGS_INLINE constexpr float4x4&
        operator=(float4x4&& other) noexcept
        {
            if(this != &other)
            {
                Data[0] = std::move(other.Data[0]);
                Data[1] = std::move(other.Data[1]);
                Data[2] = std::move(other.Data[2]);
                Data[3] = std::move(other.Data[3]);
            }
            return *this;
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

    CGS_INLINE constexpr float4x4 
    operator*(const float4x4& lhs, const float4x4& rhs) noexcept
    {
        return float4x4
        {
            lhs * rhs.Data[0],
            lhs * rhs.Data[1],
            lhs * rhs.Data[2],
            lhs * rhs.Data[3],
        };
    }

    CGS_INLINE constexpr bool
    GetInverse(const float4x4& matrix, float4x4& out) noexcept
    {
        // ----- fast path: affine matrix -----
        const bool isAffine =
            matrix.Data[3].X == 0.0f && matrix.Data[3].Y == 0.0f &&
            matrix.Data[3].Z == 0.0f && matrix.Data[3].W == 1.0f;

        if (isAffine)
        {
            // Upper-left 3x3
            const float a00 = matrix.Data[0].X, a01 = matrix.Data[0].Y, a02 = matrix.Data[0].Z;
            const float a10 = matrix.Data[1].X, a11 = matrix.Data[1].Y, a12 = matrix.Data[1].Z;
            const float a20 = matrix.Data[2].X, a21 = matrix.Data[2].Y, a22 = matrix.Data[2].Z;

            // det and adjugate of 3x3
            const float c00 =  (a11*a22 - a12*a21);
            const float c01 = -(a10*a22 - a12*a20);
            const float c02 =  (a10*a21 - a11*a20);

            const float c10 = -(a01*a22 - a02*a21);
            const float c11 =  (a00*a22 - a02*a20);
            const float c12 = -(a00*a21 - a01*a20);

            const float c20 =  (a01*a12 - a02*a11);
            const float c21 = -(a00*a12 - a02*a10);
            const float c22 =  (a00*a11 - a01*a10);

            const float det3 = a00*c00 + a01*c01 + a02*c02;
            if (det3 == 0.0f) 
            {
                return false;
            }
            const float invDet3 = 1.0f / det3;

            // inverse 3x3 = adj(M3)^T / det
            const float r00 = c00 * invDet3, r01 = c10 * invDet3, r02 = c20 * invDet3;
            const float r10 = c01 * invDet3, r11 = c11 * invDet3, r12 = c21 * invDet3;
            const float r20 = c02 * invDet3, r21 = c12 * invDet3, r22 = c22 * invDet3;

            // translation column
            const float tx = matrix.Data[0].W;
            const float ty = matrix.Data[1].W;
            const float tz = matrix.Data[2].W;

            // new translation = -R3x3 * t
            const float itx = -(r00*tx + r01*ty + r02*tz);
            const float ity = -(r10*tx + r11*ty + r12*tz);
            const float itz = -(r20*tx + r21*ty + r22*tz);

            out.Data[0] = { r00, r01, r02, itx };
            out.Data[1] = { r10, r11, r12, ity };
            out.Data[2] = { r20, r21, r22, itz };
            out.Data[3] = { 0.0f, 0.0f, 0.0f, 1.0f };
            return true;
        }
        
        // ----- general path: Gauss–Jordan with partial pivoting (double intermediates) -----
        double a[4][8] = { 0.0, };
        // left = m, right = I
        for (int r = 0; r < 4; ++r)
        {
            const float4 row = matrix.Data[r];
            a[r][0] = row.X; a[r][1] = row.Y; a[r][2] = row.Z; a[r][3] = row.W;
            a[r][4] = (r == 0); a[r][5] = (r == 1); a[r][6] = (r == 2); a[r][7] = (r == 3);
        }

        // Forward elimination
        for (int col = 0; col < 4; ++col)
        {
            // pivot: choose best row >= col
            int piv = col;
            double maxAbs = std::fabs(a[piv][col]);
            for (int r = col + 1; r < 4; ++r)
            {
                double v = std::fabs(a[r][col]);
                if (v > maxAbs) 
                { 
                    maxAbs = v; 
                    piv = r; 
                }
            }
            if (maxAbs == 0.0) 
            {
                return false;
            }

            // swap rows
            if (piv != col) 
            {
                for (int k = 0; k < 8; ++k) std::swap(a[piv][k], a[col][k]);
            }

            // scale pivot row
            const double inv = 1.0 / a[col][col];
            for (int k = 0; k < 8; ++k) 
            {
                a[col][k] *= inv;
            }

            // eliminate other rows
            for (int r = 0; r < 4; ++r) 
            {
                if (r == col) 
                {
                    continue;
                }
                const double f = a[r][col];
                if (f == 0.0) 
                {
                    continue;
                }
                for (int k = 0; k < 8; ++k) 
                {
                    a[r][k] -= f * a[col][k];
                }
            }
        }

        // Extract right half as inverse
        for (int r = 0; r < 4; ++r) 
        {
            out.Data[r].X = static_cast<float>(a[r][4]);
            out.Data[r].Y = static_cast<float>(a[r][5]);
            out.Data[r].Z = static_cast<float>(a[r][6]);
            out.Data[r].W = static_cast<float>(a[r][7]);
        }
        return true;
    }

    template<eCoordinateSpace SPACE>
    using Coordinate = float3;
    template<eCoordinateSpace SPACE>
    using Direction = float3;
    template<eCoordinateSpace SPACE>
    using HomogenousCoordinate = float4;

    template<std::unsigned_integral T>
    CGS_INLINE constexpr T
    Align(const T value, const T alignment) noexcept
    {
        return ((value + alignment - 1) / alignment) * alignment;
    }
}