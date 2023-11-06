// Lookup table for instruction definitions
// Date: 6-Nov-23
// Author: Daniel Walls
// RedID: 825776127

#ifndef ASSIG2_INSTRUCTION_DEFINITION_TABLE_HPP
#define ASSIG2_INSTRUCTION_DEFINITION_TABLE_HPP

#include <string>
#include <unordered_map>
#include "types.hpp"

// Information about an instructions name and format, keyed to an opcode number in the table.
struct InstructionDefinition
{
    InstructionDefinition(std::string name, InstructionInfo::Format format);
    InstructionDefinition();

    std::string name;
    InstructionInfo::Format format;
};

namespace InstructionDefinitionTable
{
    InstructionDefinition get(u8 opcode);
    bool contains(u8 opcode);
}

#endif // ASSIG2_INSTRUCTION_DEFINITION_TABLE_HPP