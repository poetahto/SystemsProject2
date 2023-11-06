// Helper type definitions
// Date: 28-Oct-23
// Author: Daniel Walls
// RedID: 825776127

#ifndef ASSIG2_TYPES_H
#define ASSIG2_TYPES_H

#include <cstdint>

// Basic types
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;

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

struct Symbol
{
    std::string name;
    std::string addressHex;
    std::string flags;
    int addressValue;
};

struct Literal
{
    std::string name;
    std::string value;
    std::string lengthHex;
    std::string addressHex;
    int lengthValue;
    int addressValue;
};

struct SymbolTableData
{
    u32 symbolCount;
    Symbol* symbols;

    u32 literalCount;
    Literal* literals;
};

struct AssemblyLine
{
    enum class Type
    {
        Instruction,
        Literal,
        Decoration,

    } type;

    std::string addressHex;
    std::string label;
    std::string instruction;
    std::string value;
    std::string objectCode;

    size_t addressValue;
    InstructionInfo instructionInfo;
};

struct ObjectCodeData
{
    u32 assemblyLineCount;
    AssemblyLine* assemblyLines;
};

#endif // ASSIG2_TYPES_H
