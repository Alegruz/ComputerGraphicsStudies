#if defined(CGS_WINDOWS)
#if !defined(UNICODE)
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int WINAPI wWinMain([[maybe_unused]] HINSTANCE instance, HINSTANCE, [[maybe_unused]] PWSTR commandLine, [[maybe_unused]] int commandShow)
{
    return 0;
}

#endif // defined(CGS_WINDOWS)