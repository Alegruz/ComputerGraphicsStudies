module;

#include "Core/PchCommon.h"
#include "Core/Concepts.h"

export module Core.Image;

import Core.Math.Matrix;

namespace cgs
{
	namespace core
	{
		export template<CArithmeticType ArithmeticType = float, uint16_t ROW_SIZE = 4, uint16_t COLUMN_SIZE = 4, eMatrixMajorType MATRIX_MAJOR_TYPE = eMatrixMajorType::COLUMN>
		using Image = Matrix;
	}
}