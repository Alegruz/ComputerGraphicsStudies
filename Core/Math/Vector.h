#pragma once

#include "Core/Concepts.h"

namespace cgs::core::math
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

	template<CArithmeticType ArithmeticType>
	struct CORE_API Vector2
	{
	public:
		explicit constexpr Vector2(const ArithmeticType x, const ArithmeticType y) noexcept;
		CGS_INLINE explicit constexpr Vector2(const Vector2& other) noexcept = default;
		CGS_INLINE explicit constexpr Vector2(Vector2&& other) noexcept = default;
		CGS_INLINE constexpr ~Vector2() noexcept = default;

		CGS_INLINE constexpr Vector2& operator=(const Vector2& other) noexcept = default;
		CGS_INLINE constexpr Vector2& operator=(Vector2&& other) noexcept = default;

	public:
		constexpr Vector2 operator+(const Vector2& other) noexcept;
		constexpr Vector2 operator-(const Vector2& other) noexcept;
		constexpr ArithmeticType Dot(const Vector2& other) noexcept;
		constexpr ArithmeticType LengthSquared() noexcept;
		constexpr ArithmeticType Length() noexcept;

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

	template<CArithmeticType ArithmeticType>
	CORE_API constexpr Vector2<ArithmeticType> operator*(const ArithmeticType s, const Vector2<ArithmeticType>& other) noexcept;

	template<CArithmeticType ArithmeticType>
	CORE_API constexpr Vector2<ArithmeticType> operator*(const Vector2<ArithmeticType>& other, const ArithmeticType s) noexcept;

	template<CArithmeticType ArithmeticType>
	CORE_API constexpr Vector2<ArithmeticType> operator/(const ArithmeticType s, const Vector2<ArithmeticType>& other) noexcept;

	template<CArithmeticType ArithmeticType>
	CORE_API constexpr Vector2<ArithmeticType> operator/(const Vector2<ArithmeticType>& other, const ArithmeticType s) noexcept;

	template<CArithmeticType ArithmeticType>
	struct CORE_API Vector3
	{
	public:
		explicit constexpr Vector3(const ArithmeticType x, const ArithmeticType y, const ArithmeticType z) noexcept;
		CGS_INLINE explicit constexpr Vector3(const Vector3& other) noexcept = default;
		CGS_INLINE explicit constexpr Vector3(Vector3&& other) noexcept = default;
		CGS_INLINE constexpr ~Vector3() noexcept = default;

		CGS_INLINE constexpr Vector3& operator=(const Vector3& other) noexcept = default;
		CGS_INLINE constexpr Vector3& operator=(Vector3&& other) noexcept = default;

	public:
		constexpr Vector3 operator+(const Vector3& other) noexcept;
		constexpr Vector3 operator-(const Vector3& other) noexcept;
		constexpr ArithmeticType Dot(const Vector3& other) noexcept;
		constexpr ArithmeticType LengthSquared() noexcept;
		constexpr ArithmeticType Length() noexcept;
		constexpr Vector3 Cross(const Vector3& other) noexcept;

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

	template<CArithmeticType ArithmeticType>
	CORE_API constexpr Vector3<ArithmeticType> operator*(const ArithmeticType s, const Vector3<ArithmeticType>& other) noexcept;

	template<CArithmeticType ArithmeticType>
	CORE_API constexpr Vector3<ArithmeticType> operator*(const Vector3<ArithmeticType>& other, const ArithmeticType s) noexcept;

	template<CArithmeticType ArithmeticType>
	CORE_API constexpr Vector3<ArithmeticType> operator/(const ArithmeticType s, const Vector3<ArithmeticType>& other) noexcept;

	template<CArithmeticType ArithmeticType>
	CORE_API constexpr Vector3<ArithmeticType> operator/(const Vector3<ArithmeticType>& other, const ArithmeticType s) noexcept;

	template<CArithmeticType ArithmeticType>
	struct CORE_API Vector4
	{
	public:
		explicit constexpr Vector4(const ArithmeticType x, const ArithmeticType y, const ArithmeticType z, const ArithmeticType w) noexcept;
		CGS_INLINE explicit constexpr Vector4(const Vector4& other) noexcept = default;
		CGS_INLINE explicit constexpr Vector4(Vector4&& other) noexcept = default;
		CGS_INLINE constexpr ~Vector4() noexcept = default;

		CGS_INLINE constexpr Vector4& operator=(const Vector4& other) noexcept = default;
		CGS_INLINE constexpr Vector4& operator=(Vector4&& other) noexcept = default;

	public:
		CGS_INLINE constexpr Vector4 operator+(const Vector4& other) noexcept;
		CGS_INLINE constexpr Vector4 operator-(const Vector4& other) noexcept;
		CGS_INLINE constexpr ArithmeticType Dot(const Vector4& other) noexcept;
		CGS_INLINE constexpr ArithmeticType LengthSquared() noexcept;
		CGS_INLINE constexpr ArithmeticType Length() noexcept;

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

	template<CArithmeticType ArithmeticType>
	CORE_API constexpr Vector4<ArithmeticType> operator*(const ArithmeticType s, const Vector4<ArithmeticType>& other) noexcept;

	template<CArithmeticType ArithmeticType>
	CORE_API constexpr Vector4<ArithmeticType> operator*(const Vector4<ArithmeticType>& other, const ArithmeticType s) noexcept;

	template<CArithmeticType ArithmeticType>
	CORE_API constexpr Vector4<ArithmeticType> operator/(const ArithmeticType s, const Vector4<ArithmeticType>& other) noexcept;

	template<CArithmeticType ArithmeticType>
	CORE_API constexpr Vector4<ArithmeticType> operator/(const Vector4<ArithmeticType>& other, const ArithmeticType s) noexcept;

	template<CArithmeticType ArithmeticType>
	using RgbType = Vector3<ArithmeticType>;

	using RgbF = RgbType<float>;
	using RgbD = RgbType<double>;

	using Rgb = RgbF;

	template<CArithmeticType ArithmeticType>
	using RgbaType = Vector4<ArithmeticType>;

	using RgbaF = RgbaType<float>;
	using RgbaD = RgbaType<double>;

	using Rgba = RgbaF;
}