#include "Core/pch.h"

#include "Core/Log.h"

#include <iostream>

namespace cgs::core
{
    void Log::Print(const eLogLevel logLevel, const char* fileName, const uint32_t lineNumber, const char* functionName, const char* format, ...) const noexcept
    {
        if (logLevel < mCurrentLogLevel)
        {
            return;
        }
        // ANSI color codes
        constexpr const char* RESET   = "\033[0m";
        constexpr const char* INFO    = "\033[36m";  // Cyan
        constexpr const char* WARNING = "\033[33m";  // Yellow
        constexpr const char* ERROR   = "\033[31m";  // Red
        constexpr const char* DEBUG   = "\033[35m";  // Magenta
        constexpr const char* CRITICAL= "\033[41;97m"; // White on Red background

        const char* color = RESET;
        const char* prefix = "";

        switch (logLevel)
        {
        case eLogLevel::Info:
            color = INFO;
            prefix = "[I]";
            break;
        case eLogLevel::Warning:
            color = WARNING;
            prefix = "[W]";
            break;
        case eLogLevel::Error:
            color = ERROR;
            prefix = "[E]";
            break;
        case eLogLevel::Debug:
            color = DEBUG;
            prefix = "[D]";
            break;
        case eLogLevel::Critical:
            color = CRITICAL;
            prefix = "[C]";
            break;
        default:
            DEBUG_BREAK();
            return;
        }

        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        std::cout << color << prefix << " " << fileName << ":" << lineNumber << " (" << functionName << "): " << buffer << RESET << '\n';
    }
} // namespace cgs::core
