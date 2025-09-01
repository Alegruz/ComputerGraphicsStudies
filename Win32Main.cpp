#include "pch.hpp"

#if defined(CGS_WINDOWS)
#include "Common.cpp"

#if defined(CGS_GRAPHICS_API_CPU)
#elif defined(CGS_GRAPHICS_API_D3D12)   // NOT defined(CGS_GRAPHICS_API_CPU)
#include "D3D12Renderer.cpp"
#else   // NOT defined(CGS_GRAPHICS_API_D3D12) && NOT defined(CGS_GRAPHICS_API_CPU)
#error Unsupported graphics API type
#endif  // NOT defined(CGS_GRAPHICS_API_D3D12) && NOT defined(CGS_GRAPHICS_API_CPU)

#include "Win32Thread.cpp"

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
enum : UINT 
{
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

namespace cgs
{
    std::vector<std::filesystem::path> gRecentFiles;
    std::vector<Texture> gBackBuffers(BACK_BUFFERS_COUNT, Texture(Texture::CreateInfo{ .Format = RenderResource::eFormat::BGRA8_UNORM, .Width = 1600, .Height = 900, .Depth = 1, .Name = std::string("Back Buffer") }));
    std::vector<Texture> gDepthBuffers(BACK_BUFFERS_COUNT, Texture(Texture::CreateInfo{ .Format = RenderResource::eFormat::D32_UNORM, .Width = 1600, .Height = 900, .Depth = 1, .Name = std::string("Depth Buffer") }));
    static RenderThreadInfo gRenderThread;
}

// -------------------- MRU submenu rebuild --------------------
static void
RebuildRecentMenu(HWND hwnd) noexcept
{
    // clear previous items
    for (int i = GetMenuItemCount(gRecentMenu) - 1; i >= 0; --i)
        DeleteMenu(gRecentMenu, i, MF_BYPOSITION);

    if (cgs::gRecentFiles.empty()) 
    {
        AppendMenu(gRecentMenu, MF_STRING | MF_DISABLED, IDM_FILE_RECENT_BASE, TEXT("(Empty)"));
    } 
    else 
    {
        const size_t count = std::min(cgs::gRecentFiles.size(), static_cast<size_t>(IDM_FILE_RECENT_MAX));
        for (size_t i = 0; i < count; ++i) 
        {
            const UINT id = IDM_FILE_RECENT_BASE + static_cast<UINT>(i);
            AppendMenuW(gRecentMenu, MF_STRING, id, cgs::gRecentFiles[i].c_str());
        }
    }
    DrawMenuBar(hwnd);
}

static void
AddRecentFile(HWND hwnd, 
              const std::wstring& path) noexcept
{
    // move-to-front unique
    auto it = std::find(cgs::gRecentFiles.begin(), cgs::gRecentFiles.end(), path);
    if (it != cgs::gRecentFiles.end()) 
    {
        cgs::gRecentFiles.erase(it);
    }
    cgs::gRecentFiles.insert(cgs::gRecentFiles.begin(), path);
    if (cgs::gRecentFiles.size() > IDM_FILE_RECENT_MAX) 
    {
        cgs::gRecentFiles.resize(IDM_FILE_RECENT_MAX);
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
            if (idx < cgs::gRecentFiles.size()) 
            {
                const std::wstring& path = cgs::gRecentFiles[idx];
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

namespace cgs
{
    static void
    Present(HWND window, const Texture& backBuffer) noexcept
    {
        BITMAPINFO bitmapInfo =
        {
            .bmiHeader = 
            BITMAPINFOHEADER
            {
                .biSize = sizeof(BITMAPINFOHEADER),
                .biWidth = static_cast<LONG>(backBuffer.GetWidth()),
                .biHeight = static_cast<LONG>(backBuffer.GetHeight()),
                .biPlanes = 1,
                .biBitCount = 32,
                .biCompression = BI_RGB,
                .biSizeImage = 0,
                .biXPelsPerMeter = 0,
                .biYPelsPerMeter = 0,
                .biClrUsed = 0,
                .biClrImportant = 0,
            },
            .bmiColors = 
            { 
                RGBQUAD
                {
                    .rgbBlue = 0,
                    .rgbGreen = 0,
                    .rgbRed = 0,
                    .rgbReserved = 0,
                }
            },
        };
        HDC deviceContext = GetDC(window);
        StretchDIBits(deviceContext, 0, 0, backBuffer.GetWidth(), backBuffer.GetHeight(), 0, 0, backBuffer.GetWidth(), backBuffer.GetHeight(), backBuffer.GetData(), &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
        ReleaseDC(window, deviceContext);
    }
}

/// @brief The entry point for a Windows application.
/// @param instance the handle to an instance or handle to a module. The operating system uses this value to identify the executable or EXE when it's loaded in memory. Certain Windows functions need the instance handle, for example to load icons or bitmaps.
/// @param
/// @param commandLine the command-line arguments as a Unicode string.
/// @param commandShowFlag a flag that indicates whether the main application window is minimized, maximized, or shown normally.
/// @return The exit value of the application.
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, [[maybe_unused]] int commandShowFlag)
{
    cgs::CommandLineParser commandLineParser(commandLine);
    const bool isOptionFound = commandLineParser.ParseArguments();
    if (isOptionFound == false)
    {
        return 0;
    }
    
    if (isOptionFound == false)
    {
        OutputDebugString(L"No command line options found.\n");
        OutputDebugString(L"Exiting application.\n");
        return 0;
    }
    

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
    const int windowWidth = cgs::gBackBuffers[0].GetWidth();
    const int windowHeight = cgs::gBackBuffers[0].GetHeight();
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

    bool hasInitializedRenderer = false;
    cgs::eRenderDeviceType renderDeviceType = cgs::ConvertStringToEnumValue<cgs::eRenderDeviceType>(commandLineParser.GetArgument(cgs::eOptionType::RENDER_DEVICE));
    switch (renderDeviceType)
    {
    case cgs::eRenderDeviceType::CPU:
        break;
    case cgs::eRenderDeviceType::D3D12:
    {
        hasInitializedRenderer = cgs::InitializeRenderer<cgs::eRenderDeviceType::D3D12>();
        if (hasInitializedRenderer == false)
        {
            renderDeviceType = cgs::eRenderDeviceType::CPU;
        }
    }
        break;
    default:
        assert(false && "Unknown render device type");
        renderDeviceType = cgs::eRenderDeviceType::CPU;
        break;
    }

    switch (renderDeviceType)
    {
    case cgs::eRenderDeviceType::CPU:
    {
        hasInitializedRenderer = cgs::InitializeRenderer<cgs::eRenderDeviceType::CPU>();
    }
        break;
    case cgs::eRenderDeviceType::D3D12:
        break;
    default:
        assert(false && "Unknown render device type");
        break;
    }

    if(hasInitializedRenderer == false)
    {
        OutputDebugString(L"Failed to initialize renderer.\n");
        OutputDebugString(L"Exiting application.\n");
        return -1;
    }

    [[maybe_unused]] float deltaTimeInMs = 0.0f;
    std::vector<cgs::Geometry> cornellBox;
    cgs::CreateCornellBoxScene(cornellBox);

    cgs::gRenderThread.RenderMethod = cgs::eRenderMethod::RASTERIZATION;
    cgs::ThreadCreateInfo createInfo =
    {
        .Name = "RenderThread",
        .StackSize = 0,
        .Process = &cgs::RenderThreadMain,
        .Argument = &cgs::gRenderThread
    };
    const bool threadCreateResult = cgs::Create(cgs::gRenderThread.CurrentThreadHandle, createInfo);
    if (threadCreateResult == false)
    {
        assert(false && "Failed to create render thread");
    }

    uint64 workIndex = 0;
    LARGE_INTEGER startTime;
    LARGE_INTEGER endTime;
    LARGE_INTEGER elapsedMicroseconds;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&startTime);
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

        const uint32 currentFrameIndexToRender = static_cast<uint32>(workIndex % 3);
        bool isFirstFrame = false;
        while (true)
        {
            const uint64 lastCompleteWorkIndex = cgs::gRenderThread.LastCompleteWorkIndex.load();
            // const uint64 currentWorkIndex = cgs::gRenderThread.CurrentWorkIndex.load();
            // uint32 renderWorksCount = 0;
            // {
            //     const std::lock_guard lock(cgs::gRenderThread.RenderWorksMutex);
            //     renderWorksCount = static_cast<uint32>(cgs::gRenderThread.RenderWorksPerFrame.size());
            // }
            isFirstFrame = workIndex < cgs::BACK_BUFFERS_COUNT;
            const bool hasCompletedWork = lastCompleteWorkIndex != std::numeric_limits<uint64>::max();

            if (isFirstFrame == true || (hasCompletedWork && lastCompleteWorkIndex >= static_cast<uint64>(static_cast<int64>(workIndex) - static_cast<int64>(cgs::BACK_BUFFERS_COUNT))))
            {
                break;
            }
            cgs::Yield();
        }

        {
            if (isFirstFrame == false)
            {
                cgs::Present(window, cgs::gBackBuffers[currentFrameIndexToRender]);
            }

            const std::lock_guard lock(cgs::gRenderThread.RenderWorksMutex);
            cgs::gRenderThread.RenderWorksPerFrame.push(
                cgs::RenderWork
                {
                    .OutTexture = cgs::gBackBuffers[currentFrameIndexToRender],
                    .OutDepthBuffer = cgs::gDepthBuffers[currentFrameIndexToRender],
                    .Geometries = cornellBox,
                    .WorkIndex = workIndex++
                });
        }

        QueryPerformanceCounter(&endTime);
        elapsedMicroseconds.QuadPart = (endTime.QuadPart - startTime.QuadPart) * 1000000 / frequency.QuadPart;
        startTime = endTime;
        deltaTimeInMs = elapsedMicroseconds.QuadPart / 1000.0f;
    }

    if(cgs::IsThreadValid(*cgs::gRenderThread.CurrentThreadHandle))
    {
        cgs::Join(*cgs::gRenderThread.CurrentThreadHandle);
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