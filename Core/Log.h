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
        void Print(const eLogLevel logLevel, const char* fileName, const uint32_t lineNumber, const char* functionName, const char* format, ...) const noexcept;
    
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

// Macros now accept format and variadic arguments
#define CGS_LOG(level, format, ...) \
    cgs::core::Log::GetInstance().Print(level, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define CGS_LOG_INFO(format, ...) CGS_LOG(cgs::core::Log::eLogLevel::Info, format, ##__VA_ARGS__)
#define CGS_LOG_WARNING(format, ...) CGS_LOG(cgs::core::Log::eLogLevel::Warning, format, ##__VA_ARGS__); DEBUG_BREAK()
#define CGS_LOG_ERROR(format, ...) CGS_LOG(cgs::core::Log::eLogLevel::Error, format, ##__VA_ARGS__); DEBUG_BREAK()
#define CGS_LOG_DEBUG(format, ...) CGS_LOG(cgs::core::Log::eLogLevel::Debug, format, ##__VA_ARGS__)
#define CGS_LOG_CRITICAL(format, ...) CGS_LOG(cgs::core::Log::eLogLevel::Critical, format, ##__VA_ARGS__); DEBUG_BREAK()
}