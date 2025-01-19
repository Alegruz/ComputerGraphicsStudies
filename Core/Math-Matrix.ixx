module;

#include <cassert>
#include <cmath>

#include "Core/PchCommon.h"
#include "Core/Concepts.h"

export module Core.Math.Matrix;

namespace cgs
{
namespace core
{
	export template<CArithmeticType ArithmeticType = float, uint16_t ROW_SIZE = 4, uint16_t COLUMN_SIZE = 4, eMatrixMajorType MATRIX_MAJOR_TYPE = eMatrixMajorType::COLUMN>
	class Matrix
	{
		static_assert(ROW_SIZE > 0);
		static_assert(COLUMN_SIZE > 0);

	public:
		friend constexpr Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& rhs) noexcept;
		friend constexpr Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& rhs) noexcept;
		friend constexpr Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& rhs) noexcept;

	public:
		CGS_INLINE explicit constexpr Matrix() noexcept
			: mData{ 0, }
		{
		}
		CGS_INLINE explicit constexpr Matrix(const Matrix& other) noexcept = default;
		CGS_INLINE explicit constexpr Matrix(Matrix&& other) noexcept = default;
		CGS_INLINE constexpr ~Matrix() noexcept = default;

		CGS_INLINE constexpr Matrix& operator=(const Matrix& other) noexcept = default;
		CGS_INLINE constexpr Matrix& operator=(Matrix&& other) noexcept = default;

	public:	// Element Access
		CGS_INLINE constexpr const ArithmeticType& At(const uint16_t rowIndex, const uint16_t columnIndex) const noexcept
		{
			assert(rowIndex < ROW_SIZE);
			assert(columnIndex < COLUMN_SIZE);
			if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
			{
				return mData[ROW_SIZE * columnIndex + rowIndex];
			}
			else
			{
				return mData[COLUMN_SIZE * rowIndex + columnIndex];
			}
		}

		CGS_INLINE constexpr ArithmeticType& At(const uint16_t rowIndex, const uint16_t columnIndex) noexcept
		{
			assert(rowIndex < ROW_SIZE);
			assert(columnIndex < COLUMN_SIZE);
			if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
			{
				return mData[ROW_SIZE * columnIndex + rowIndex];
			}
			else
			{
				return mData[COLUMN_SIZE * rowIndex + columnIndex];
			}
		}

	public:	// Operations

	private:
		ArithmeticType mData[ROW_SIZE * COLUMN_SIZE];
	};

	export template<CArithmeticType ArithmeticType, uint16_t ROW_SIZE, uint16_t COLUMN_SIZE, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			for (uint16_t row = 0; row < ROW_SIZE; ++row)
			{
				for (uint16_t column = 0; column < COLUMN_SIZE; ++column)
				{
					sum.mData[row][column] = lhs.At(row, column) + rhs.At(row, column);
				}
			}
		}
		else
		{
			for (uint16_t column = 0; column < COLUMN_SIZE; ++column)
			{
				for (uint16_t row = 0; row < ROW_SIZE; ++row)
				{
					sum.mData[column][row] = lhs.At(row, column) + rhs.At(row, column);
				}
			}
		}
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
		sum.mData[1] = lhs.At(0, 1) + rhs.At(0, 1);

		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
		sum.mData[1] = lhs.At(0, 1) + rhs.At(0, 1);
		sum.mData[2] = lhs.At(0, 2) + rhs.At(0, 2);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
		sum.mData[1] = lhs.At(0, 1) + rhs.At(0, 1);
		sum.mData[2] = lhs.At(0, 2) + rhs.At(0, 2);
		sum.mData[3] = lhs.At(0, 3) + rhs.At(0, 3);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
		sum.mData[1] = lhs.At(1, 0) + rhs.At(1, 0);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[1] = lhs.At(1, 0) + rhs.At(1, 0);

			sum.mData[2] = lhs.At(0, 1) + rhs.At(0, 1);
			sum.mData[3] = lhs.At(1, 1) + rhs.At(1, 1);
		}
		else
		{
			sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[1] = lhs.At(0, 1) + rhs.At(0, 1);

			sum.mData[2] = lhs.At(1, 0) + rhs.At(1, 0);
			sum.mData[3] = lhs.At(1, 1) + rhs.At(1, 1);
		}

		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[1] = lhs.At(1, 0) + rhs.At(1, 0);

			sum.mData[2] = lhs.At(0, 1) + rhs.At(0, 1);
			sum.mData[3] = lhs.At(1, 1) + rhs.At(1, 1);

			sum.mData[4] = lhs.At(0, 2) + rhs.At(0, 2);
			sum.mData[5] = lhs.At(1, 2) + rhs.At(1, 2);
		}
		else
		{
			sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[1] = lhs.At(0, 1) + rhs.At(0, 1);
			sum.mData[2] = lhs.At(0, 2) + rhs.At(0, 2);

			sum.mData[3] = lhs.At(1, 0) + rhs.At(1, 0);
			sum.mData[4] = lhs.At(1, 1) + rhs.At(1, 1);
			sum.mData[5] = lhs.At(1, 2) + rhs.At(1, 2);
		}
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[1] = lhs.At(1, 0) + rhs.At(1, 0);

			sum.mData[2] = lhs.At(0, 1) + rhs.At(0, 1);
			sum.mData[3] = lhs.At(1, 1) + rhs.At(1, 1);

			sum.mData[4] = lhs.At(0, 2) + rhs.At(0, 2);
			sum.mData[5] = lhs.At(1, 2) + rhs.At(1, 2);

			sum.mData[6] = lhs.At(0, 3) + rhs.At(0, 3);
			sum.mData[7] = lhs.At(1, 3) + rhs.At(1, 3);
		}
		else
		{
			sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[1] = lhs.At(0, 1) + rhs.At(0, 1);
			sum.mData[2] = lhs.At(0, 2) + rhs.At(0, 2);
			sum.mData[3] = lhs.At(0, 3) + rhs.At(0, 3);

			sum.mData[4] = lhs.At(1, 0) + rhs.At(1, 0);
			sum.mData[5] = lhs.At(1, 1) + rhs.At(1, 1);
			sum.mData[6] = lhs.At(1, 2) + rhs.At(1, 2);
			sum.mData[7] = lhs.At(1, 3) + rhs.At(1, 3);
		}
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
		sum.mData[1] = lhs.At(1, 0) + rhs.At(1, 0);
		sum.mData[2] = lhs.At(2, 0) + rhs.At(2, 0);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[1] = lhs.At(1, 0) + rhs.At(1, 0);
			sum.mData[2] = lhs.At(2, 0) + rhs.At(2, 0);

			sum.mData[3] = lhs.At(0, 1) + rhs.At(0, 1);
			sum.mData[4] = lhs.At(1, 1) + rhs.At(1, 1);
			sum.mData[5] = lhs.At(2, 1) + rhs.At(2, 1);
		}
		else
		{
			sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[1] = lhs.At(0, 1) + rhs.At(0, 1);

			sum.mData[2] = lhs.At(1, 0) + rhs.At(1, 0);
			sum.mData[3] = lhs.At(1, 1) + rhs.At(1, 1);

			sum.mData[4] = lhs.At(2, 0) + rhs.At(2, 0);
			sum.mData[5] = lhs.At(2, 1) + rhs.At(2, 1);
		}

		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[1] = lhs.At(1, 0) + rhs.At(1, 0);
			sum.mData[2] = lhs.At(2, 0) + rhs.At(2, 0);

			sum.mData[3] = lhs.At(0, 1) + rhs.At(0, 1);
			sum.mData[4] = lhs.At(1, 1) + rhs.At(1, 1);
			sum.mData[5] = lhs.At(2, 1) + rhs.At(2, 1);

			sum.mData[6] = lhs.At(0, 2) + rhs.At(0, 2);
			sum.mData[7] = lhs.At(1, 2) + rhs.At(1, 2);
			sum.mData[8] = lhs.At(2, 2) + rhs.At(2, 2);
		}
		else
		{
			sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[1] = lhs.At(0, 1) + rhs.At(0, 1);
			sum.mData[2] = lhs.At(0, 2) + rhs.At(0, 2);

			sum.mData[3] = lhs.At(1, 0) + rhs.At(1, 0);
			sum.mData[4] = lhs.At(1, 1) + rhs.At(1, 1);
			sum.mData[5] = lhs.At(1, 2) + rhs.At(1, 2);

			sum.mData[6] = lhs.At(2, 0) + rhs.At(2, 0);
			sum.mData[7] = lhs.At(2, 1) + rhs.At(2, 1);
			sum.mData[8] = lhs.At(2, 2) + rhs.At(2, 2);
		}
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[ 0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[ 1] = lhs.At(1, 0) + rhs.At(1, 0);
			sum.mData[ 2] = lhs.At(2, 0) + rhs.At(2, 0);

			sum.mData[ 3] = lhs.At(0, 1) + rhs.At(0, 1);
			sum.mData[ 4] = lhs.At(1, 1) + rhs.At(1, 1);
			sum.mData[ 5] = lhs.At(2, 1) + rhs.At(2, 1);

			sum.mData[ 6] = lhs.At(0, 2) + rhs.At(0, 2);
			sum.mData[ 7] = lhs.At(1, 2) + rhs.At(1, 2);
			sum.mData[ 8] = lhs.At(2, 2) + rhs.At(2, 2);

			sum.mData[ 9] = lhs.At(0, 3) + rhs.At(0, 3);
			sum.mData[10] = lhs.At(1, 3) + rhs.At(1, 3);
			sum.mData[11] = lhs.At(2, 3) + rhs.At(2, 3);
		}
		else
		{
			sum.mData[ 0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[ 1] = lhs.At(0, 1) + rhs.At(1, 1);
			sum.mData[ 2] = lhs.At(0, 2) + rhs.At(2, 2);
			sum.mData[ 3] = lhs.At(0, 3) + rhs.At(2, 3);

			sum.mData[ 4] = lhs.At(1, 0) + rhs.At(0, 0);
			sum.mData[ 5] = lhs.At(1, 1) + rhs.At(1, 1);
			sum.mData[ 6] = lhs.At(1, 2) + rhs.At(2, 2);
			sum.mData[ 7] = lhs.At(1, 3) + rhs.At(2, 3);

			sum.mData[ 8] = lhs.At(2, 0) + rhs.At(0, 0);
			sum.mData[ 9] = lhs.At(2, 1) + rhs.At(1, 1);
			sum.mData[10] = lhs.At(2, 2) + rhs.At(2, 2);
			sum.mData[11] = lhs.At(2, 3) + rhs.At(2, 3);
		}
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
		sum.mData[1] = lhs.At(1, 0) + rhs.At(1, 0);
		sum.mData[2] = lhs.At(2, 0) + rhs.At(2, 0);
		sum.mData[3] = lhs.At(3, 0) + rhs.At(3, 0);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[1] = lhs.At(1, 0) + rhs.At(1, 0);
			sum.mData[2] = lhs.At(2, 0) + rhs.At(2, 0);
			sum.mData[3] = lhs.At(3, 0) + rhs.At(3, 0);

			sum.mData[4] = lhs.At(0, 1) + rhs.At(0, 1);
			sum.mData[5] = lhs.At(1, 1) + rhs.At(1, 1);
			sum.mData[6] = lhs.At(2, 1) + rhs.At(2, 1);
			sum.mData[7] = lhs.At(3, 1) + rhs.At(3, 1);
		}
		else
		{
			sum.mData[0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[1] = lhs.At(0, 1) + rhs.At(0, 1);

			sum.mData[2] = lhs.At(1, 0) + rhs.At(1, 0);
			sum.mData[3] = lhs.At(1, 1) + rhs.At(1, 1);

			sum.mData[4] = lhs.At(2, 0) + rhs.At(2, 0);
			sum.mData[5] = lhs.At(2, 1) + rhs.At(2, 1);

			sum.mData[5] = lhs.At(3, 0) + rhs.At(3, 0);
			sum.mData[6] = lhs.At(3, 1) + rhs.At(3, 1);
		}

		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[ 0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[ 1] = lhs.At(1, 0) + rhs.At(1, 0);
			sum.mData[ 2] = lhs.At(2, 0) + rhs.At(2, 0);
			sum.mData[ 3] = lhs.At(3, 0) + rhs.At(3, 0);

			sum.mData[ 4] = lhs.At(0, 1) + rhs.At(0, 1);
			sum.mData[ 5] = lhs.At(1, 1) + rhs.At(1, 1);
			sum.mData[ 6] = lhs.At(2, 1) + rhs.At(2, 1);
			sum.mData[ 7] = lhs.At(3, 1) + rhs.At(3, 1);

			sum.mData[ 8] = lhs.At(0, 2) + rhs.At(0, 2);
			sum.mData[ 9] = lhs.At(1, 2) + rhs.At(1, 2);
			sum.mData[10] = lhs.At(2, 2) + rhs.At(2, 2);
			sum.mData[11] = lhs.At(3, 2) + rhs.At(3, 2);
		}
		else
		{
			sum.mData[ 0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[ 1] = lhs.At(0, 1) + rhs.At(0, 1);
			sum.mData[ 2] = lhs.At(0, 2) + rhs.At(0, 2);

			sum.mData[ 3] = lhs.At(1, 0) + rhs.At(1, 0);
			sum.mData[ 4] = lhs.At(1, 1) + rhs.At(1, 1);
			sum.mData[ 5] = lhs.At(1, 2) + rhs.At(1, 2);

			sum.mData[ 6] = lhs.At(2, 0) + rhs.At(2, 0);
			sum.mData[ 7] = lhs.At(2, 1) + rhs.At(2, 1);
			sum.mData[ 8] = lhs.At(2, 2) + rhs.At(2, 2);

			sum.mData[ 9] = lhs.At(3, 0) + rhs.At(3, 0);
			sum.mData[10] = lhs.At(3, 1) + rhs.At(3, 1);
			sum.mData[11] = lhs.At(3, 2) + rhs.At(3, 2);
		}
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE> operator+(const Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[ 0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[ 1] = lhs.At(1, 0) + rhs.At(1, 0);
			sum.mData[ 2] = lhs.At(2, 0) + rhs.At(2, 0);
			sum.mData[ 3] = lhs.At(3, 0) + rhs.At(3, 0);

			sum.mData[ 4] = lhs.At(0, 1) + rhs.At(0, 1);
			sum.mData[ 5] = lhs.At(1, 1) + rhs.At(1, 1);
			sum.mData[ 6] = lhs.At(2, 1) + rhs.At(2, 1);
			sum.mData[ 7] = lhs.At(3, 1) + rhs.At(3, 1);

			sum.mData[ 8] = lhs.At(0, 2) + rhs.At(0, 2);
			sum.mData[ 9] = lhs.At(1, 2) + rhs.At(1, 2);
			sum.mData[10] = lhs.At(2, 2) + rhs.At(2, 2);
			sum.mData[11] = lhs.At(3, 2) + rhs.At(3, 2);

			sum.mData[12] = lhs.At(0, 3) + rhs.At(0, 3);
			sum.mData[13] = lhs.At(1, 3) + rhs.At(1, 3);
			sum.mData[14] = lhs.At(2, 3) + rhs.At(2, 3);
			sum.mData[15] = lhs.At(3, 3) + rhs.At(3, 3);
		}
		else
		{
			sum.mData[ 0] = lhs.At(0, 0) + rhs.At(0, 0);
			sum.mData[ 1] = lhs.At(0, 1) + rhs.At(0, 1);
			sum.mData[ 2] = lhs.At(0, 2) + rhs.At(0, 2);
			sum.mData[ 3] = lhs.At(0, 3) + rhs.At(0, 3);

			sum.mData[ 4] = lhs.At(1, 0) + rhs.At(0, 0);
			sum.mData[ 5] = lhs.At(1, 1) + rhs.At(1, 1);
			sum.mData[ 6] = lhs.At(1, 2) + rhs.At(1, 2);
			sum.mData[ 7] = lhs.At(1, 3) + rhs.At(1, 3);

			sum.mData[ 8] = lhs.At(2, 0) + rhs.At(0, 0);
			sum.mData[ 9] = lhs.At(2, 1) + rhs.At(2, 1);
			sum.mData[10] = lhs.At(2, 2) + rhs.At(2, 2);
			sum.mData[11] = lhs.At(2, 3) + rhs.At(2, 3);

			sum.mData[12] = lhs.At(3, 0) + rhs.At(3, 0);
			sum.mData[13] = lhs.At(3, 1) + rhs.At(3, 1);
			sum.mData[14] = lhs.At(3, 2) + rhs.At(3, 2);
			sum.mData[15] = lhs.At(3, 3) + rhs.At(3, 3);
		}
		return sum;
	}

	export template<CArithmeticType ArithmeticType, uint16_t ROW_SIZE, uint16_t COLUMN_SIZE, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE> difference;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			for (uint16_t row = 0; row < ROW_SIZE; ++row)
			{
				for (uint16_t column = 0; column < COLUMN_SIZE; ++column)
				{
					difference.mData[row][column] = lhs.At(row, column) - rhs.At(row, column);
				}
			}
		}
		else
		{
			for (uint16_t column = 0; column < COLUMN_SIZE; ++column)
			{
				for (uint16_t row = 0; row < ROW_SIZE; ++row)
				{
					difference.mData[column][row] = lhs.At(row, column) - rhs.At(row, column);
				}
			}
		}
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
		sum.mData[1] = lhs.At(0, 1) - rhs.At(0, 1);

		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
		sum.mData[1] = lhs.At(0, 1) - rhs.At(0, 1);
		sum.mData[2] = lhs.At(0, 2) - rhs.At(0, 2);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
		sum.mData[1] = lhs.At(0, 1) - rhs.At(0, 1);
		sum.mData[2] = lhs.At(0, 2) - rhs.At(0, 2);
		sum.mData[3] = lhs.At(0, 3) - rhs.At(0, 3);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
		sum.mData[1] = lhs.At(1, 0) - rhs.At(1, 0);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(1, 0) - rhs.At(1, 0);

			sum.mData[2] = lhs.At(0, 1) - rhs.At(0, 1);
			sum.mData[3] = lhs.At(1, 1) - rhs.At(1, 1);
		}
		else
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(0, 1) - rhs.At(0, 1);

			sum.mData[2] = lhs.At(1, 0) - rhs.At(1, 0);
			sum.mData[3] = lhs.At(1, 1) - rhs.At(1, 1);
		}

		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(1, 0) - rhs.At(1, 0);

			sum.mData[2] = lhs.At(0, 1) - rhs.At(0, 1);
			sum.mData[3] = lhs.At(1, 1) - rhs.At(1, 1);

			sum.mData[4] = lhs.At(0, 2) - rhs.At(0, 2);
			sum.mData[5] = lhs.At(1, 2) - rhs.At(1, 2);
		}
		else
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(0, 1) - rhs.At(0, 1);
			sum.mData[2] = lhs.At(0, 2) - rhs.At(0, 2);

			sum.mData[3] = lhs.At(1, 0) - rhs.At(1, 0);
			sum.mData[4] = lhs.At(1, 1) - rhs.At(1, 1);
			sum.mData[5] = lhs.At(1, 2) - rhs.At(1, 2);
		}
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(1, 0) - rhs.At(1, 0);

			sum.mData[2] = lhs.At(0, 1) - rhs.At(0, 1);
			sum.mData[3] = lhs.At(1, 1) - rhs.At(1, 1);

			sum.mData[4] = lhs.At(0, 2) - rhs.At(0, 2);
			sum.mData[5] = lhs.At(1, 2) - rhs.At(1, 2);

			sum.mData[6] = lhs.At(0, 3) - rhs.At(0, 3);
			sum.mData[7] = lhs.At(1, 3) - rhs.At(1, 3);
		}
		else
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(0, 1) - rhs.At(0, 1);
			sum.mData[2] = lhs.At(0, 2) - rhs.At(0, 2);
			sum.mData[3] = lhs.At(0, 3) - rhs.At(0, 3);

			sum.mData[4] = lhs.At(1, 0) - rhs.At(1, 0);
			sum.mData[5] = lhs.At(1, 1) - rhs.At(1, 1);
			sum.mData[6] = lhs.At(1, 2) - rhs.At(1, 2);
			sum.mData[7] = lhs.At(1, 3) - rhs.At(1, 3);
		}
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
		sum.mData[1] = lhs.At(1, 0) - rhs.At(1, 0);
		sum.mData[2] = lhs.At(2, 0) - rhs.At(2, 0);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(1, 0) - rhs.At(1, 0);
			sum.mData[2] = lhs.At(2, 0) - rhs.At(2, 0);

			sum.mData[3] = lhs.At(0, 1) - rhs.At(0, 1);
			sum.mData[4] = lhs.At(1, 1) - rhs.At(1, 1);
			sum.mData[5] = lhs.At(2, 1) - rhs.At(2, 1);
		}
		else
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(0, 1) - rhs.At(0, 1);

			sum.mData[2] = lhs.At(1, 0) - rhs.At(1, 0);
			sum.mData[3] = lhs.At(1, 1) - rhs.At(1, 1);

			sum.mData[4] = lhs.At(2, 0) - rhs.At(2, 0);
			sum.mData[5] = lhs.At(2, 1) - rhs.At(2, 1);
		}

		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(1, 0) - rhs.At(1, 0);
			sum.mData[2] = lhs.At(2, 0) - rhs.At(2, 0);

			sum.mData[3] = lhs.At(0, 1) - rhs.At(0, 1);
			sum.mData[4] = lhs.At(1, 1) - rhs.At(1, 1);
			sum.mData[5] = lhs.At(2, 1) - rhs.At(2, 1);

			sum.mData[6] = lhs.At(0, 2) - rhs.At(0, 2);
			sum.mData[7] = lhs.At(1, 2) - rhs.At(1, 2);
			sum.mData[8] = lhs.At(2, 2) - rhs.At(2, 2);
		}
		else
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(0, 1) - rhs.At(0, 1);
			sum.mData[2] = lhs.At(0, 2) - rhs.At(0, 2);

			sum.mData[3] = lhs.At(1, 0) - rhs.At(1, 0);
			sum.mData[4] = lhs.At(1, 1) - rhs.At(1, 1);
			sum.mData[5] = lhs.At(1, 2) - rhs.At(1, 2);

			sum.mData[6] = lhs.At(2, 0) - rhs.At(2, 0);
			sum.mData[7] = lhs.At(2, 1) - rhs.At(2, 1);
			sum.mData[8] = lhs.At(2, 2) - rhs.At(2, 2);
		}
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(1, 0) - rhs.At(1, 0);
			sum.mData[2] = lhs.At(2, 0) - rhs.At(2, 0);

			sum.mData[3] = lhs.At(0, 1) - rhs.At(0, 1);
			sum.mData[4] = lhs.At(1, 1) - rhs.At(1, 1);
			sum.mData[5] = lhs.At(2, 1) - rhs.At(2, 1);

			sum.mData[6] = lhs.At(0, 2) - rhs.At(0, 2);
			sum.mData[7] = lhs.At(1, 2) - rhs.At(1, 2);
			sum.mData[8] = lhs.At(2, 2) - rhs.At(2, 2);

			sum.mData[9] = lhs.At(0, 3) - rhs.At(0, 3);
			sum.mData[10] = lhs.At(1, 3) - rhs.At(1, 3);
			sum.mData[11] = lhs.At(2, 3) - rhs.At(2, 3);
		}
		else
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(0, 1) - rhs.At(1, 1);
			sum.mData[2] = lhs.At(0, 2) - rhs.At(2, 2);
			sum.mData[3] = lhs.At(0, 3) - rhs.At(2, 3);

			sum.mData[4] = lhs.At(1, 0) - rhs.At(0, 0);
			sum.mData[5] = lhs.At(1, 1) - rhs.At(1, 1);
			sum.mData[6] = lhs.At(1, 2) - rhs.At(2, 2);
			sum.mData[7] = lhs.At(1, 3) - rhs.At(2, 3);

			sum.mData[8] = lhs.At(2, 0) - rhs.At(0, 0);
			sum.mData[9] = lhs.At(2, 1) - rhs.At(1, 1);
			sum.mData[10] = lhs.At(2, 2) - rhs.At(2, 2);
			sum.mData[11] = lhs.At(2, 3) - rhs.At(2, 3);
		}
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
		sum.mData[1] = lhs.At(1, 0) - rhs.At(1, 0);
		sum.mData[2] = lhs.At(2, 0) - rhs.At(2, 0);
		sum.mData[3] = lhs.At(3, 0) - rhs.At(3, 0);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(1, 0) - rhs.At(1, 0);
			sum.mData[2] = lhs.At(2, 0) - rhs.At(2, 0);
			sum.mData[3] = lhs.At(3, 0) - rhs.At(3, 0);

			sum.mData[4] = lhs.At(0, 1) - rhs.At(0, 1);
			sum.mData[5] = lhs.At(1, 1) - rhs.At(1, 1);
			sum.mData[6] = lhs.At(2, 1) - rhs.At(2, 1);
			sum.mData[7] = lhs.At(3, 1) - rhs.At(3, 1);
		}
		else
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(0, 1) - rhs.At(0, 1);

			sum.mData[2] = lhs.At(1, 0) - rhs.At(1, 0);
			sum.mData[3] = lhs.At(1, 1) - rhs.At(1, 1);

			sum.mData[4] = lhs.At(2, 0) - rhs.At(2, 0);
			sum.mData[5] = lhs.At(2, 1) - rhs.At(2, 1);

			sum.mData[5] = lhs.At(3, 0) - rhs.At(3, 0);
			sum.mData[6] = lhs.At(3, 1) - rhs.At(3, 1);
		}

		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(1, 0) - rhs.At(1, 0);
			sum.mData[2] = lhs.At(2, 0) - rhs.At(2, 0);
			sum.mData[3] = lhs.At(3, 0) - rhs.At(3, 0);

			sum.mData[4] = lhs.At(0, 1) - rhs.At(0, 1);
			sum.mData[5] = lhs.At(1, 1) - rhs.At(1, 1);
			sum.mData[6] = lhs.At(2, 1) - rhs.At(2, 1);
			sum.mData[7] = lhs.At(3, 1) - rhs.At(3, 1);

			sum.mData[8] = lhs.At(0, 2) - rhs.At(0, 2);
			sum.mData[9] = lhs.At(1, 2) - rhs.At(1, 2);
			sum.mData[10] = lhs.At(2, 2) - rhs.At(2, 2);
			sum.mData[11] = lhs.At(3, 2) - rhs.At(3, 2);
		}
		else
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(0, 1) - rhs.At(0, 1);
			sum.mData[2] = lhs.At(0, 2) - rhs.At(0, 2);

			sum.mData[3] = lhs.At(1, 0) - rhs.At(1, 0);
			sum.mData[4] = lhs.At(1, 1) - rhs.At(1, 1);
			sum.mData[5] = lhs.At(1, 2) - rhs.At(1, 2);

			sum.mData[6] = lhs.At(2, 0) - rhs.At(2, 0);
			sum.mData[7] = lhs.At(2, 1) - rhs.At(2, 1);
			sum.mData[8] = lhs.At(2, 2) - rhs.At(2, 2);

			sum.mData[9] = lhs.At(3, 0) - rhs.At(3, 0);
			sum.mData[10] = lhs.At(3, 1) - rhs.At(3, 1);
			sum.mData[11] = lhs.At(3, 2) - rhs.At(3, 2);
		}
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE> operator-(const Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE>& lhs, const Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(1, 0) - rhs.At(1, 0);
			sum.mData[2] = lhs.At(2, 0) - rhs.At(2, 0);
			sum.mData[3] = lhs.At(3, 0) - rhs.At(3, 0);

			sum.mData[4] = lhs.At(0, 1) - rhs.At(0, 1);
			sum.mData[5] = lhs.At(1, 1) - rhs.At(1, 1);
			sum.mData[6] = lhs.At(2, 1) - rhs.At(2, 1);
			sum.mData[7] = lhs.At(3, 1) - rhs.At(3, 1);

			sum.mData[8] = lhs.At(0, 2) - rhs.At(0, 2);
			sum.mData[9] = lhs.At(1, 2) - rhs.At(1, 2);
			sum.mData[10] = lhs.At(2, 2) - rhs.At(2, 2);
			sum.mData[11] = lhs.At(3, 2) - rhs.At(3, 2);

			sum.mData[12] = lhs.At(0, 3) - rhs.At(0, 3);
			sum.mData[13] = lhs.At(1, 3) - rhs.At(1, 3);
			sum.mData[14] = lhs.At(2, 3) - rhs.At(2, 3);
			sum.mData[15] = lhs.At(3, 3) - rhs.At(3, 3);
		}
		else
		{
			sum.mData[0] = lhs.At(0, 0) - rhs.At(0, 0);
			sum.mData[1] = lhs.At(0, 1) - rhs.At(0, 1);
			sum.mData[2] = lhs.At(0, 2) - rhs.At(0, 2);
			sum.mData[3] = lhs.At(0, 3) - rhs.At(0, 3);

			sum.mData[4] = lhs.At(1, 0) - rhs.At(0, 0);
			sum.mData[5] = lhs.At(1, 1) - rhs.At(1, 1);
			sum.mData[6] = lhs.At(1, 2) - rhs.At(1, 2);
			sum.mData[7] = lhs.At(1, 3) - rhs.At(1, 3);

			sum.mData[8] = lhs.At(2, 0) - rhs.At(0, 0);
			sum.mData[9] = lhs.At(2, 1) - rhs.At(2, 1);
			sum.mData[10] = lhs.At(2, 2) - rhs.At(2, 2);
			sum.mData[11] = lhs.At(2, 3) - rhs.At(2, 3);

			sum.mData[12] = lhs.At(3, 0) - rhs.At(3, 0);
			sum.mData[13] = lhs.At(3, 1) - rhs.At(3, 1);
			sum.mData[14] = lhs.At(3, 2) - rhs.At(3, 2);
			sum.mData[15] = lhs.At(3, 3) - rhs.At(3, 3);
		}
		return sum;
	}
	
	export template<CArithmeticType ArithmeticType, uint16_t ROW_SIZE, uint16_t COLUMN_SIZE, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, ROW_SIZE, COLUMN_SIZE, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			for (uint16_t row = 0; row < ROW_SIZE; ++row)
			{
				for (uint16_t column = 0; column < COLUMN_SIZE; ++column)
				{
					sum.mData[row][column] = s * rhs.At(row, column);
				}
			}
		}
		else
		{
			for (uint16_t column = 0; column < COLUMN_SIZE; ++column)
			{
				for (uint16_t row = 0; row < ROW_SIZE; ++row)
				{
					sum.mData[column][row] = s * rhs.At(row, column);
				}
			}
		}
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 1, 1, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = s * rhs.At(0, 0);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 1, 2, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = s * rhs.At(0, 0);
		sum.mData[1] = s * rhs.At(0, 1);

		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 1, 3, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = s * rhs.At(0, 0);
		sum.mData[1] = s * rhs.At(0, 1);
		sum.mData[2] = s * rhs.At(0, 2);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 1, 4, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = s * rhs.At(0, 0);
		sum.mData[1] = s * rhs.At(0, 1);
		sum.mData[2] = s * rhs.At(0, 2);
		sum.mData[3] = s * rhs.At(0, 3);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 2, 1, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = s * rhs.At(0, 0);
		sum.mData[1] = s * rhs.At(1, 0);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 2, 2, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = s * rhs.At(0, 0);
			sum.mData[1] = s * rhs.At(1, 0);

			sum.mData[2] = s * rhs.At(0, 1);
			sum.mData[3] = s * rhs.At(1, 1);
		}
		else
		{
			sum.mData[0] = s * rhs.At(0, 0);
			sum.mData[1] = s * rhs.At(0, 1);

			sum.mData[2] = s * rhs.At(1, 0);
			sum.mData[3] = s * rhs.At(1, 1);
		}

		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 2, 3, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = s * rhs.At(0, 0);
			sum.mData[1] = s * rhs.At(1, 0);

			sum.mData[2] = s * rhs.At(0, 1);
			sum.mData[3] = s * rhs.At(1, 1);

			sum.mData[4] = s * rhs.At(0, 2);
			sum.mData[5] = s * rhs.At(1, 2);
		}
		else
		{
			sum.mData[0] = s * rhs.At(0, 0);
			sum.mData[1] = s * rhs.At(0, 1);
			sum.mData[2] = s * rhs.At(0, 2);

			sum.mData[3] = s * rhs.At(1, 0);
			sum.mData[4] = s * rhs.At(1, 1);
			sum.mData[5] = s * rhs.At(1, 2);
		}
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 2, 4, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = s * rhs.At(0, 0);
			sum.mData[1] = s * rhs.At(1, 0);

			sum.mData[2] = s * rhs.At(0, 1);
			sum.mData[3] = s * rhs.At(1, 1);

			sum.mData[4] = s * rhs.At(0, 2);
			sum.mData[5] = s * rhs.At(1, 2);

			sum.mData[6] = s * rhs.At(0, 3);
			sum.mData[7] = s * rhs.At(1, 3);
		}
		else
		{
			sum.mData[0] = s * rhs.At(0, 0);
			sum.mData[1] = s * rhs.At(0, 1);
			sum.mData[2] = s * rhs.At(0, 2);
			sum.mData[3] = s * rhs.At(0, 3);

			sum.mData[4] = s * rhs.At(1, 0);
			sum.mData[5] = s * rhs.At(1, 1);
			sum.mData[6] = s * rhs.At(1, 2);
			sum.mData[7] = s * rhs.At(1, 3);
		}
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 3, 1, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = s * rhs.At(0, 0);
		sum.mData[1] = s * rhs.At(1, 0);
		sum.mData[2] = s * rhs.At(2, 0);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 3, 2, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = s * rhs.At(0, 0);
			sum.mData[1] = s * rhs.At(1, 0);
			sum.mData[2] = s * rhs.At(2, 0);

			sum.mData[3] = s * rhs.At(0, 1);
			sum.mData[4] = s * rhs.At(1, 1);
			sum.mData[5] = s * rhs.At(2, 1);
		}
		else
		{
			sum.mData[0] = s * rhs.At(0, 0);
			sum.mData[1] = s * rhs.At(0, 1);

			sum.mData[2] = s * rhs.At(1, 0);
			sum.mData[3] = s * rhs.At(1, 1);

			sum.mData[4] = s * rhs.At(2, 0);
			sum.mData[5] = s * rhs.At(2, 1);
		}

		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 3, 3, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = s * rhs.At(0, 0);
			sum.mData[1] = s * rhs.At(1, 0);
			sum.mData[2] = s * rhs.At(2, 0);

			sum.mData[3] = s * rhs.At(0, 1);
			sum.mData[4] = s * rhs.At(1, 1);
			sum.mData[5] = s * rhs.At(2, 1);

			sum.mData[6] = s * rhs.At(0, 2);
			sum.mData[7] = s * rhs.At(1, 2);
			sum.mData[8] = s * rhs.At(2, 2);
		}
		else
		{
			sum.mData[0] = s * rhs.At(0, 0);
			sum.mData[1] = s * rhs.At(0, 1);
			sum.mData[2] = s * rhs.At(0, 2);

			sum.mData[3] = s * rhs.At(1, 0);
			sum.mData[4] = s * rhs.At(1, 1);
			sum.mData[5] = s * rhs.At(1, 2);

			sum.mData[6] = s * rhs.At(2, 0);
			sum.mData[7] = s * rhs.At(2, 1);
			sum.mData[8] = s * rhs.At(2, 2);
		}
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 3, 4, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[ 0] = s * rhs.At(0, 0);
			sum.mData[ 1] = s * rhs.At(1, 0);
			sum.mData[ 2] = s * rhs.At(2, 0);

			sum.mData[ 3] = s * rhs.At(0, 1);
			sum.mData[ 4] = s * rhs.At(1, 1);
			sum.mData[ 5] = s * rhs.At(2, 1);

			sum.mData[ 6] = s * rhs.At(0, 2);
			sum.mData[ 7] = s * rhs.At(1, 2);
			sum.mData[ 8] = s * rhs.At(2, 2);

			sum.mData[ 9] = s * rhs.At(0, 3);
			sum.mData[10] = s * rhs.At(1, 3);
			sum.mData[11] = s * rhs.At(2, 3);
		}
		else
		{
			sum.mData[ 0] = s * rhs.At(0, 0);
			sum.mData[ 1] = s * rhs.At(1, 1);
			sum.mData[ 2] = s * rhs.At(2, 2);
			sum.mData[ 3] = s * rhs.At(2, 3);

			sum.mData[ 4] = s * rhs.At(0, 0);
			sum.mData[ 5] = s * rhs.At(1, 1);
			sum.mData[ 6] = s * rhs.At(2, 2);
			sum.mData[ 7] = s * rhs.At(2, 3);

			sum.mData[ 8] = s * rhs.At(0, 0);
			sum.mData[ 9] = s * rhs.At(1, 1);
			sum.mData[10] = s * rhs.At(2, 2);
			sum.mData[11] = s * rhs.At(2, 3);
		}
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 4, 1, MATRIX_MAJOR_TYPE> sum;
		sum.mData[0] = s * rhs.At(0, 0);
		sum.mData[1] = s * rhs.At(1, 0);
		sum.mData[2] = s * rhs.At(2, 0);
		sum.mData[3] = s * rhs.At(3, 0);
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 4, 2, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[0] = s * rhs.At(0, 0);
			sum.mData[1] = s * rhs.At(1, 0);
			sum.mData[2] = s * rhs.At(2, 0);
			sum.mData[3] = s * rhs.At(3, 0);

			sum.mData[4] = s * rhs.At(0, 1);
			sum.mData[5] = s * rhs.At(1, 1);
			sum.mData[6] = s * rhs.At(2, 1);
			sum.mData[7] = s * rhs.At(3, 1);
		}
		else
		{
			sum.mData[0] = s * rhs.At(0, 0);
			sum.mData[1] = s * rhs.At(0, 1);

			sum.mData[2] = s * rhs.At(1, 0);
			sum.mData[3] = s * rhs.At(1, 1);

			sum.mData[4] = s * rhs.At(2, 0);
			sum.mData[5] = s * rhs.At(2, 1);

			sum.mData[5] = s * rhs.At(3, 0);
			sum.mData[6] = s * rhs.At(3, 1);
		}

		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 4, 3, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[ 0] = s * rhs.At(0, 0);
			sum.mData[ 1] = s * rhs.At(1, 0);
			sum.mData[ 2] = s * rhs.At(2, 0);
			sum.mData[ 3] = s * rhs.At(3, 0);

			sum.mData[ 4] = s * rhs.At(0, 1);
			sum.mData[ 5] = s * rhs.At(1, 1);
			sum.mData[ 6] = s * rhs.At(2, 1);
			sum.mData[ 7] = s * rhs.At(3, 1);

			sum.mData[ 8] = s * rhs.At(0, 2);
			sum.mData[ 9] = s * rhs.At(1, 2);
			sum.mData[10] = s * rhs.At(2, 2);
			sum.mData[11] = s * rhs.At(3, 2);
		}
		else
		{
			sum.mData[ 0] = s * rhs.At(0, 0);
			sum.mData[ 1] = s * rhs.At(0, 1);
			sum.mData[ 2] = s * rhs.At(0, 2);

			sum.mData[ 3] = s * rhs.At(1, 0);
			sum.mData[ 4] = s * rhs.At(1, 1);
			sum.mData[ 5] = s * rhs.At(1, 2);

			sum.mData[ 6] = s * rhs.At(2, 0);
			sum.mData[ 7] = s * rhs.At(2, 1);
			sum.mData[ 8] = s * rhs.At(2, 2);

			sum.mData[ 9] = s * rhs.At(3, 0);
			sum.mData[10] = s * rhs.At(3, 1);
			sum.mData[11] = s * rhs.At(3, 2);
		}
		return sum;
	}

	export template<CArithmeticType ArithmeticType, eMatrixMajorType MATRIX_MAJOR_TYPE>
	CGS_INLINE constexpr Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE> operator*(const float s, const Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE>& rhs) noexcept
	{
		Matrix<ArithmeticType, 4, 4, MATRIX_MAJOR_TYPE> sum;
		if constexpr (MATRIX_MAJOR_TYPE == eMatrixMajorType::COLUMN)
		{
			sum.mData[ 0] = s * rhs.At(0, 0);
			sum.mData[ 1] = s * rhs.At(1, 0);
			sum.mData[ 2] = s * rhs.At(2, 0);
			sum.mData[ 3] = s * rhs.At(3, 0);

			sum.mData[ 4] = s * rhs.At(0, 1);
			sum.mData[ 5] = s * rhs.At(1, 1);
			sum.mData[ 6] = s * rhs.At(2, 1);
			sum.mData[ 7] = s * rhs.At(3, 1);

			sum.mData[ 8] = s * rhs.At(0, 2);
			sum.mData[ 9] = s * rhs.At(1, 2);
			sum.mData[10] = s * rhs.At(2, 2);
			sum.mData[11] = s * rhs.At(3, 2);

			sum.mData[12] = s * rhs.At(0, 3);
			sum.mData[13] = s * rhs.At(1, 3);
			sum.mData[14] = s * rhs.At(2, 3);
			sum.mData[15] = s * rhs.At(3, 3);
		}
		else
		{
			sum.mData[ 0] = s * rhs.At(0, 0);
			sum.mData[ 1] = s * rhs.At(0, 1);
			sum.mData[ 2] = s * rhs.At(0, 2);
			sum.mData[ 3] = s * rhs.At(0, 3);

			sum.mData[ 4] = s * rhs.At(0, 0);
			sum.mData[ 5] = s * rhs.At(1, 1);
			sum.mData[ 6] = s * rhs.At(1, 2);
			sum.mData[ 7] = s * rhs.At(1, 3);

			sum.mData[ 8] = s * rhs.At(0, 0);
			sum.mData[ 9] = s * rhs.At(2, 1);
			sum.mData[10] = s * rhs.At(2, 2);
			sum.mData[11] = s * rhs.At(2, 3);

			sum.mData[12] = s * rhs.At(3, 0);
			sum.mData[13] = s * rhs.At(3, 1);
			sum.mData[14] = s * rhs.At(3, 2);
			sum.mData[15] = s * rhs.At(3, 3);
		}
		return sum;
	}
}
}