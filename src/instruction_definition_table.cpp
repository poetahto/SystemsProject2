#include "instruction_definition_table.hpp"

static std::unordered_map<u8, InstructionDefinition> s_instructionTable {
        {
                {24, {"ADD", InstructionInfo::Format::ThreeOrFour}},
                {88, {"ADDF", InstructionInfo::Format::ThreeOrFour}},
                {144, {"ADDR", InstructionInfo::Format::Two}},
                {64, {"AND", InstructionInfo::Format::ThreeOrFour}},
                {180, {"CLEAR", InstructionInfo::Format::Two}},
                {40, {"COMP", InstructionInfo::Format::ThreeOrFour}},
                {136, {"COMPF", InstructionInfo::Format::ThreeOrFour}},
                {160, {"COMPR", InstructionInfo::Format::Two}},
                {36, {"DIV", InstructionInfo::Format::ThreeOrFour}},
                {100, {"DIVF", InstructionInfo::Format::ThreeOrFour}},
                {156, {"DIVR", InstructionInfo::Format::Two}},
                {196, {"FIX", InstructionInfo::Format::One}},
                {192, {"FLOAT", InstructionInfo::Format::One}},
                {244, {"HIO", InstructionInfo::Format::One}},
                {60, {"J", InstructionInfo::Format::ThreeOrFour}},
                {48, {"JEQ", InstructionInfo::Format::ThreeOrFour}},
                {52, {"JGT", InstructionInfo::Format::ThreeOrFour}},
                {56, {"JLT", InstructionInfo::Format::ThreeOrFour}},
                {72, {"JSUB", InstructionInfo::Format::ThreeOrFour}},
                {0, {"LDA", InstructionInfo::Format::ThreeOrFour}},
                {104, {"LDB", InstructionInfo::Format::ThreeOrFour}},
                {80, {"LDCH", InstructionInfo::Format::ThreeOrFour}},
                {112, {"LDF", InstructionInfo::Format::ThreeOrFour}},
                {8, {"LDL", InstructionInfo::Format::ThreeOrFour}},
                {108, {"LDS", InstructionInfo::Format::ThreeOrFour}},
                {116, {"LDT", InstructionInfo::Format::ThreeOrFour}},
                {4, {"LDX", InstructionInfo::Format::ThreeOrFour}},
                {208, {"LPS", InstructionInfo::Format::ThreeOrFour}},
                {32, {"MUL", InstructionInfo::Format::ThreeOrFour}},
                {96, {"MULF", InstructionInfo::Format::ThreeOrFour}},
                {152, {"MULR", InstructionInfo::Format::Two}},
                {200, {"NORM", InstructionInfo::Format::One}},
                {68, {"OR", InstructionInfo::Format::ThreeOrFour}},
                {216, {"RD", InstructionInfo::Format::ThreeOrFour}},
                {172, {"RMO", InstructionInfo::Format::Two}},
                {76, {"RSUB", InstructionInfo::Format::ThreeOrFour}},
                {164, {"SHIFTL", InstructionInfo::Format::Two}},
                {168, {"SHIFTR", InstructionInfo::Format::Two}},
                {240, {"SIO", InstructionInfo::Format::One}},
                {236, {"SSK", InstructionInfo::Format::ThreeOrFour}},
                {12, {"STA", InstructionInfo::Format::ThreeOrFour}},
                {120, {"STB", InstructionInfo::Format::ThreeOrFour}},
                {84, {"STCH", InstructionInfo::Format::ThreeOrFour}},
                {128, {"STF", InstructionInfo::Format::ThreeOrFour}},
                {212, {"STI", InstructionInfo::Format::ThreeOrFour}},
                {20, {"STL", InstructionInfo::Format::ThreeOrFour}},
                {124, {"STS", InstructionInfo::Format::ThreeOrFour}},
                {232, {"STSW", InstructionInfo::Format::ThreeOrFour}},
                {132, {"STT", InstructionInfo::Format::ThreeOrFour}},
                {16, {"STX", InstructionInfo::Format::ThreeOrFour}},
                {28, {"SUB", InstructionInfo::Format::ThreeOrFour}},
                {92, {"SUBF", InstructionInfo::Format::ThreeOrFour}},
                {148, {"SUBR", InstructionInfo::Format::Two}},
                {176, {"SVC", InstructionInfo::Format::Two}},
                {224, {"TD", InstructionInfo::Format::ThreeOrFour}},
                {248, {"TIO", InstructionInfo::Format::One}},
                {44, {"TIX", InstructionInfo::Format::ThreeOrFour}},
                {184, {"TIXR", InstructionInfo::Format::Two}},
                {220, {"WD", InstructionInfo::Format::ThreeOrFour}},
        }
};

InstructionDefinition InstructionDefinitionTable::get(u8 opcode)
{
    return s_instructionTable[opcode];
}

bool InstructionDefinitionTable::contains(u8 opcode)
{
    return s_instructionTable.count(opcode) != 0;
}


InstructionDefinition::InstructionDefinition(std::string name, InstructionInfo::Format format) :
        name {std::move(name)},
        format{format}
{
}

InstructionDefinition::InstructionDefinition() :
        name {},
        format {}
{
}
