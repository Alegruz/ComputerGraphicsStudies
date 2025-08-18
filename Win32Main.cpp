#if defined(CGS_WINDOWS)
#if !defined(UNICODE)
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int commandShow)
{
    return 0;
}

#endif // defined(CGS_WINDOWS)