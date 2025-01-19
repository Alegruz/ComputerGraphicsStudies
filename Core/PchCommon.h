#pragma once

#include <cassert>
#include <cstdint>
#include <string>

#define CGS_INLINE __forceinline
#define MAKE_API_VERSION(variant, major, minor, patch) \
    ((((uint32_t)(variant)) << 29U) | (((uint32_t)(major)) << 22U) | (((uint32_t)(minor)) << 12U) | ((uint32_t)(patch)))

#if defined(Core_EXPORTS)
#define CORE_API __declspec(dllexport)
#else	// NOT defined(Renderer_EXPORTS)
#define CORE_API __declspec(dllimport)
#endif	// NOT defined(Renderer_EXPORTS)

namespace cgs
{
	namespace core
	{
		enum class eMatrixMajorType : uint8_t
		{
			COLUMN,
			ROW,
		};

		struct ProjectInfo final
		{
			std::string Name;
			uint32_t	Version;
		};
	}
}