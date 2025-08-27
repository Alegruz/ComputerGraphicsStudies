#pragma once

namespace cgs
{
    enum class eOptionType : uint8
    {
        INVALID,
        HELP,
        VERSION,
        INPUT_RESOURCE,
        RENDER_DEVICE,
        COUNT,
    };

    class CommandLineParser final
    {
    public:
#if defined(CGS_WINDOWS)
        using CharType = wchar;
        using StringType = std::wstring;
        using StringViewType = std::wstring_view;
        CGS_INLINE static bool IsSpace(CharType ch) noexcept { return std::iswspace(ch); }
        CGS_INLINE static std::string ToString(const StringType& str) noexcept 
        {
            if (str.empty()) 
            {
                return std::string();
            }

            const int size_needed = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0, nullptr, nullptr);
            std::string result(size_needed, 0);
            WideCharToMultiByte(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), &result[0], size_needed, nullptr, nullptr);
            return result;
        }
#elif defined(CGS_LINUX)
        using CharType = char;
        using StringType = std::string;
        using StringViewType = std::string_view;
        CGS_INLINE static constexpr bool IsSpace(CharType ch) noexcept { return std::isspace(ch); }
        CGS_INLINE static std::string ToString(const StringType& str) noexcept 
        {
            return str;
        }
#else   // NOT defined(CGS_WINDOWS) && NOT defined(CGS_LINUX)
#error "Unsupported platform"
#endif  // NOT defined(CGS_WINDOWS) && NOT defined(CGS_LINUX)

    struct OptionInfo final
    {
        const CharType* LongOption = nullptr;
        const CharType* ShortOption = nullptr;
        eOptionType Type = eOptionType::INVALID;
        StringType DataType = StringType();
        std::vector<StringType> PossibleValues = std::vector<StringType>();
        StringType Description = StringType();
        StringType HelpDescription = StringType();
    };

    public:
        static int FormatString(CharType* str, const size_t bufferSize, const CharType* format, ...) noexcept;

    public:
        CGS_INLINE CommandLineParser() noexcept: mArguments() { initializeOptionsMap(); }
        CommandLineParser(const CharType* commandLine) noexcept;
        CGS_INLINE CommandLineParser(const int32 argc, CharType** argv) noexcept
            : CommandLineParser()
        {
            mArguments.reserve(static_cast<size_t>(argc));
            for (int32 i = 0; i < argc; ++i)
            {
                mArguments.emplace_back(argv[i]);
            }
        }

        bool ParseArguments() noexcept;
        CGS_INLINE const std::string& GetArgument(const eOptionType optionType) const noexcept { return mOptionValues[static_cast<uint32>(optionType)]; }

    private:
        static void initializeOptionsMap() noexcept;

    private:
        static std::unordered_map<StringType, OptionInfo> sOptionsMap;

    private:
        std::vector<StringType> mArguments;
        std::array<std::string, static_cast<uint32>(eOptionType::COUNT)> mOptionValues;
    };
}