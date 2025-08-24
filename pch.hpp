#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <span>
#include <string>
#include <vector>
#include <unordered_map>

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
#include <cairo/cairo.h>
#include <wayland-client.h>
#include <poll.h>
#include <pthread.h>

#include <climits>   // INT32_MAX
#include <cstdio>
#include <cstring>
#include <cstdint>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#define TEXT(str) (str)

// Generated from wayland-protocols' xdg-shell.xml (see CMake notes below)
#include "xdg-shell-client-protocol.h"
#endif  // defined(CGS_LINUX)

#include "Common.h"