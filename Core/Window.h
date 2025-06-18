#pragma once

#if defined(CGS_WIN32)
#define UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#elif defined(CGS_UNIX)
#endif // defined(CGS_WIN32)

#include "Core/pch.h"

namespace cgs::core
{
#if defined(CGS_WIN32)
	class WindowWin32
	{
    public:
        static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept;

	public:
        WindowWin32() = delete;
		explicit WindowWin32(const ProjectInfo& projectInfo) noexcept;

        CGS_INLINE constexpr HINSTANCE GetInstance() const noexcept { return mhInstance; }
        CGS_INLINE constexpr HWND GetWindow() const noexcept { return mhWnd; }

	private:
        ProjectInfo     mProjectInfo;
        std::wstring    mProjectNameW;
		HINSTANCE       mhInstance;
        HWND            mhWnd;
        HMENU           mhMenu;
        RECT            mRect;
	};
#elif defined(CGS_UNIX)
    // Placeholder for Unix implementation
    class WindowUnix
    {
    public:
        WindowUnix() = delete;
        explicit WindowUnix(const ProjectInfo& projectInfo) noexcept;

        CGS_INLINE constexpr void* GetDisplay() const noexcept { return nullptr; } // Placeholder
        CGS_INLINE constexpr void* GetWindow() const noexcept { return nullptr; } // Placeholder
    };
#else
#error "Unsupported platform for Window implementation"
#endif // defined(CGS_WIN32)

#if defined(CGS_WIN32)
    using Window = WindowWin32;
#elif defined(CGS_UNIX)
    using Window = WindowUnix;
#else
#error "Unsupported platform for Window implementation"
#endif // defined(CGS_WIN32)
}
