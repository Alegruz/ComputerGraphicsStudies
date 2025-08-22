#pragma once

#if defined(CGS_COMPILER_MSVC)
#define CGS_INLINE __forceinline
#elif defined(CGS_COMPILER_CLANG) || defined(CGS_COMPILER_GCC)
#define CGS_INLINE inline __attribute__((always_inline))
#else	// NOT defined(CGS_WIN32) && NOT defined(CGS_LINUX)
#define CGS_INLINE inline
#endif	// NOT defined(CGS_WIN32) && NOT defined(CGS_LINUX)

#define CGS_ARRAYSIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


#define MAKE_API_VERSION(variant, major, minor, patch) \
	((((uint32)(variant)) << 29U) | (((uint32)(major)) << 22U) | (((uint32)(minor)) << 12U) | ((uint32)(patch)))
#define GET_API_VERSION_VARIANT(version) (((version) >> 29U) & 0x7U)
#define GET_API_VERSION_MAJOR(version) (((version) >> 22U) & 0x7FU)
#define GET_API_VERSION_MINOR(version) (((version) >> 12U) & 0x3FFU)
#define GET_API_VERSION_PATCH(version) (((version) >> 0U) & 0xFFFU)

using byte = uint8_t;

using wchar = wchar_t;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

namespace cgs
{
    constexpr uint32 API_VERSION = MAKE_API_VERSION(0, 0, 0, 0);

    extern std::vector<std::filesystem::path> gRecentFiles;
}

#include "Math.h"