#pragma once

#if defined(CGS_COMPILER_MSVC)
#define CGS_INLINE __forceinline
#elif defined(CGS_COMPILER_CLANG) || defined(CGS_COMPILER_GCC)
#define CGS_INLINE inline __attribute__((always_inline))
#else	// NOT defined(CGS_WIN32) && NOT defined(CGS_LINUX)
#define CGS_INLINE inline
#endif	// NOT defined(CGS_WIN32) && NOT defined(CGS_LINUX)

#define CGS_ARRAYSIZE(arr) (sizeof(arr) / sizeof((arr)[0]))