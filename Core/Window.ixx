module;

#include <iostream>

#define UNICODE
#include <windows.h>

#include "Core/PchCommon.h"

export module Core.Window.Win32;

namespace cgs
{
namespace core
{
	export class WindowWin32
	{
    public:
        static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
        {
            WindowWin32* pWindow;
            if (uMsg == WM_CREATE)
            {
                CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
                pWindow = reinterpret_cast<WindowWin32*>(pCreate->lpCreateParams);
                SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pWindow);
            }
            else
            {
                pWindow = reinterpret_cast<WindowWin32*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            }

            switch (uMsg)
            {
            case WM_CLOSE:
            {
                HMENU hMenu;
                hMenu = GetMenu(hWnd);
                if (hMenu != NULL)
                {
                    DestroyMenu(hMenu);
                }
                DestroyWindow(hWnd);
                UnregisterClass(
                    pWindow->mProjectNameW.c_str(),
                    pWindow->mhInstance
                );
                return 0;
            }

            case WM_DESTROY:
                PostQuitMessage(0);
                break;
            }

            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }

	public:
        WindowWin32() = delete;
		CGS_INLINE explicit WindowWin32(const ProjectInfo& projectInfo) noexcept
			: mProjectInfo(projectInfo)
            , mhInstance(static_cast<HINSTANCE>(GetModuleHandle(nullptr)))
		{
            mProjectNameW.assign(mProjectInfo.Name.begin(), mProjectInfo.Name.end());

            HICON hIcon = NULL;
            WCHAR szExePath[MAX_PATH];
            GetModuleFileName(NULL, szExePath, MAX_PATH);

            // If the icon is NULL, then use the first one found in the exe
            if (hIcon == NULL)
            {
                hIcon = ExtractIcon(mhInstance, szExePath, 0);
            }

            // Register the windows class
            WNDCLASS wndClass;
            wndClass.style = CS_DBLCLKS;
            wndClass.lpfnWndProc = StaticWindowProc;
            wndClass.cbClsExtra = 0;
            wndClass.cbWndExtra = 0;
            wndClass.hInstance = mhInstance;
            wndClass.hIcon = hIcon;
            wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
            wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
            wndClass.lpszMenuName = NULL;
            wndClass.lpszClassName = mProjectNameW.c_str();

            if (!RegisterClass(&wndClass))
            {
                DWORD dwError = GetLastError();
                if (dwError != ERROR_CLASS_ALREADY_EXISTS)
                {
                    const HRESULT hr = HRESULT_FROM_WIN32(dwError);
                    std::cout << std::hex << "HRESULT: " << hr << '\n';
                }
                else
                {
                    std::cout << std::hex << "error: " << dwError << '\n';
                }
            }

            mRect;
            int x = CW_USEDEFAULT;
            int y = CW_USEDEFAULT;

            // No menu in this example.
            mhMenu = NULL;

            // This example uses a non-resizable 640 by 480 viewport for simplicity.
            int nDefaultWidth = 640;
            int nDefaultHeight = 480;
            SetRect(&mRect, 0, 0, nDefaultWidth, nDefaultHeight);
            AdjustWindowRect(
                &mRect,
                WS_OVERLAPPEDWINDOW,
                (mhMenu != NULL) ? true : false
            );

            // Create the window for our viewport.
            mhWnd = CreateWindow(
                mProjectNameW.c_str(),
                mProjectNameW.c_str(),
                WS_OVERLAPPEDWINDOW,
                x, y,
                (mRect.right - mRect.left), (mRect.bottom - mRect.top),
                0,
                mhMenu,
                mhInstance,
                0
            );

            if (mhWnd == NULL)
            {
                DWORD dwError = GetLastError();
                if (dwError != ERROR_CLASS_ALREADY_EXISTS)
                {
                    const HRESULT hr = HRESULT_FROM_WIN32(dwError);
                    std::cout << std::hex << "HRESULT: " << hr << '\n';
                }
                else
                {
                    std::cout << std::hex << "error: " << dwError << '\n';
                }
            }
		}

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
}




