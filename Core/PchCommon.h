#pragma once

#define CGS_INLINE __forceinline

namespace cgs
{
	namespace core
	{
		enum class eMatrixMajorType : uint8_t
		{
			COLUMN,
			ROW,
		};
	}
}