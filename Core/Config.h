#pragma once

namespace cgs::core
{
    template<typename T>
    concept ConfigValueType = std::is_same_v<T, std::string> || std::is_same_v<T, uint32_t> || std::is_same_v<T, bool> || std::is_same_v<T, float> || std::is_same_v<T, std::filesystem::path>;

    class Config final
    {
    public:
        struct CreateInfo final
        {
            std::filesystem::path ConfigFilePath; // Path to the configuration file
        };

    public:
        Config() = delete;
        explicit Config(const CreateInfo &createInfo) noexcept;

        Config(const Config&) = delete;
        Config(Config&&) noexcept = default;
        ~Config() noexcept = default;

        Config& operator=(const Config&) = delete;
        Config& operator=(Config&&) noexcept = delete;

        CGS_INLINE constexpr const std::filesystem::path& GetConfigFilePath() const noexcept { return mConfigFilePath; } // Accessor for the configuration file path
        CGS_INLINE constexpr const std::unordered_map<std::string, std::string>& GetStringSettings() const noexcept { return mStringSettings; } // Accessor for string settings
        CGS_INLINE constexpr const std::unordered_map<std::string, uint32_t>& GetIntSettings() const noexcept { return mIntSettings; } // Accessor for integer settings
        CGS_INLINE constexpr const std::unordered_map<std::string, bool>& GetBoolSettings() const noexcept { return mBoolSettings; } // Accessor for boolean settings
        CGS_INLINE constexpr const std::unordered_map<std::string, float>& GetFloatSettings() const noexcept { return mFloatSettings; } // Accessor for float settings

        template<ConfigValueType T>
        bool GetSetting(const std::string& key, T& outValue) const noexcept; // Get a configuration setting by key
        
        template<ConfigValueType T>
        constexpr void SetSetting(const std::string& key, const T& value) noexcept; // Set a configuration setting
    
    private:
        void loadConfig(const std::filesystem::path& filePath) noexcept;

    private:
        std::filesystem::path mConfigFilePath; // Path to the configuration file
        std::unordered_map<std::string, std::string> mStringSettings; // Key-value pairs for configuration settings
        std::unordered_map<std::string, uint32_t> mIntSettings; // Key-value pairs for integer configuration settings
        std::unordered_map<std::string, bool> mBoolSettings; // Key-value pairs for boolean configuration settings
        std::unordered_map<std::string, float> mFloatSettings; // Key-value pairs for float configuration settings
    };

    template<ConfigValueType T>
    CGS_INLINE bool Config::GetSetting(const std::string& key, T& outValue) const noexcept
    {
        if constexpr (std::is_same_v<T, std::string>)
        {
            auto it = mStringSettings.find(key);
            if (it != mStringSettings.end())
            {
                outValue = it->second;
                return true;
            }
        }
        else if constexpr (std::is_same_v<T, std::filesystem::path>)
        {
            auto it = mStringSettings.find(key);
            if (it != mStringSettings.end())
            {
                outValue = std::filesystem::path(it->second);
                return true;
            }
        }
        else if constexpr (std::is_same_v<T, uint32_t>)
        {
            auto it = mIntSettings.find(key);
            if (it != mIntSettings.end())
            {
                outValue = it->second;
                return true;
            }
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            auto it = mBoolSettings.find(key);
            if (it != mBoolSettings.end())
            {
                outValue = it->second;
                return true;
            }
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            auto it = mFloatSettings.find(key);
            if (it != mFloatSettings.end())
            {
                outValue = it->second;
                return true;
            }
        }
        return false; // Setting not found
    }

    template<ConfigValueType T>
    CGS_INLINE constexpr void Config::SetSetting(const std::string& key, const T& value) noexcept
    {
        if constexpr (std::is_same_v<T, std::string>)
        {
            mStringSettings[key] = value;
        }
        else if constexpr (std::is_same_v<T, std::filesystem::path>)
        {
            mStringSettings[key] = value.string(); // Store as string
        }
        else if constexpr (std::is_same_v<T, uint32_t>)
        {
            mIntSettings[key] = value;
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            mBoolSettings[key] = value;
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            mFloatSettings[key] = value;
        }
    }
} // namespace cgs::core
