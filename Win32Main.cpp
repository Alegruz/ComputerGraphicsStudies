#if defined(CGS_WINDOWS)
#include "pch.hpp"

static bool gIsRunning = true;

/// @brief Prints the last error message.
static void 
PrintErrorMessage() noexcept
{
    const DWORD error = GetLastError();
    static constexpr const size_t ERROR_MESSAGE_SIZE = 1024;
    TCHAR errorMessage[ERROR_MESSAGE_SIZE] = { 0, };
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errorMessage, (sizeof(errorMessage) / sizeof(TCHAR)), NULL);
    MessageBox(NULL, errorMessage, TEXT("Error"), MB_OK | MB_ICONERROR);
}

// -------------------- Menu IDs & MRU config --------------------
enum : UINT {
    IDM_FILE_OPEN          = 1001,
    IDM_FILE_RECENT_BASE   = 1100,   // contiguous block for recent entries
    IDM_FILE_RECENT_MAX    = 10      // how many you want to show
};

class File final
{
public:
    CGS_INLINE File()
        : mPath()
        , mFileHandle(NULL)
    {
    }

    CGS_INLINE File(const std::filesystem::path& path)
        : mPath(path)
        , mFileHandle(NULL)
    {
        Open(path);
    }

    CGS_INLINE ~File()
    {
        if (mFileHandle != NULL)
        {
            const BOOL result = CloseHandle(mFileHandle);
            if (result == FALSE)
            {
                PrintErrorMessage();
            }
        }
    }

    CGS_INLINE bool Open(const std::filesystem::path& path)
    {
        mPath = path;
        mFileHandle = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        if (mFileHandle == INVALID_HANDLE_VALUE)
        {
            PrintErrorMessage();
            return false;
        }

        return true;
    }

private:
    std::filesystem::path mPath;
    HANDLE mFileHandle;
};

static HMENU gMenuBar    = NULL;
static HMENU gFileMenu   = NULL;
static HMENU gRecentMenu = NULL;
static File gCurrentFile;
static std::vector<std::filesystem::path> gRecentFiles;

// -------------------- MRU submenu rebuild --------------------
static void
RebuildRecentMenu(HWND hwnd) noexcept
{
    // clear previous items
    for (int i = GetMenuItemCount(gRecentMenu) - 1; i >= 0; --i)
        DeleteMenu(gRecentMenu, i, MF_BYPOSITION);

    if (gRecentFiles.empty()) 
    {
        AppendMenu(gRecentMenu, MF_STRING | MF_DISABLED, IDM_FILE_RECENT_BASE, TEXT("(Empty)"));
    } 
    else 
    {
        const size_t count = std::min(gRecentFiles.size(), static_cast<size_t>(IDM_FILE_RECENT_MAX));
        for (size_t i = 0; i < count; ++i) 
        {
            const UINT id = IDM_FILE_RECENT_BASE + static_cast<UINT>(i);
            AppendMenuW(gRecentMenu, MF_STRING, id, gRecentFiles[i].c_str());
        }
    }
    DrawMenuBar(hwnd);
}

static void
AddRecentFile(HWND hwnd, 
              const std::wstring& path) noexcept
{
    // move-to-front unique
    auto it = std::find(gRecentFiles.begin(), gRecentFiles.end(), path);
    if (it != gRecentFiles.end()) 
    {
        gRecentFiles.erase(it);
    }
    gRecentFiles.insert(gRecentFiles.begin(), path);
    if (gRecentFiles.size() > IDM_FILE_RECENT_MAX) 
    {
        gRecentFiles.resize(IDM_FILE_RECENT_MAX);
    }
    RebuildRecentMenu(hwnd);
}

// -------------------- File open dialog (IFileOpenDialog) --------------------
static bool
ShowOpenFile(HWND owner, 
             std::filesystem::path& outPath) noexcept
{
    IFileOpenDialog* pDlg = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDlg));
    if (FAILED(hr)) 
    {
        return false;
    }

    // Example: set file types if you want
    static constexpr COMDLG_FILTERSPEC filters[] = { {L"OBJ Files", L"*.obj"} };
    pDlg->SetFileTypes(CGS_ARRAYSIZE(filters), filters);

    bool ok = false;
    hr = pDlg->Show(owner);
    if (SUCCEEDED(hr))
    {
        IShellItem* pItem = nullptr;
        if (SUCCEEDED(pDlg->GetResult(&pItem)) && pItem)
        {
            PWSTR pszFile = nullptr;
            if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFile)) && pszFile)
            {
                outPath = pszFile;
                CoTaskMemFree(pszFile);
                ok = true;
            }
            pItem->Release();
        }
    }
    pDlg->Release();
    return ok;
}

// -------------------- Menu creation --------------------
static void 
BuildMenuBar(HWND hwnd) noexcept
{
    gMenuBar    = CreateMenu();
    gFileMenu   = CreatePopupMenu();
    gRecentMenu = CreatePopupMenu();

    AppendMenu(gFileMenu, MF_STRING, IDM_FILE_OPEN, TEXT("&Open...\tCtrl+O"));
    AppendMenu(gFileMenu, MF_POPUP,  (UINT_PTR)gRecentMenu, TEXT("Open &Recent"));

    AppendMenu(gMenuBar, MF_POPUP, (UINT_PTR)gFileMenu, TEXT("&File"));

    SetMenu(hwnd, gMenuBar);
    RebuildRecentMenu(hwnd);
}

// -------------------- (Optional) accelerators: Ctrl+O --------------------
static HACCEL 
CreateAccelerators() noexcept
{
    ACCEL acc =
    {
        .fVirt = FCONTROL | FVIRTKEY,
        .key   = 'O',
        .cmd   = IDM_FILE_OPEN,
    };
    return CreateAcceleratorTable(&acc, 1);
}

/// @brief A callback function, which you define in your application, that processes messages sent to a window.
/// @param window A handle to the window.
/// @param message The message.
/// @param wParam Additional message information.
/// @param lParam Additional message information.
/// @return The return value is the result of the message processing, and depends on the message sent.
static LRESULT
WindowProcedure(HWND window, 
                UINT message, 
                WPARAM wParam, 
                LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND: 
    {
        const UINT id = LOWORD(wParam);
        if (id == IDM_FILE_OPEN) 
        {
            std::filesystem::path path;
            if (ShowOpenFile(window, path)) 
            {
                AddRecentFile(window, path);

                gCurrentFile.Open(path);

                MessageBoxW(window, path.c_str(), L"Opened", MB_OK); // TODO: actually open
            }
            return 0;
        }
        if (id >= IDM_FILE_RECENT_BASE && id < IDM_FILE_RECENT_BASE + IDM_FILE_RECENT_MAX) 
        {
            size_t idx = id - IDM_FILE_RECENT_BASE;
            if (idx < gRecentFiles.size()) 
            {
                const std::wstring& path = gRecentFiles[idx];
                MessageBoxW(window, path.c_str(), L"Open Recent", MB_OK); // TODO: actually open
            }
            return 0;
        }
    } break;
    case WM_DESTROY:
    {
        gIsRunning = false;
        return 0;
    } break;
    default:
        break;
    }

    return DefWindowProc(window, message, wParam, lParam);
}

class ComLibrary final
{
public:
    CGS_INLINE ComLibrary() noexcept
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (FAILED(hr))
        {
            PrintErrorMessage();
        }
    }

    CGS_INLINE ~ComLibrary() noexcept
    {
        CoUninitialize();
    }
};

/// @brief The entry point for a Windows application.
/// @param instance the handle to an instance or handle to a module. The operating system uses this value to identify the executable or EXE when it's loaded in memory. Certain Windows functions need the instance handle, for example to load icons or bitmaps.
/// @param
/// @param commandLine the command-line arguments as a Unicode string.
/// @param commandShowFlag a flag that indicates whether the main application window is minimized, maximized, or shown normally.
/// @return The exit value of the application.
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, [[maybe_unused]] PWSTR commandLine, [[maybe_unused]] int commandShowFlag)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr))
    {
        PrintErrorMessage();
        return hr;
    }

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

    BuildMenuBar(window);
    // Sets the specified window's show state.
    ShowWindow(window, commandShowFlag);

    HACCEL accelerators = CreateAccelerators();

    while(gIsRunning == true)
    {
        MSG message = { 0 };
        while(PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
        {
            if(TranslateAccelerator(message.hwnd, accelerators, &message) == 0)
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            }
        }
    }

    if(accelerators)
    {
        DestroyAcceleratorTable(accelerators);
    }

#if defined(_CRTDBG_MAP_ALLOC)
	_CrtDumpMemoryLeaks(); // Check for memory leaks if using MSVC
#endif	// defined(_CRTDBG_MAP_ALLOC)
    return 0;
}

#endif // defined(CGS_WINDOWS)