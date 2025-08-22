#pragma once

#include <algorithm>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>

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

#if defined(CGS_WINDOWS)
#if !defined(UNICODE)
#define UNICODE
#endif

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <shobjidl.h>

#if defined(CGS_COMPILER_MSVC)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif	// defined(CGS_COMPILER_MSVC)
#endif  // defined(CGS_WINDOWS)


#if defined(CGS_LINUX) // build this file only for your Linux/Wayland target

#include "pch.hpp"

#include <cairo/cairo.h>
#include <wayland-client.h>
#include <poll.h>

#include <climits>   // INT32_MAX
#include <cstdio>
#include <cstring>
#include <cstdint>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define TEXT(str) (str)

// Generated from wayland-protocols' xdg-shell.xml (see CMake notes below)
#include "xdg-shell-client-protocol.h"
#endif  // defined(CGS_LINUX)

#include "Common.h"