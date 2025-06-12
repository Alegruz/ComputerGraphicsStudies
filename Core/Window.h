#pragma once

#define UNICODE
#include <windows.h>

#include "Core/pch.h"

namespace cgs::core
{
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
}
