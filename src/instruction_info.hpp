// SIC/XC Human-readable instruction data
// Date: 28-Oct-23
// Author: Daniel Walls
// RedID: 825776127

#ifndef ASSIG2_INSTRUCTION_INFO_HPP
#define ASSIG2_INSTRUCTION_INFO_HPP

#include <string>
#include "types.hpp"

// A structured representation of an SIC/XC instruction.
// Uses a tagged union to potentially better represent different formats in the future, and clarify
// what data is valid.
struct InstructionInfo
{
    enum class Format
    {
        One,
        Two,
        ThreeOrFour
    };

    struct FormatOneInfo
    {
        // Empty for now
    };
    struct FormatTwoInfo
    {
        // Empty for now
    };
    struct FormatThreeOrFourInfo
    {
        // todo: this could save a lot of memory by leaving this as a single byte and decoding when needed
        bool n, i, x, b, p, e;
    };

    Format format;
    int opcode;

    // Data that may be unique to a certain instruction format.
    union
    {
        FormatOneInfo formatOneInfo;
        FormatTwoInfo formatTwoInfo;
        FormatThreeOrFourInfo formatThreeOrFourInfo;
    };
};

#endif // ASSIG2_INSTRUCTION_INFO_HPP
