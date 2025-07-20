#pragma once

#if defined(CGS_WIN32)
#define UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#elif defined(CGS_UNIX)
#endif // defined(CGS_WIN32)

namespace cgs::core
{
    template <typename T>
    class Delegate;

#if defined(CGS_WIN32)
	class Window
	{
    public:
        static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept;

    public:
        struct CreateInfo final
        {
			ProjectInfo& CurrentProjectInfo; // Reference to the project information
            uint32_t Width = 1920;
            uint32_t Height = 1080;
        };

	public:
        Window() = delete;
		explicit Window(const CreateInfo& createInfo) noexcept;
		Window(const Window&) = delete;
		Window(Window&&) noexcept = default;
		~Window() noexcept;
		Window& operator=(const Window&) = delete;
		Window& operator=(Window&&) noexcept = delete;

        LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept; // Handle window messages
		void RegisterDelegate(uint32_t messageId, Delegate<void(void)>&& delegate) noexcept; // Register a delegate for a specific message ID
		void Show() const noexcept; // Show the window

        CGS_INLINE constexpr HINSTANCE GetInstance() const noexcept { return mhInstance; }
        CGS_INLINE constexpr HWND GetWindow() const noexcept { return mhWnd; }

	private:
        ProjectInfo     mProjectInfo;
        std::wstring    mProjectNameW;
		HINSTANCE       mhInstance;
        HWND            mhWnd;
        HMENU           mhMenu;
        RECT            mRect;

		std::unordered_map<uint32_t, Delegate<void(void)>> mDelegates; // Map of message IDs to delegates for handling messages
	};
#elif defined(CGS_UNIX)
    // Placeholder for Unix implementation
    class Window
    {
    public:
        struct CreateInfo final
        {
            ProjectInfo& CurrentProjectInfo; // Reference to the project information
            uint32_t Width = 1920;
            uint32_t Height = 1080;
        };

    public:
        Window() = delete;
        explicit Window(const CreateInfo& createInfo) noexcept;

        void Show() const noexcept;// Show the window

        CGS_INLINE constexpr void* GetDisplay() const noexcept { return nullptr; } // Placeholder
        CGS_INLINE constexpr void* GetWindow() const noexcept { return nullptr; } // Placeholder
    };
#else
#error "Unsupported platform for Window implementation"
#endif // defined(CGS_WIN32)
}
