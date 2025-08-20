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

namespace cgs
{
    constexpr uint32 API_VERSION = MAKE_API_VERSION(0, 0, 0, 0);
}