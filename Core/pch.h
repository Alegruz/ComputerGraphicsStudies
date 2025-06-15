#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cctype>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(CGS_COMPILER_MSVC)
#define CGS_INLINE __forceinline
#elif defined(CGS_COMPILER_CLANG) || defined(CGS_COMPILER_GCC)
#define CGS_INLINE inline __attribute__((always_inline))
#else	// NOT defined(CGS_WIN32) && NOT defined(CGS_LINUX)
#define CGS_INLINE inline
#endif	// NOT defined(CGS_WIN32) && NOT defined(CGS_LINUX)

#define MAKE_API_VERSION(variant, major, minor, patch) \
	((((uint32_t)(variant)) << 29U) | (((uint32_t)(major)) << 22U) | (((uint32_t)(minor)) << 12U) | ((uint32_t)(patch)))

#if defined(CGS_COMPILER_MSVC)
#define DEBUG_BREAK()	__debugbreak()
#elif defined(CGS_COMPILER_CLANG) || defined(CGS_COMPILER_GCC)
	#if defined(CGS_UNIX)
		#include <signal.h>
		#define DEBUG_BREAK() raise(SIGTRAP)
	#endif	// defined(CGS_UNIX)
#endif	// defined(CGS_COMPILER_MSVC)

#if !defined(DEBUG_BREAK)
#error "DEBUG_BREAK is not defined for the current compiler"
#endif	// !defined(DEBUG_BREAK)

#if defined(Core_EXPORTS)
#define CORE_API __declspec(dllexport)
#else	// NOT defined(Renderer_EXPORTS)
#define CORE_API /*__declspec(dllimport)*/
#endif	// NOT defined(Renderer_EXPORTS)

#include "Core/Log.h"
#include "Core/Config.h"

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