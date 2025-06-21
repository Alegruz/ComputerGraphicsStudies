#include "Core/pch.h"

#include "Core/Window.h"

#if defined(CGS_WIN32)

#include "Core/Delegate.h"

#include <iostream>

namespace cgs::core
{
    LRESULT CALLBACK Window::StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
    {
        Window* pWindow;
        if (uMsg == WM_CREATE)
        {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pWindow = reinterpret_cast<Window*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pWindow);
        }
        else
        {
            pWindow = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        }

        if (pWindow != nullptr)
        {
			return pWindow->HandleMessage(uMsg, wParam, lParam);
        }

        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    Window::Window(const CreateInfo& createInfo) noexcept
        : mProjectInfo(createInfo.CurrentProjectInfo)
        , mhInstance(static_cast<HINSTANCE>(GetModuleHandle(nullptr)))
    {
        mProjectNameW.assign(mProjectInfo.Name.begin(), mProjectInfo.Name.end());

        WCHAR szExePath[MAX_PATH];
        GetModuleFileName(NULL, szExePath, MAX_PATH);

        // Register the windows class
        WNDCLASS wndClass =
        {
			.style = CS_HREDRAW | CS_VREDRAW,
			.lpfnWndProc = StaticWindowProc,
			.cbClsExtra = 0,
			.cbWndExtra = 0,
			.hInstance = mhInstance,
			.hIcon = LoadIcon(NULL, IDI_APPLICATION), // Load the default application icon
			.hCursor = LoadCursor(NULL, IDC_ARROW), // Load the default arrow cursor
			.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH), // Use a black brush for the background
			.lpszMenuName = NULL, // No menu for this window
			.lpszClassName = mProjectNameW.c_str() // Use the project name as the class name
		};

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

        int x = CW_USEDEFAULT;
        int y = CW_USEDEFAULT;

        // No menu in this example.
        mhMenu = NULL;

        // This example uses a non-resizable 640 by 480 viewport for simplicity.
        int nDefaultWidth = createInfo.Width;
        int nDefaultHeight = createInfo.Height;
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
            this
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

	Window::~Window() noexcept
	{
		if (mhWnd != NULL)
		{
			DestroyWindow(mhWnd);
			mhWnd = NULL;
		}
		if (mhInstance != NULL)
		{
			UnregisterClass(mProjectNameW.c_str(), mhInstance);
			mhInstance = NULL;
		}
	}

    LRESULT Window::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) noexcept
	{
        switch (message)
        {
        case WM_CLOSE:
        {
            HMENU hMenu;
            hMenu = GetMenu(mhWnd);
            if (hMenu != NULL)
            {
                DestroyMenu(hMenu);
            }
            DestroyWindow(mhWnd);
            UnregisterClass(
                mProjectNameW.c_str(),
                mhInstance
            );
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        }

        std::unordered_map<uint32_t, Delegate<void(void)>>::const_iterator it = mDelegates.find(message);
        if (it != mDelegates.end())
        {
            it->second.Invoke(); // Invoke the delegate associated with the message ID
            return 0; // Return 0 to indicate that the message has been handled
        }

        return DefWindowProc(mhWnd, message, wParam, lParam);
	}

	void Window::RegisterDelegate(uint32_t messageId, Delegate<void(void)>&& delegate) noexcept
	{
		mDelegates[messageId] = std::move(delegate); // Register the delegate for the specified message ID
	}

	void Window::Show() const noexcept
	{
		if (mhWnd != NULL)
		{
			ShowWindow(mhWnd, SW_SHOWDEFAULT);
		}
		else
		{
			CGS_LOG_ERROR("Window handle is null, cannot show the window.");
		}
	}
}
#endif // defined(CGS_WIN32)