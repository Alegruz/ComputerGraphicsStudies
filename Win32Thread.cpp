#include "pch.hpp"

#if defined(CGS_WINDOWS)
#include "Thread.h"

namespace cgs
{
    struct ThreadHandle final
    {
        HANDLE Handle;
        DWORD ThreadId;
    };

    struct StartThunk final
    {
        ThreadProcess Process;
        ThreadProcessArgument Argument;
    };

    static DWORD WINAPI StartThread(LPVOID param)
    {
        StartThunk thunk = *static_cast<StartThunk*>(param);
        while (thunk.Argument.InoutThreadHandle == nullptr || IsThreadValid(*thunk.Argument.InoutThreadHandle) == false)
        {
            Yield();
        }
        
        thunk.Process(thunk.Argument);
        delete static_cast<StartThunk*>(param);
        return 0;
    }

    bool Create(std::shared_ptr<ThreadHandle>& outHandle, const ThreadCreateInfo& createInfo) noexcept
    {
        if(outHandle && outHandle->Handle)
        {
            return false;
        }

        outHandle = std::make_shared<ThreadHandle>();
        StartThunk* thunk = new StartThunk
        {
            .Process = createInfo.Process,
            .Argument =
            {
                .InoutThreadHandle = outHandle,
                .Argument = createInfo.Argument
            } 
        };
        const DWORD flags = 0;
        const SIZE_T stackSize = createInfo.StackSize;
        outHandle->Handle = CreateThread(nullptr, stackSize, StartThread, thunk, flags, &outHandle->ThreadId);
        if(outHandle->Handle == NULL)
        {
            delete thunk;
            return false;
        }

        wchar_t threadName[64] = { 0, };
        MultiByteToWideChar(CP_UTF8, 0, createInfo.Name.c_str(), -1, threadName, static_cast<int>(std::size(threadName)));
        SetThreadDescription(outHandle->Handle, threadName);
        return true;
    }

    void Join(ThreadHandle& inoutHandle) noexcept
    {
        if(inoutHandle.Handle == NULL)
        {
            return;
        }

        WaitForSingleObject(inoutHandle.Handle, INFINITE);
        CloseHandle(inoutHandle.Handle);
        inoutHandle.Handle = NULL;
        inoutHandle.ThreadId = 0;
    }

    void Detach(ThreadHandle& inoutHandle) noexcept
    {
        if(inoutHandle.Handle == NULL)
        {
            return;
        }

        CloseHandle(inoutHandle.Handle);
        inoutHandle.Handle = NULL;
        inoutHandle.ThreadId = 0;
    }

    bool IsThreadAlive(const ThreadHandle& inoutHandle) noexcept
    {
        if(inoutHandle.Handle == NULL)
        {
            return false;
        }

        DWORD exitCode;
        if(GetExitCodeThread(inoutHandle.Handle, &exitCode))
        {
            return exitCode == STILL_ACTIVE;
        }

        return false;
    }

    bool IsThreadValid(const ThreadHandle& inoutHandle) noexcept
    {
        return inoutHandle.Handle != NULL;
    }

    void SleepForMs(const uint32 ms) noexcept
    {
        Sleep(ms);
    }

    void Yield() noexcept
    {
        SwitchToThread();
    }

    void Destroy(ThreadHandle& inoutHandle) noexcept
    {
        Detach(inoutHandle);
    }

    uint32 GetLogicalProcessorsCount() noexcept
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        return sysInfo.dwNumberOfProcessors;
    }
}
#endif  // defined(CGS_WINDOWS)