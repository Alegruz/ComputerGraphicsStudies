#include "pch.hpp"

#include "CommandLineParser.h"

namespace cgs
{
    extern std::vector<std::filesystem::path> gRecentFiles;

    std::unordered_map<CommandLineParser::StringType, CommandLineParser::OptionInfo> CommandLineParser::sOptionsMap =
    {
        { TEXT("--help"), {.LongOption = TEXT("--help"), .ShortOption = TEXT("-h"), .Type = eOptionType::HELP, .Description = TEXT("Show help information.") } },
        { TEXT("--version"), {.LongOption = TEXT("--version"), .ShortOption = TEXT("-v"), .Type = eOptionType::VERSION, .Description = TEXT("Show version information.") } },
        { TEXT("--input-resource"), {.LongOption = TEXT("--input-resource"), .ShortOption = TEXT("-i"), .Type = eOptionType::INPUT_RESOURCE, .Description = TEXT("Path to the input model file.") } },
        { TEXT("--render-device"), {.LongOption = TEXT("--render-device"), .ShortOption = TEXT("-rd"), .Type = eOptionType::RENDER_DEVICE, .Description = TEXT("Specify the render device.") } }
    };

    CommandLineParser::CommandLineParser(const CommandLineParser::CharType* commandLine) noexcept
        : CommandLineParser()
    {
        const CharType* commandLineCurrentPtr = commandLine;
        while (commandLineCurrentPtr != nullptr && *commandLineCurrentPtr != TEXT('\0'))
        {
            const bool isWhiteSpace = IsSpace(*commandLineCurrentPtr);
            if (isWhiteSpace)
            {
                // Skip whitespace
                ++commandLineCurrentPtr;
                continue;
            }

            // Parse the command line argument
            const CharType* argumentStart = commandLineCurrentPtr;
            while (*commandLineCurrentPtr != TEXT('\0') && !IsSpace(*commandLineCurrentPtr))
            {
                ++commandLineCurrentPtr;
            }
            const CharType* argumentEnd = commandLineCurrentPtr;

            // Process the argument
            const StringViewType argument(argumentStart, argumentEnd);
            mArguments.push_back(StringType(argument));
        }
    }

    bool CommandLineParser::ParseArguments() noexcept
    {
        bool isOptionFound = false;
        const size_t numArguments = mArguments.size();
        for (size_t argumentIndex = 0; argumentIndex < numArguments; ++argumentIndex)
        {
            const StringType& argument = mArguments[argumentIndex];
            if (argument.empty())
            {
                continue;
            }

            const bool isOption = argument.starts_with(TEXT('-'));
            if (isOption == true)
            {
                const OptionInfo& optionInfo = sOptionsMap[StringType(argument)];
                isOptionFound = true;
                switch (optionInfo.Type)
                {
                case cgs::eOptionType::HELP:
                {
                    for (auto& iter : sOptionsMap)
                    {
                        OptionInfo& optInfo = iter.second;
                        if (optInfo.HelpDescription.empty() == true)
                        {
                            static constexpr size_t NUM_OPTION_SPACES = 32;
                            optInfo.HelpDescription = optInfo.ShortOption;
                            optInfo.HelpDescription += TEXT(", ");
                            optInfo.HelpDescription += optInfo.LongOption;
                            if (optInfo.DataType.empty() == false)
                            {
                                optInfo.HelpDescription += TEXT(" <") + optInfo.DataType + TEXT(">");
                            }
                            else if (optInfo.PossibleValues.empty() == false)
                            {
                                optInfo.HelpDescription += TEXT("{");
                                const size_t numValues = optInfo.PossibleValues.size();
                                for (size_t valueIndex = 0; valueIndex < numValues; ++valueIndex)
                                {
                                    const StringType& value = optInfo.PossibleValues[valueIndex];
                                    optInfo.HelpDescription += value;
                                    if (valueIndex < numValues - 1)
                                    {
                                        optInfo.HelpDescription += TEXT(",");
                                    }
                                }
                                optInfo.HelpDescription += TEXT("}");
                            }
                            
                            if (optInfo.HelpDescription.size() < NUM_OPTION_SPACES)
                            {
                                optInfo.HelpDescription.append(NUM_OPTION_SPACES - optInfo.HelpDescription.size(), TEXT(' '));
                            }
                            optInfo.HelpDescription += optInfo.Description;
                        }
                    }
                }
                break;
                case cgs::eOptionType::VERSION:
                {
                    CharType versionString[32] = { 0, };
                    FormatString(versionString, sizeof(versionString) / sizeof(CharType), TEXT("Version: %d.%d.%d.%d"), GET_API_VERSION_VARIANT(cgs::API_VERSION), GET_API_VERSION_MAJOR(cgs::API_VERSION), GET_API_VERSION_MINOR(cgs::API_VERSION), GET_API_VERSION_PATCH(cgs::API_VERSION));
                }
                break;
                case cgs::eOptionType::INPUT_RESOURCE:
                {
                    if (argumentIndex + 1 >= numArguments)
                    {
                        isOptionFound = false;
                        break;
                    }

                    const StringType optionArgument = mArguments[argumentIndex + 1];
                    cgs::gRecentFiles.push_back(optionArgument);
                    mOptionValues[static_cast<uint32>(eOptionType::INPUT_RESOURCE)] = ToString(optionArgument);
                    ++argumentIndex;
                }
                break;
                case cgs::eOptionType::RENDER_DEVICE:
                {
                    if (argumentIndex + 1 >= numArguments)
                    {
                        isOptionFound = false;
                        break;
                    }

                    const StringType optionArgument = mArguments[argumentIndex + 1];
                    mOptionValues[static_cast<uint32>(eOptionType::RENDER_DEVICE)] = ToString(optionArgument);
                    ++argumentIndex;
                }
                break;
                case cgs::eOptionType::INVALID:
                    [[fallthrough]];
                default:
                    isOptionFound = false;
                    break;
                }
            }
        }

     
        return isOptionFound;
    }

    void CommandLineParser::initializeOptionsMap() noexcept
    {
        std::vector<OptionInfo> optionInfos;
        optionInfos.reserve(sOptionsMap.size());
        for (const auto& iter : sOptionsMap)
        {
            const OptionInfo& optionInfo = iter.second;
            optionInfos.push_back(optionInfo);
        }

        for (const auto& optionInfo : optionInfos)
        {
            sOptionsMap[optionInfo.ShortOption] = optionInfo;
        }
    }

    int CommandLineParser::FormatString(CharType* str, const size_t bufferSize, const CharType* format, ...) noexcept
    {
        va_list args;
        va_start(args, format);
#if defined(CGS_WINDOWS)
        const int result = std::vswprintf(str, bufferSize, format, args);
#elif defined(CGS_LINUX)
        const int result = std::vsnprintf(str, bufferSize, format, args);
#else
        #error "Unsupported platform"
#endif
        va_end(args);
        return result;
    }
}