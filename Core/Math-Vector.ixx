module;

#include <cmath>

#include "Core/PchCommon.h"
#include "Core/Concepts.h"

export module Core.Math.Vector;

namespace cgs
{
namespace core
{
#define CGS_CORE_VECTOR2_SWIZZLE_METHOD_DEFINITION_HELPER(VectorType, Component0, Component1)	\
	CGS_INLINE constexpr VectorType Component0##Component1() noexcept { return VectorType(Component0, Component1); }

#define CGS_CORE_VECTOR2_SWIZZLE_METHOD_DEFINITIONS(Component0, Component1)	\
	CGS_CORE_VECTOR2_SWIZZLE_METHOD_DEFINITION_HELPER(Vector2<ArithmeticType>, Component0, Component0)	\
	CGS_CORE_VECTOR2_SWIZZLE_METHOD_DEFINITION_HELPER(Vector2<ArithmeticType>, Component0, Component1)	\
	CGS_CORE_VECTOR2_SWIZZLE_METHOD_DEFINITION_HELPER(Vector2<ArithmeticType>, Component1, Component0)	\
	CGS_CORE_VECTOR2_SWIZZLE_METHOD_DEFINITION_HELPER(Vector2<ArithmeticType>, Component1, Component1)

#define CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(VectorType, Component0, Component1, Component2)	\
	CGS_INLINE constexpr VectorType Component0##Component1##Component2() noexcept { return VectorType(Component0, Component1, Component2); }

#define CGS_CORE_VECTOR2_3_SWIZZLE_METHOD_DEFINITIONS(Component0, Component1, Component2)	\
	CGS_CORE_VECTOR2_SWIZZLE_METHOD_DEFINITION_HELPER(Vector2<ArithmeticType>, Component0, Component0)	\
	CGS_CORE_VECTOR2_SWIZZLE_METHOD_DEFINITION_HELPER(Vector2<ArithmeticType>, Component0, Component1)	\
	CGS_CORE_VECTOR2_SWIZZLE_METHOD_DEFINITION_HELPER(Vector2<ArithmeticType>, Component0, Component2)	\
	CGS_CORE_VECTOR2_SWIZZLE_METHOD_DEFINITION_HELPER(Vector2<ArithmeticType>, Component1, Component0)	\
	CGS_CORE_VECTOR2_SWIZZLE_METHOD_DEFINITION_HELPER(Vector2<ArithmeticType>, Component1, Component1)	\
	CGS_CORE_VECTOR2_SWIZZLE_METHOD_DEFINITION_HELPER(Vector2<ArithmeticType>, Component1, Component2)	\
	CGS_CORE_VECTOR2_SWIZZLE_METHOD_DEFINITION_HELPER(Vector2<ArithmeticType>, Component2, Component0)	\
	CGS_CORE_VECTOR2_SWIZZLE_METHOD_DEFINITION_HELPER(Vector2<ArithmeticType>, Component2, Component1)	\
	CGS_CORE_VECTOR2_SWIZZLE_METHOD_DEFINITION_HELPER(Vector2<ArithmeticType>, Component2, Component2)

#define CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITIONS(Component0, Component1, Component2)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component0, Component0, Component0)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component0, Component0, Component1)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component0, Component0, Component2)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component0, Component1, Component0)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component0, Component1, Component1)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component0, Component1, Component2)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component0, Component2, Component0)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component0, Component2, Component1)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component0, Component2, Component2)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component1, Component0, Component1)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component1, Component0, Component2)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component1, Component1, Component0)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component1, Component1, Component1)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component1, Component1, Component2)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component1, Component2, Component0)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component1, Component2, Component1)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component1, Component2, Component2)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component2, Component0, Component1)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component2, Component0, Component2)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component2, Component1, Component0)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component2, Component1, Component1)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component2, Component1, Component2)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component2, Component2, Component0)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component2, Component2, Component1)	\
	CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITION_HELPER(Vector3<ArithmeticType>, Component2, Component2, Component2)

	export template<CArithmeticType ArithmeticType>
	struct CORE_API Vector2
	{
	public:
		CGS_INLINE explicit constexpr Vector2(const ArithmeticType x, const ArithmeticType y) noexcept
			: mData{ x, y }
		{
		}
		CGS_INLINE explicit constexpr Vector2(const Vector2& other) noexcept = default;
		CGS_INLINE explicit constexpr Vector2(Vector2&& other) noexcept = default;
		CGS_INLINE constexpr ~Vector2() noexcept = default;

		CGS_INLINE constexpr Vector2& operator=(const Vector2& other) noexcept = default;
		CGS_INLINE constexpr Vector2& operator=(Vector2&& other) noexcept = default;

	public:
		CGS_INLINE constexpr Vector2 operator+(const Vector2& other) noexcept
		{
			return Vector2(X + other.X, Y + other.Y);
		}

		CGS_INLINE constexpr Vector2 operator-(const Vector2& other) noexcept
		{
			return Vector2(X - other.X, Y - other.Y);
		}

		CGS_INLINE constexpr ArithmeticType Dot(const Vector2& other) noexcept
		{
			return X * other.X + Y * other.Y;
		}

		CGS_INLINE constexpr ArithmeticType LengthSquared() noexcept
		{
			return Dot(*this);
		}

		CGS_INLINE constexpr ArithmeticType Length() noexcept
		{
			return std::sqrt(LengthSquared());
		}

		CGS_CORE_VECTOR2_SWIZZLE_METHOD_DEFINITIONS(X, Y)

	public:
		union
		{
			ArithmeticType mData[2];
			struct
			{
				ArithmeticType X;
				ArithmeticType Y;
			};
		};
	};

	export template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector2<ArithmeticType> operator*(const ArithmeticType s, const Vector2<ArithmeticType>& other) noexcept
	{
		return Vector2<ArithmeticType>(s * other.X, s * other.Y);
	}

	export template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector2<ArithmeticType> operator*(const Vector2<ArithmeticType>& other, const ArithmeticType s) noexcept
	{
		return s * other;
	}

	export template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector2<ArithmeticType> operator/(const ArithmeticType s, const Vector2<ArithmeticType>& other) noexcept
	{
		return Vector2<ArithmeticType>(s / other.X, s / other.Y);
	}

	export template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector2<ArithmeticType> operator/(const Vector2<ArithmeticType>& other, const ArithmeticType s) noexcept
	{
		return Vector2<ArithmeticType>(other.X / s, other.Y / s);
	}

	export template<CArithmeticType ArithmeticType>
	struct CORE_API Vector3
	{
	public:
		CGS_INLINE explicit constexpr Vector3(const ArithmeticType x, const ArithmeticType y, const ArithmeticType z) noexcept
			: mData{ x, y, z }
		{
		}
		CGS_INLINE explicit constexpr Vector3(const Vector3& other) noexcept = default;
		CGS_INLINE explicit constexpr Vector3(Vector3&& other) noexcept = default;
		CGS_INLINE constexpr ~Vector3() noexcept = default;

		CGS_INLINE constexpr Vector3& operator=(const Vector3& other) noexcept = default;
		CGS_INLINE constexpr Vector3& operator=(Vector3&& other) noexcept = default;

	public:
		CGS_INLINE constexpr Vector3 operator+(const Vector3& other) noexcept
		{
			return Vector3(X + other.X, Y + other.Y, Z + other.Z);
		}

		CGS_INLINE constexpr Vector3 operator-(const Vector3& other) noexcept
		{
			return Vector3(X - other.X, Y - other.Y, Z - other.Z);
		}

		CGS_INLINE constexpr ArithmeticType Dot(const Vector3& other) noexcept
		{
			return X * other.X + Y * other.Y + Z * other.Z;
		}

		CGS_INLINE constexpr ArithmeticType LengthSquared() noexcept
		{
			return Dot(*this);
		}

		CGS_INLINE constexpr ArithmeticType Length() noexcept
		{
			return std::sqrt(LengthSquared());
		}

		CGS_INLINE constexpr Vector3 Cross(const Vector3& other) noexcept
		{
			return Vector3(Y * other.Z - Z * other.Y, Z * other.X - X * other.Z, X * other.Y - Y * other.X);
		}

		CGS_CORE_VECTOR2_3_SWIZZLE_METHOD_DEFINITIONS(X, Y, Z)
		CGS_CORE_VECTOR2_3_SWIZZLE_METHOD_DEFINITIONS(R, G, B)
		CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITIONS(X, Y, Z)
		CGS_CORE_VECTOR3_SWIZZLE_METHOD_DEFINITIONS(R, G, B)

	public:
		union
		{
			ArithmeticType mData[3];
			struct
			{
				ArithmeticType X;
				ArithmeticType Y;
				ArithmeticType Z;
			};
			struct
			{
				ArithmeticType R;
				ArithmeticType G;
				ArithmeticType B;
			};
		};
	};

	export template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector3<ArithmeticType> operator*(const ArithmeticType s, const Vector3<ArithmeticType>& other) noexcept
	{
		return Vector3<ArithmeticType>(s * other.X, s * other.Y, s * other.Z);
	}

	export template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector3<ArithmeticType> operator*(const Vector3<ArithmeticType>& other, const ArithmeticType s) noexcept
	{
		return s * other;
	}

	export template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector3<ArithmeticType> operator/(const ArithmeticType s, const Vector3<ArithmeticType>& other) noexcept
	{
		return Vector3<ArithmeticType>(s / other.X, s / other.Y, s / other.Z);
	}

	export template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector3<ArithmeticType> operator/(const Vector3<ArithmeticType>& other, const ArithmeticType s) noexcept
	{
		return Vector3<ArithmeticType>(other.X / s, other.Y / s, other.Z / s);
	}

	export template<CArithmeticType ArithmeticType>
	struct CORE_API Vector4
	{
	public:
		CGS_INLINE explicit constexpr Vector4(const ArithmeticType x, const ArithmeticType y, const ArithmeticType z, const ArithmeticType w) noexcept
			: mData{ x, y, z, w }
		{
		}
		CGS_INLINE explicit constexpr Vector4(const Vector4& other) noexcept = default;
		CGS_INLINE explicit constexpr Vector4(Vector4&& other) noexcept = default;
		CGS_INLINE constexpr ~Vector4() noexcept = default;

		CGS_INLINE constexpr Vector4& operator=(const Vector4& other) noexcept = default;
		CGS_INLINE constexpr Vector4& operator=(Vector4&& other) noexcept = default;

	public:
		CGS_INLINE constexpr Vector4 operator+(const Vector4& other) noexcept
		{
			return Vector4(X + other.X, Y + other.Y, Z + other.Z, W + other.W);
		}

		CGS_INLINE constexpr Vector4 operator-(const Vector4& other) noexcept
		{
			return Vector4(X - other.X, Y - other.Y, Z - other.Z, W - other.W);
		}

		CGS_INLINE constexpr ArithmeticType Dot(const Vector4& other) noexcept
		{
			return X * other.X + Y * other.Y + Z * other.Z + W * other.W;
		}

		CGS_INLINE constexpr ArithmeticType LengthSquared() noexcept
		{
			return Dot(*this);
		}

		CGS_INLINE constexpr ArithmeticType Length() noexcept
		{
			return std::sqrt(LengthSquared());
		}

	public:
		union
		{
			ArithmeticType mData[4];
			struct
			{
				ArithmeticType X;
				ArithmeticType Y;
				ArithmeticType Z;
				ArithmeticType W;
			};
		};
	};

	export template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector4<ArithmeticType> operator*(const ArithmeticType s, const Vector4<ArithmeticType>& other) noexcept
	{
		return Vector4<ArithmeticType>(s * other.X, s * other.Y, s * other.Z, s * other.W);
	}

	export template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector4<ArithmeticType> operator*(const Vector4<ArithmeticType>& other, const ArithmeticType s) noexcept
	{
		return s * other;
	}

	export template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector4<ArithmeticType> operator/(const ArithmeticType s, const Vector4<ArithmeticType>& other) noexcept
	{
		return Vector4<ArithmeticType>(s / other.X, s / other.Y, s / other.Z, s / other.W);
	}

	export template<CArithmeticType ArithmeticType>
	CORE_API CGS_INLINE constexpr Vector4<ArithmeticType> operator/(const Vector4<ArithmeticType>& other, const ArithmeticType s) noexcept
	{
		return Vector4<ArithmeticType>(other.X / s, other.Y / s, other.Z / s, other.W / s);
	}

	template<CArithmeticType ArithmeticType>
	using RgbType = Vector3<ArithmeticType>;

	export using RgbF = RgbType<float>;
	export using RgbD = RgbType<double>;

	export using Rgb = RgbF;

	template<CArithmeticType ArithmeticType>
	using RgbaType = Vector4<ArithmeticType>;

	export using RgbaF = RgbaType<float>;
	export using RgbaD = RgbaType<double>;

	export using Rgba = RgbaF;
}
}