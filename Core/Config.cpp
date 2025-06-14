#include "Core/pch.h"
#include "Core/Config.h"

namespace cgs::core
{
    Config::Config(const CreateInfo& createInfo) noexcept
        : mConfigFilePath(createInfo.ConfigFilePath)
        , mStringSettings()
        , mIntSettings()
        , mBoolSettings()
        , mFloatSettings()
    {
        // Ensure the configuration file path is valid
        mConfigFilePath = std::filesystem::absolute(mConfigFilePath);

        if (mConfigFilePath.empty() || !std::filesystem::exists(mConfigFilePath))
        {
            CGS_LOG_ERROR("Configuration file path is invalid or does not exist: %s", mConfigFilePath.string().c_str());
            return;
        }

        if (mConfigFilePath.extension() != ".ini")
        {
            CGS_LOG_ERROR("Config file extension is not .ini: %s", mConfigFilePath.extension().string().c_str());
            return;
        }
        CGS_LOG_INFO("Loading configuration from: %s", mConfigFilePath.string().c_str());

        // Load configuration from the specified file path
        loadConfig(mConfigFilePath);
    }

    void Config::loadConfig(const std::filesystem::path& filePath) noexcept
    {
        // Load configuration from the specified file path
        // This is a placeholder implementation; actual loading logic should be implemented here.
        CGS_LOG_INFO("Loading configuration from: %s", filePath.string().c_str());
        
        // Example settings for demonstration purposes
        if (filePath.extension() != ".ini")
        {
            CGS_LOG_ERROR("Config file extension is not .ini: %s", filePath.extension().string().c_str());
            return;
        }

        std::ifstream file(filePath);
        if (!file.is_open())
        {
            CGS_LOG_ERROR("Failed to open config file: %s", filePath.string().c_str());
            return;
        }

        std::string line;
        while (std::getline(file, line))
        {
            // Remove comments
            auto commentPos = line.find_first_of(";#");
            if (commentPos != std::string::npos)
            {
                line = line.substr(0, commentPos);
            }
            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            if (line.empty() || line.front() == '[')
            {
                continue; // Skip empty lines and section headers
            }

            auto delimiterPos = line.find('=');
            if (delimiterPos == std::string::npos)
            {
                continue; // Skip lines without '='
            }

            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            // Trim whitespace from key and value
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            if (!key.empty())
            {
                // Try to detect boolean
                std::string lowerValue = value;
                std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (lowerValue == "true" || lowerValue == "false")
                {
                    mBoolSettings[key] = (lowerValue == "true");
                }
                // Try to detect integer
                else
                {
                    char* endPtr = nullptr;
                    long intVal = std::strtol(value.c_str(), &endPtr, 10);
                    if (*endPtr == '\0')
                    {
                        mIntSettings[key] = static_cast<uint32_t>(intVal);
                    }
                    else
                    {
                        // Try to detect float
                        char* floatEndPtr = nullptr;
                        float floatVal = std::strtof(value.c_str(), &floatEndPtr);
                        if (*floatEndPtr == '\0')
                        {
                            mFloatSettings[key] = floatVal;
                        }
                        else
                        {
                            // Default to string
                            mStringSettings[key] = value;
                        }
                    }
                }
            }
        }
        file.close();
    }
} // namespace cgs::core
