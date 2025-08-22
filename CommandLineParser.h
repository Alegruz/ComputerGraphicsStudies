#pragma once

namespace cgs
{
    enum class eOptionType : uint8
    {
        INVALID,
        HELP,
        VERSION,
        INPUT_RESOURCE,
    };

    class CommandLineParser final
    {
    public:
#if defined(CGS_WINDOWS)
        using CharType = wchar;
        using StringType = std::wstring;
        using StringViewType = std::wstring_view;
        CGS_INLINE static constexpr bool IsSpace(CharType ch) noexcept { return std::iswspace(ch); }
#elif defined(CGS_LINUX)
        using CharType = char;
        using StringType = std::string;
        using StringViewType = std::string_view;
        CGS_INLINE static constexpr bool IsSpace(CharType ch) noexcept { return std::isspace(ch); }
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

    private:
        static void initializeOptionsMap() noexcept;

    private:
        static std::unordered_map<StringType, OptionInfo> sOptionsMap;

    private:
        std::vector<StringType> mArguments;
    };
}