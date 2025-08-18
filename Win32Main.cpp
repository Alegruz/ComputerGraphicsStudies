#if defined(CGS_WINDOWS)
#if !defined(UNICODE)
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static bool gIsRunning = true;

/// @brief A callback function, which you define in your application, that processes messages sent to a window.
/// @param window A handle to the window.
/// @param message The message.
/// @param wParam Additional message information.
/// @param lParam Additional message information.
/// @return The return value is the result of the message processing, and depends on the message sent.
static LRESULT WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        gIsRunning = false;
        return 0;
    
    default:
        break;
    }

    return DefWindowProc(window, message, wParam, lParam);
}

/// @brief Prints the last error message.
static void PrintErrorMessage()
{
    const DWORD error = GetLastError();
    static constexpr const size_t ERROR_MESSAGE_SIZE = 1024;
    TCHAR errorMessage[ERROR_MESSAGE_SIZE] = { 0, };
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errorMessage, (sizeof(errorMessage) / sizeof(TCHAR)), NULL);
    MessageBox(NULL, errorMessage, TEXT("Error"), MB_OK | MB_ICONERROR);
}

/// @brief The entry point for a Windows application.
/// @param instance the handle to an instance or handle to a module. The operating system uses this value to identify the executable or EXE when it's loaded in memory. Certain Windows functions need the instance handle, for example to load icons or bitmaps.
/// @param
/// @param commandLine the command-line arguments as a Unicode string.
/// @param commandShowFlag a flag that indicates whether the main application window is minimized, maximized, or shown normally.
/// @return The exit value of the application.
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, [[maybe_unused]] PWSTR commandLine, [[maybe_unused]] int commandShowFlag)
{
    const WNDCLASS windowClass =
    {
        .style = CS_HREDRAW | CS_VREDRAW,
        // A pointer to the window procedure.
        .lpfnWndProc = WindowProcedure,
        // The number of extra bytes to allocate following the window-class structure. The system initializes the bytes to zero.
        // .cbClsExtra = 0,
        // The number of extra bytes to allocate following the window instance. The system initializes the bytes to zero.
        // .cbWndExtra = 0,
        .hInstance = instance,
        .hIcon = NULL,
        .hCursor = NULL,
        .hbrBackground = NULL,
        .lpszMenuName = NULL,
        .lpszClassName = TEXT("ComputerGraphicsStudies")
    };

    const ATOM classRegistrationResult = RegisterClass(&windowClass);
    if (classRegistrationResult == 0)
    {
        PrintErrorMessage();
        return GetLastError();
    }

    const LPCTSTR className = windowClass.lpszClassName;
    const LPCTSTR windowName = TEXT("Computer Graphics Studies");
    const DWORD windowStyle = WS_VISIBLE;
    const int windowX = 0;
    const int windowY = 0;
    const int windowWidth = CW_USEDEFAULT;
    const int windowHeight = CW_USEDEFAULT;

    const HWND window = CreateWindow(
        className,
        windowName,
        windowStyle,
        windowX, windowY, windowWidth, windowHeight,
        NULL, NULL, instance, NULL
    );

    if(window == NULL)
    {
        PrintErrorMessage();
        return GetLastError();
    }

    // Sets the specified window's show state.
    ShowWindow(window, commandShowFlag);

    while(gIsRunning == true)
    {
        MSG message = { 0 };
        while(PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }

    return 0;
}

#endif // defined(CGS_WINDOWS)