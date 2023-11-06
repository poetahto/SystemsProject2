#include "string_parsing_tools.hpp"

bool StringParsingTools::tryGetInt(const std::string& hex, int& outResult)
{
    try
    {
        outResult = std::stoi(hex, nullptr, 16);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

// Finds a substring within two characters.
std::string StringParsingTools::getBetween(const std::string& value, char delimiter)
{
    size_t start {value.find_first_of(delimiter) + 1};
    size_t end {value.find_first_of(delimiter, start)};
    return value.substr(start, end - start);
}

// Tries to extract a word at an index from a line.
bool StringParsingTools::tryGetArg(const std::string& line, size_t index, std::string* outResult, char delimiter)
{
    size_t argStart {};

    for (size_t i = 0; i < index; ++i)
    {
        // Find the first space between args
        argStart = line.find(delimiter, argStart);

        // Often, we have multiple spaces between args, so skip them all.
        argStart = line.find_first_not_of(' ', argStart + 1);
    }

    // Make sure we actually found our target: otherwise we failed.
    if (argStart == std::string::npos)
        return false;

    // Now we skip over to the end of the word, *assuming no spaces within*.
    size_t argEnd {line.find(delimiter, argStart + 1)};

    // If we were given a valid buffer, write the result into it.
    if (outResult != nullptr)
        *outResult = line.substr(argStart, argEnd - argStart);

    return true;
}
