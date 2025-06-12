#pragma once

#include "Core/Concepts.h"

namespace cgs::core::math
{
	template<CArithmeticType ArithmeticType = float, uint16_t ROW_SIZE = 4, uint16_t COLUMN_SIZE = 4, eMatrixMajorType MATRIX_MAJOR_TYPE = eMatrixMajorType::COLUMN>
	class CORE_API Matrix
	{
		static_assert(ROW_SIZE > 0);
		static_assert(COLUMN_SIZE > 0);

	public:
		friend constexpr Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& rhs) noexcept;
		friend constexpr Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& rhs) noexcept;
		friend constexpr Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	public:
		explicit constexpr Matrix() noexcept;
		CGS_INLINE explicit constexpr Matrix(const Matrix& other) noexcept = default;
		CGS_INLINE explicit constexpr Matrix(Matrix&& other) noexcept = default;
		CGS_INLINE constexpr ~Matrix() noexcept = default;

		CGS_INLINE constexpr Matrix& operator=(const Matrix& other) noexcept = default;
		CGS_INLINE constexpr Matrix& operator=(Matrix&& other) noexcept = default;

	public:	// Element Access
		constexpr const ArithmeticType& At(const uint16_t rowIndex, const uint16_t columnIndex) const noexcept;
        constexpr ArithmeticType& At(const uint16_t rowIndex, const uint16_t columnIndex) noexcept;

	public:	// Operations

	private:
		ArithmeticType mData[ROW_SIZE * COLUMN_SIZE];
	};

	template<CArithmeticType ArithmeticType, uint16_t ROW_SIZE, uint16_t COLUMN_SIZE, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, uint16_t ROW_SIZE, uint16_t COLUMN_SIZE, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept;
	
	template<CArithmeticType ArithmeticType, uint16_t ROW_SIZE, uint16_t COLUMN_SIZE, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CORE_API constexpr Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept;
}