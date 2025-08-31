#include "pch.hpp"

#if defined(CGS_LINUX)
#include "Thread.h"

namespace cgs
{
    struct ThreadHandle final
    {
        pthread_t Handle;
        std::atomic<bool> IsAlive = false;
    };

    struct StartThunk final
    {
        ThreadProcess Process;
        ThreadProcessArgument Argument;
    };

    static void* StartThread(void* param)
    {
        StartThunk thunk = *static_cast<StartThunk*>(param);
        while (thunk.Argument.InoutThreadHandle == nullptr || IsThreadValid(*thunk.Argument.InoutThreadHandle) == false)
        {
            Yield();
        }

        thunk.Argument.InoutThreadHandle->IsAlive.store(true);
        thunk.Process(thunk.Argument);
        delete static_cast<StartThunk*>(param);
        return nullptr;
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
        if(pthread_create(&outHandle->Handle, nullptr, StartThread, thunk) != 0)
        {
            delete thunk;
            return false;
        }
        pthread_setname_np(outHandle->Handle, createInfo.Name.c_str());

        return true;
    }

    void Join(ThreadHandle& inoutHandle) noexcept
    {
        if(inoutHandle.Handle == 0)
        {
            return;
        }

        pthread_join(inoutHandle.Handle, nullptr);
        inoutHandle.Handle = 0;
    }

    void Detach(ThreadHandle& inoutHandle) noexcept
    {
        if(inoutHandle.Handle == 0)
        {
            return;
        }

        pthread_detach(inoutHandle.Handle);
        inoutHandle.Handle = 0;
    }

    bool IsThreadAlive(const ThreadHandle& inoutHandle) noexcept
    {
        if(inoutHandle.Handle == 0)
        {
            return false;
        }

        return inoutHandle.IsAlive.load();
    }

    bool IsThreadValid(const ThreadHandle& inoutHandle) noexcept
    {
        return inoutHandle.Handle != 0;
    }

    void SleepForMs(const uint32 ms) noexcept
    {
        usleep(ms * 1000);
    }

    void Yield() noexcept
    {
        sched_yield();
    }

    void Destroy(ThreadHandle& inoutHandle) noexcept
    {
        Detach(inoutHandle);
    }

    uint32 GetLogicalProcessorsCount() noexcept
    {
        return static_cast<uint32>(get_nprocs());
    }
}
#endif  // defined(CGS_LINUX)