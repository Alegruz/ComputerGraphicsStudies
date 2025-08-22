#if defined(CGS_WINDOWS)
#include "pch.hpp"

#include "CommandLineParser.h"

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
    enum class eCoordinateSpace : uint8
    {
        DEFAULT,
        OBJECT = DEFAULT,
        LOCAL = DEFAULT,
        WORLD,
        VIEW,
        PERSPECTIVE,
        SCREEN,
        NORMALIZED_DEVICE_COORDINATE,
    };

    struct float3 final
    {
        float X = 0.0f;
        float Y = 0.0f;
        float Z = 0.0f;

        CGS_INLINE constexpr float& operator[](size_t index) noexcept
        {
            return *(&X + index);
        }

        CGS_INLINE constexpr float operator[](size_t index) const noexcept
        {
            return *(&X + index);
        }
    };

    CGS_INLINE constexpr float3 operator+(const float3& lhs, const float3& rhs) noexcept
    {
        return float3{ lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z };
    }

    CGS_INLINE constexpr float3 operator-(const float3& lhs, const float3& rhs) noexcept
    {
        return float3{ lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z };
    }

    CGS_INLINE constexpr float Dot(const float3& lhs, const float3& rhs) noexcept
    {
        return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z;
    }

    CGS_INLINE constexpr float3 Cross(const float3& lhs, const float3& rhs) noexcept
    {
        return float3{
            lhs.Y * rhs.Z - lhs.Z * rhs.Y,
            lhs.Z * rhs.X - lhs.X * rhs.Z,
            lhs.X * rhs.Y - lhs.Y * rhs.X
        };
    }

    CGS_INLINE constexpr float3 ComputeBarycentricCoordinates(const float3& v0, const float3& v1, const float3& v2, const float3& point) noexcept
    {
        const float area = 0.5f * Dot(Cross(v1 - v0, v2 - v0), float3{ 0.0f, 0.0f, 1.0f });
        const float alpha = 0.5f * Dot(Cross(v2 - v1, point - v1), float3{ 0.0f, 0.0f, 1.0f }) / area;
        const float beta = 0.5f * Dot(Cross(v0 - v2, point - v2), float3{ 0.0f, 0.0f, 1.0f }) / area;
        const float gamma = 1.0f - alpha - beta;

        return float3{ alpha, beta, gamma };
    }

    template<eCoordinateSpace SPACE>
    using Coordinate = float3;

    template<eCoordinateSpace SPACE>
    class TriangleMesh final
    {
    public:
        CGS_INLINE constexpr TriangleMesh() = default;
        CGS_INLINE constexpr TriangleMesh(const Coordinate<SPACE>& v0, const Coordinate<SPACE>& v1, const Coordinate<SPACE>& v2) noexcept
            : mVertices{ v0, v1, v2 } {}
        CGS_INLINE constexpr TriangleMesh(const TriangleMesh& mesh) noexcept
            : mVertices{ mesh.mVertices[0], mesh.mVertices[1], mesh.mVertices[2] }  {}
        CGS_INLINE constexpr TriangleMesh(TriangleMesh&&) noexcept = default;
        CGS_INLINE ~TriangleMesh() noexcept = default;

        CGS_INLINE constexpr TriangleMesh& operator=(const TriangleMesh& mesh) noexcept
        {
            if (this != &mesh)
            {
                mVertices[0] = mesh.mVertices[0];
                mVertices[1] = mesh.mVertices[1];
                mVertices[2] = mesh.mVertices[2];
            }
            return *this;
        }
        CGS_INLINE constexpr TriangleMesh& operator=(TriangleMesh&&) noexcept = default;
        CGS_INLINE constexpr const Coordinate<SPACE>& GetVertex(size_t index) const noexcept
        {
            return mVertices[index];
        }
        CGS_INLINE constexpr void SetVertex(size_t index, const Coordinate<SPACE>& vertex) noexcept
        {
            if (index < 3)
            {
                mVertices[index] = vertex;
            }
        }

    private:
        Coordinate<SPACE> mVertices[3];
    };

    template<eCoordinateSpace SPACE>
    CGS_INLINE constexpr Coordinate<SPACE> ComputeBarycentricCoordinates(const TriangleMesh<SPACE>& mesh, const float3& point) noexcept
    {
        const float3& v0 = mesh.GetVertex(0);
        const float3& v1 = mesh.GetVertex(1);
        const float3& v2 = mesh.GetVertex(2);
        
        return ComputeBarycentricCoordinates(v0, v1, v2, point);
    }

    class Texture final
    {
    public:
        CGS_INLINE constexpr Texture() noexcept = default;
        CGS_INLINE constexpr Texture(const uint32 width, const uint32 height) noexcept
            : Texture(width, height, 0) {}
        CGS_INLINE constexpr Texture(const uint32 width, const uint32 height, const byte initialValue) noexcept
            : mWidth(width), mHeight(height), mData(width * height * 4, initialValue) {}
        CGS_INLINE constexpr Texture(const Texture&) noexcept = default;
        CGS_INLINE constexpr Texture(Texture&&) noexcept = default;
        CGS_INLINE ~Texture() noexcept = default;

        CGS_INLINE constexpr Texture& operator=(const Texture&) noexcept = default;
        CGS_INLINE constexpr Texture& operator=(Texture&&) noexcept = default;

        CGS_INLINE constexpr uint32 GetWidth() const noexcept { return mWidth; }
        CGS_INLINE constexpr uint32 GetHeight() const noexcept { return mHeight; }
        CGS_INLINE constexpr const byte* GetData() const noexcept { return mData.data(); }

        CGS_INLINE constexpr void Clear() noexcept { std::fill(mData.begin(), mData.end(), static_cast<byte>(0)); }
        CGS_INLINE constexpr bool SetFragmentValue(const uint32 x, const uint32 y, const byte r, const byte g, const byte b, const byte a) noexcept
        {
            if (x < mWidth && y < mHeight)
            {
                const size_t index = (y * mWidth + x) * 4;
                mData[index + 0] = r;
                mData[index + 1] = g;
                mData[index + 2] = b;
                mData[index + 3] = a;
                return true;
            }
            return false;
        }

    private:
        uint32 mWidth = 0;
        uint32 mHeight = 0;
        std::vector<byte> mData;
    };

    enum class eRasterizationMethod : uint8
    {
        DEFAULT,
        BARYCENTRIC = DEFAULT,
    };

    template<eCoordinateSpace SPACE, eRasterizationMethod METHOD = eRasterizationMethod::DEFAULT>
    static void
    Rasterize(Texture& outTexture, const std::vector<TriangleMesh<SPACE>>& meshes) noexcept
    {
        const uint32 width = outTexture.GetWidth();
        const uint32 height = outTexture.GetHeight();

        for (const TriangleMesh<SPACE>& mesh : meshes)
        {
            if constexpr (METHOD == eRasterizationMethod::BARYCENTRIC)
            {
                for(uint32 y = 0; y < height; ++y)
                {
                    for(uint32 x = 0; x < width; ++x)
                    {
                        if constexpr (SPACE == eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE)
                        {
                            const Coordinate<eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE> point{ 
                                static_cast<float>(x) / static_cast<float>(width) * 2.0f - 1.0f, 
                                static_cast<float>(y) / static_cast<float>(height) * 2.0f - 1.0f, 
                                0.0f 
                            };
                            const Coordinate<eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE> barycentricCoords = ComputeBarycentricCoordinates(mesh, point);
                            const bool isInTriangle = 0.0f <= barycentricCoords.X && barycentricCoords.X <= 1.0f &&
                                                      0.0f <= barycentricCoords.Y && barycentricCoords.Y <= 1.0f &&
                                                      0.0f <= barycentricCoords.Z && barycentricCoords.Z <= 1.0f;
                            if(isInTriangle)
                            {
                                // Simple rasterization logic: set every pixel to a color
                                // In a real application, you would perform actual rasterization here
                                outTexture.SetFragmentValue(x, y, 255, 0, 0, 255); // Set to red
                            }
                        }
                    }
                }
            }
        }
    }

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

    cgs::Texture backBuffer(1920, 1080);

    const LPCTSTR className = windowClass.lpszClassName;
    const LPCTSTR windowName = TEXT("Computer Graphics Studies");
    const DWORD windowStyle = WS_VISIBLE;
    const int windowX = 0;
    const int windowY = 0;
    const int windowWidth = backBuffer.GetWidth();
    const int windowHeight = backBuffer.GetHeight();
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

    float deltaTimeInMs = 0.0f;
    float vertexAnimationVelocity = 0.01f;
    cgs::TriangleMesh<cgs::eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE> mesh(cgs::Coordinate<cgs::eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE>{ -1.0f, -1.0f, 0.0f },
                                                            cgs::Coordinate<cgs::eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE>{ 1.0f, -1.0f, 0.0f },
                                                            cgs::Coordinate<cgs::eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE>{ 0.0f, 1.0f, 0.0f });
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

        cgs::float3 vertex = mesh.GetVertex(2);
        vertex.X += vertexAnimationVelocity * deltaTimeInMs;
        if(vertex.X > 1.0f)
        {
            vertexAnimationVelocity = -vertexAnimationVelocity;
        }
        else if(vertex.X < -1.0f)
        {
            vertexAnimationVelocity = -vertexAnimationVelocity;
        }
        mesh.SetVertex(2, vertex);
        const std::vector<cgs::TriangleMesh<cgs::eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE>> meshes = { mesh };
        backBuffer.Clear();
        cgs::Rasterize(backBuffer, meshes);
        cgs::Present(window, backBuffer);
        QueryPerformanceCounter(&endTime);
        elapsedMicroseconds.QuadPart = (endTime.QuadPart - startTime.QuadPart) * 1000000 / frequency.QuadPart;
        deltaTimeInMs = elapsedMicroseconds.QuadPart / 1000.0f;
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