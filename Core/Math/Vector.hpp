#include "Core/Math/Vector.h"

#include <cmath>

#include "Core/pch.h"
#include "Core/Concepts.h"

namespace cgs::core::math
{
	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr Vector2<ArithmeticType>::Vector2(const ArithmeticType x, const ArithmeticType y) noexcept
		: mData{ x, y }
	{
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr Vector2<ArithmeticType> Vector2<ArithmeticType>::operator+(const Vector2& other) noexcept
	{
		return Vector2(X + other.X, Y + other.Y);
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr Vector2<ArithmeticType> Vector2<ArithmeticType>::operator-(const Vector2& other) noexcept
	{
		return Vector2(X - other.X, Y - other.Y);
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr ArithmeticType Vector2<ArithmeticType>::Dot(const Vector2& other) noexcept
	{
		return X * other.X + Y * other.Y;
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr ArithmeticType Vector2<ArithmeticType>::LengthSquared() noexcept
	{
		return Dot(*this);
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr ArithmeticType Vector2<ArithmeticType>::Length() noexcept
	{
		return std::sqrt(LengthSquared());
	}

	template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector2<ArithmeticType> operator*(const ArithmeticType s, const Vector2<ArithmeticType>& other) noexcept
	{
		return Vector2<ArithmeticType>(s * other.X, s * other.Y);
	}

	template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector2<ArithmeticType> operator*(const Vector2<ArithmeticType>& other, const ArithmeticType s) noexcept
	{
		return s * other;
	}

	template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector2<ArithmeticType> operator/(const ArithmeticType s, const Vector2<ArithmeticType>& other) noexcept
	{
		return Vector2<ArithmeticType>(s / other.X, s / other.Y);
	}

	template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector2<ArithmeticType> operator/(const Vector2<ArithmeticType>& other, const ArithmeticType s) noexcept
	{
		return Vector2<ArithmeticType>(other.X / s, other.Y / s);
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr Vector3<ArithmeticType>::Vector3(const ArithmeticType x, const ArithmeticType y, const ArithmeticType z) noexcept
		: mData{ x, y, z }
	{
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr Vector3<ArithmeticType> Vector3<ArithmeticType>::operator+(const Vector3& other) noexcept
	{
		return Vector3(X + other.X, Y + other.Y, Z + other.Z);
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr Vector3<ArithmeticType> Vector3<ArithmeticType>::operator-(const Vector3& other) noexcept
	{
		return Vector3(X - other.X, Y - other.Y, Z - other.Z);
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr ArithmeticType Vector3<ArithmeticType>::Dot(const Vector3& other) noexcept
	{
		return X * other.X + Y * other.Y + Z * other.Z;
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr ArithmeticType Vector3<ArithmeticType>::LengthSquared() noexcept
	{
		return Dot(*this);
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr ArithmeticType Vector3<ArithmeticType>::Length() noexcept
	{
		return std::sqrt(LengthSquared());
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr Vector3<ArithmeticType> Vector3<ArithmeticType>::Cross(const Vector3& other) noexcept
	{
		return Vector3(Y * other.Z - Z * other.Y, Z * other.X - X * other.Z, X * other.Y - Y * other.X);
	}

	template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector3<ArithmeticType> operator*(const ArithmeticType s, const Vector3<ArithmeticType>& other) noexcept
	{
		return Vector3<ArithmeticType>(s * other.X, s * other.Y, s * other.Z);
	}

	template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector3<ArithmeticType> operator*(const Vector3<ArithmeticType>& other, const ArithmeticType s) noexcept
	{
		return s * other;
	}

	template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector3<ArithmeticType> operator/(const ArithmeticType s, const Vector3<ArithmeticType>& other) noexcept
	{
		return Vector3<ArithmeticType>(s / other.X, s / other.Y, s / other.Z);
	}

	template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector3<ArithmeticType> operator/(const Vector3<ArithmeticType>& other, const ArithmeticType s) noexcept
	{
		return Vector3<ArithmeticType>(other.X / s, other.Y / s, other.Z / s);
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr Vector4<ArithmeticType>::Vector4(const ArithmeticType x, const ArithmeticType y, const ArithmeticType z, const ArithmeticType w) noexcept
		: mData{ x, y, z, w }
	{
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr Vector4<ArithmeticType> Vector4<ArithmeticType>::operator+(const Vector4& other) noexcept
	{
		return Vector4(X + other.X, Y + other.Y, Z + other.Z, W + other.W);
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr Vector4<ArithmeticType> Vector4<ArithmeticType>::operator-(const Vector4& other) noexcept
	{
		return Vector4(X - other.X, Y - other.Y, Z - other.Z, W - other.W);
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr ArithmeticType Vector4<ArithmeticType>::Dot(const Vector4& other) noexcept
	{
		return X * other.X + Y * other.Y + Z * other.Z + W * other.W;
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr ArithmeticType Vector4<ArithmeticType>::LengthSquared() noexcept
	{
		return Dot(*this);
	}

	template<CArithmeticType ArithmeticType>
	CGS_INLINE constexpr ArithmeticType Vector4<ArithmeticType>::Length() noexcept
	{
		return std::sqrt(LengthSquared());
	}

	template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector4<ArithmeticType> operator*(const ArithmeticType s, const Vector4<ArithmeticType>& other) noexcept
	{
		return Vector4<ArithmeticType>(s * other.X, s * other.Y, s * other.Z, s * other.W);
	}

	template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector4<ArithmeticType> operator*(const Vector4<ArithmeticType>& other, const ArithmeticType s) noexcept
	{
		return s * other;
	}

	template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector4<ArithmeticType> operator/(const ArithmeticType s, const Vector4<ArithmeticType>& other) noexcept
	{
		return Vector4<ArithmeticType>(s / other.X, s / other.Y, s / other.Z, s / other.W);
	}

	template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector4<ArithmeticType> operator/(const Vector4<ArithmeticType>& other, const ArithmeticType s) noexcept
	{
		return Vector4<ArithmeticType>(other.X / s, other.Y / s, other.Z / s, other.W / s);
	}
}