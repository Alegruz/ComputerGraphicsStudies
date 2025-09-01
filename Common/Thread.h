#pragma once

namespace cgs
{
    struct ThreadHandle;
    struct ThreadProcessArgument final
    {
        std::shared_ptr<ThreadHandle> InoutThreadHandle;
        void* Argument;
    };

    using ThreadProcess = void(*)(ThreadProcessArgument& arg) noexcept;

    struct ThreadCreateInfo final
    {
        std::string Name;
        uint32 StackSize;
        ThreadProcess Process;
        void* Argument;
    };

    bool Create(std::shared_ptr<ThreadHandle>& outHandle, const ThreadCreateInfo& createInfo) noexcept;
    void Join(ThreadHandle& inoutHandle) noexcept;
    void Detach(ThreadHandle& inoutHandle) noexcept;
    bool IsThreadAlive(const ThreadHandle& inoutHandle) noexcept;
    bool IsThreadValid(const ThreadHandle& inoutHandle) noexcept;
    void SleepForMs(const uint32 ms) noexcept;
#undef Yield
    void Yield() noexcept;
    void Destroy(ThreadHandle& inoutHandle) noexcept;
    uint32 GetLogicalProcessorsCount() noexcept;
}
