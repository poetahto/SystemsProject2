// Helper type definitions
// Date: 28-Oct-23
// Author: Daniel Walls
// RedID: 825776127

#ifndef ASSIG2_TYPES_H
#define ASSIG2_TYPES_H

#include <cstdint>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;

// Tries to extract a word at an index from a line.
inline bool tryGetArg(const std::string& line, size_t index, std::string* outResult, char delimiter = ' ')
{
    size_t argStart {};

    for (size_t i = 0; i < index; ++i)
    {
        // Find the first space between args
        argStart = line.find(delimiter, argStart);

        // Often, we have multiple spaces between args, so skip them all.
        argStart = line.find_first_not_of(' ', argStart) + 1;
    }

    // Make sure we actually found our target: otherwise we failed.
    if (argStart == std::string::npos)
        return false;

    // Now we skip over to the end of the word, *assuming no spaces within*.
    size_t argEnd {line.find(delimiter, argStart)};

    // If we were given a valid buffer, write the result into it.
    if (outResult != nullptr)
        *outResult = line.substr(argStart, argEnd - argStart);

    return true;
}

#endif // ASSIG2_TYPES_H
