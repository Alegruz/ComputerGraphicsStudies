#pragma once

namespace cgs::core
{
    class Log final
    {
    public:
        enum class eLogLevel : uint8_t
        {
            Info,
            Warning,
            Error,
            Debug,
            Critical,
        };

    public:
        static Log& GetInstance() noexcept
        {
            static Log sInstance;
            return sInstance;
        }
    
    public:
        void Print(const eLogLevel logLevel, const char* fileName, const uint32_t lineNumber, const char* functionName, const char* message) const noexcept;
    
    private:
        CGS_INLINE constexpr Log() noexcept
            : mCurrentLogLevel(eLogLevel::Info)
        {
        }
        Log(const Log&) = delete;
        Log(Log&&) = delete;
        CGS_INLINE constexpr ~Log() noexcept = default;
        Log& operator=(const Log&) = delete;
        Log& operator=(Log&&) = delete;
    
    private:
        eLogLevel mCurrentLogLevel;
    };

#define CGS_LOG(level, message) \
    cgs::core::Log::GetInstance().Print(level, __FILE__, __LINE__, __FUNCTION__, message)
#define CGS_LOG_INFO(message) CGS_LOG(cgs::core::Log::eLogLevel::Info, message)
#define CGS_LOG_WARNING(message) CGS_LOG(cgs::core::Log::eLogLevel::Warning, message); DEBUG_BREAK()
#define CGS_LOG_ERROR(message) CGS_LOG(cgs::core::Log::eLogLevel::Error, message); DEBUG_BREAK()
#define CGS_LOG_DEBUG(message) CGS_LOG(cgs::core::Log::eLogLevel::Debug, message)
#define CGS_LOG_CRITICAL(message) CGS_LOG(cgs::core::Log::eLogLevel::Critical, message);   DEBUG_BREAK()
}