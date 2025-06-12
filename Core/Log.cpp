#include "Core/pch.h"

#include "Core/Log.h"

#include <iostream>

namespace cgs::core
{
    void Log::Print(const eLogLevel logLevel, const char* fileName, const uint32_t lineNumber, const char* functionName, const char* message) const noexcept
    {
        if (logLevel < mCurrentLogLevel)
        {
            return;
        }

        switch (logLevel)
        {
        case eLogLevel::Info:
            std::cout << "[INFO] ";
            break;
        case eLogLevel::Warning:
            std::cout << "[WARNING] ";
            break;
        case eLogLevel::Error:
            std::cout << "[ERROR] ";
            break;
        case eLogLevel::Debug:
            std::cout << "[DEBUG] ";
            break;
        case eLogLevel::Critical:
            std::cout << "[CRITICAL] ";
            break;
        default:
            DEBUG_BREAK();
            return;
        }

        std::cout << fileName << ":" << lineNumber << " (" << functionName << "): " << message << '\n';
    }
} // namespace cgs::core
