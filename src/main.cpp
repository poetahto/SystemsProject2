#include <string>
#include <fstream>
#include <utility>
#include <vector>
#include <iomanip>
#include <iostream>
#include <unordered_map>

#include "string_parsing_tools.hpp"
#include "instruction_info.hpp"
#include "logger.hpp"
#include "types.hpp"

// todo: clean up

struct InstructionDefinition
{
    InstructionDefinition(std::string name, InstructionInfo::Format format) : name {std::move(name)}, format{format}
    {
    }

    InstructionDefinition() : name {}, format {}
    {
    }

    std::string name;
    InstructionInfo::Format format;
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

// Adds leading ones or zeros to a signed integer (e.g. 12bit -> 16 bit)
int extend(int value, int bits)
{
    bits--;
    bool isNegative {value >> (bits) != 0};

    if (isNegative)
    {
        // Create the filling mask
        int remaining {32 - bits};
        int fillingMask {};

        for (int i {0}; i < remaining; ++i)
        {
            fillingMask = fillingMask << 1;
            fillingMask = fillingMask | 1;
        }

        fillingMask = fillingMask << bits;

        return value | fillingMask;
    }

    return value;
}

static std::unordered_map<int, std::string> s_registerNameMapping {
        {0, "A"},
        {1, "X"},
        {2, "L"},
        {3, "B"},
        {4, "S"},
        {5, "T"},
        {6, "F"},
        {8, "PC"},
        {9, "SW"},
};

void setValueConstant(AssemblyLine& line)
{
    std::string constantHex {line.objectCode.substr(2, 1)};
    int constantValue;
    StringParsingTools::tryGetInt(constantHex, constantValue);
    line.value = std::to_string(constantValue);
}

void setValueRegisterConstant(AssemblyLine& line)
{
    std::string registerHex {line.objectCode.substr(2, 1)};
    int registerValue;
    StringParsingTools::tryGetInt(registerHex, registerValue);
    std::string registerName {s_registerNameMapping[registerValue]};

    std::string constantHex {line.objectCode.substr(3, 1)};
    int constantValue;
    StringParsingTools::tryGetInt(constantHex, constantValue);
    std::string constantName {std::to_string(constantValue + 1)};

    line.value = registerName + "," + constantName;
}

void setValueRegisterMultiple(AssemblyLine& line)
{
    std::string registerHex1 {line.objectCode.substr(2, 1)};
    int registerValue1;
    StringParsingTools::tryGetInt(registerHex1, registerValue1);
    std::string registerName1 {s_registerNameMapping[registerValue1]};

    std::string registerHex2 {line.objectCode.substr(3, 1)};
    int registerValue2;
    StringParsingTools::tryGetInt(registerHex2, registerValue2);
    std::string registerName2 {s_registerNameMapping[registerValue2]};

    line.value = registerName1 + "," + registerName2;
}

void setValueRegister(AssemblyLine& line)
{
    std::string registerHex {line.objectCode.substr(2, 1)};
    int registerValue;
    StringParsingTools::tryGetInt(registerHex, registerValue);

    line.value = s_registerNameMapping[registerValue];
}

// Extracts symbol and literal information from a symbol table file.
bool parseSymbolTableFile(const std::string& fileName, SymbolTableData& outData)
{
    std::ifstream symbolFileStream {fileName};
    std::string line {};

    // Extract all symbols
    {
        auto* symbols = new std::vector<Symbol>();

        // Skip the header
        std::getline(symbolFileStream, line);
        std::getline(symbolFileStream, line);

        while (std::getline(symbolFileStream, line))
        {
            bool failure {false};

            Symbol symbol {};
            StringParsingTools::tryGetArg(line, 0, &symbol.name);
            failure |= !StringParsingTools::tryGetArg(line, 1, &symbol.addressHex);
            failure |= !StringParsingTools::tryGetArg(line, 2, &symbol.flags);
            failure |= !StringParsingTools::tryGetInt(symbol.addressHex, symbol.addressValue);

            if (failure)
                break;

            symbols->emplace_back(symbol);
        }

        outData.symbolCount = symbols->size();
        outData.symbols = symbols->data();
    }

    // Extract all the literals
    {
        auto* literals = new std::vector<Literal>();

        // Skip the header
        std::getline(symbolFileStream, line);
        std::getline(symbolFileStream, line);

        while (std::getline(symbolFileStream, line))
        {
            bool failure {false};

            Literal literal {};
            StringParsingTools::tryGetArg(line, 0, &literal.name);
            failure |= !StringParsingTools::tryGetArg(line, 1, &literal.value);
            failure |= !StringParsingTools::tryGetArg(line, 2, &literal.lengthHex);
            failure |= !StringParsingTools::tryGetArg(line, 3, &literal.addressHex);
            failure |= !StringParsingTools::tryGetInt(literal.addressHex, literal.addressValue);
            failure |= !StringParsingTools::tryGetInt(literal.lengthHex, literal.lengthValue);

            if (failure)
                break;

            literals->emplace_back(literal);
        }

        outData.literalCount = literals->size();
        outData.literals = literals->data();
    }

    return true;
}

std::unordered_map<u8, InstructionDefinition> instructionTable {
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

bool parseObjectCodeFile(const std::string& fileName, const SymbolTableData& symbolData, ObjectCodeData& outData)
{
//    std::unordered_map<u8, InstructionDefinition> instructionTable {};

    // Parse the instructions from a CSV file
    {
//        std::ifstream opCodeTableStream {"opcode_table.csv"};
//
//        if (!opCodeTableStream.is_open())
//        {
//            Logger::log_error("[Disassembler] Failed to open op code table!");
//            return false;
//        }
//
//        std::string line {};
//        std::ofstream output {"fuck_this_class.hpp"};
//        output << "std::unordered_map<u8, InstructionDefinition> instructions {" << std::endl;
//        output << std::setw(4) << "{" << std::endl;
//
//        while (std::getline(opCodeTableStream, line))
//        {
//            InstructionDefinition definition {};
//            tryGetArg(line, 0, &definition.name, ',');
//            std::string format {};
//            tryGetArg(line, 1, &format, ',');
//
//            if (format == "1")
//                definition.format = InstructionInfo::Format::One;
//            else if (format == "2")
//                definition.format = InstructionInfo::Format::Two;
//            else if (format == "3/4")
//                definition.format = InstructionInfo::Format::ThreeOrFour;
//
//            std::string opcodeHex {};
//            tryGetArg(line, 2, &opcodeHex, ',');
//            u8 opcodeValue = std::stoi(opcodeHex, nullptr, 16);
//            instructionTable.emplace(opcodeValue, definition);
//            Logger::log_info("%s: %i", definition.name.c_str(), opcodeValue);
//
//            switch (definition.format)
//            {
//                case InstructionInfo::Format::One:
//                    output << std::setw(8) << "{" << std::to_string(opcodeValue) << ", {\"" << definition.name <<"\", InstructionInfo::Format::One}}," << std::endl;
//                    break;
//                case InstructionInfo::Format::Two:
//                    output << std::setw(8) << "{" << std::to_string(opcodeValue) << ", {\"" << definition.name <<"\", InstructionInfo::Format::Two}}," << std::endl;
//                    break;
//                case InstructionInfo::Format::ThreeOrFour:
//                    output << std::setw(8) << "{" << std::to_string(opcodeValue) << ", {\"" << definition.name <<"\", InstructionInfo::Format::ThreeOrFour}}," << std::endl;
//                    break;
//            }
//        }
//        output << std::setw(4) << "}" << std::endl;
//        output << "};" << std::endl;
    }

    auto* lines = new std::vector<AssemblyLine>;

    // Header information
    std::string headerProgramName {};
    std::string headerStartingAddressHex {};
    int headerLengthBytes {};

    // Do the first pass to determine addressHex, object code, label, and instruction info
    // also we get the header info.
    {
        std::string line {};
        std::ifstream objectCodeStream {fileName};
        size_t currentAddress {};

        while (std::getline(objectCodeStream, line))
        {
            if (line[0] == 'H')
            {
                Logger::log_info("parsing header");
                headerProgramName = line.substr(1, 6);
                headerStartingAddressHex = line.substr(7, 6);

                std::string lengthBytesHex {line.substr(13, 6)};
                StringParsingTools::tryGetInt(lengthBytesHex, headerLengthBytes);

                Logger::log_info("parsed header: %s, starts at %s and has %i bytes", headerProgramName.c_str(), headerStartingAddressHex.c_str(), headerLengthBytes);
            }
            else if (line[0] == 'T')
            {
                Logger::log_info("parsing text record");

                std::string lengthHex {line.substr(7, 2)};
                int lengthValue;
                StringParsingTools::tryGetInt(lengthHex, lengthValue);

                std::string startingAddressHex {line.substr(1, 6)};
                int startingAddressValue;
                StringParsingTools::tryGetInt(startingAddressHex, startingAddressValue);

                currentAddress = startingAddressValue;
                Logger::log_info("start: %s (%i), lengthHex: %s (%i)", startingAddressHex.c_str(), startingAddressValue, lengthHex.c_str(), lengthValue);

                int index {9};
                size_t end = currentAddress + lengthValue;

                while (currentAddress < end)
                {
                    AssemblyLine result {};
                    int start {index};
                    bool isLdb {};
                    AssemblyLine baseInfo {};
                    int currentBase {};

                    // check to see if current addressHex has a label
                    for (int i {0}; i < symbolData.symbolCount; ++i)
                    {
                        if (currentAddress == symbolData.symbols[i].addressValue)
                            result.label = symbolData.symbols[i].name;
                    }

                    // check to see if current addressHex is a literal
                    bool foundLiteral {false};

                    for (int i {0}; i < symbolData.literalCount; ++i)
                    {
                        Literal cur = symbolData.literals[i];

                        if (currentAddress == cur.addressValue)
                        {
                            result.type = AssemblyLine::Type::Literal;
                            result.addressHex = StringParsingTools::getHex(currentAddress);
                            result.addressValue = currentAddress;
                            result.label = cur.name;
                            result.instruction = "BYTE"; // in reality, we can't assume everything is a byte
                            result.value = cur.value;
                            result.objectCode = StringParsingTools::getBetween(cur.value, '\'');
                            index += cur.lengthValue;
                            foundLiteral = true;
                        }
                    }

                    // Only parse an instruction if we didn't have a literal
                    if (!foundLiteral)
                    {
                        // Now we can assume we found an instruction to parse.
                        std::string opCodeHex {line.substr(index, 2)};
                        index += 2;
                        int opCodeAndNI {};
                        StringParsingTools::tryGetInt(opCodeHex, opCodeAndNI);
                        int opCodeValue = opCodeAndNI & 0b11111100;

                        // Make sure our table contains the opcode
                        if (instructionTable.count(opCodeValue) == 0)
                            return false;

                        InstructionDefinition instructionDefinition = instructionTable[opCodeValue];
                        result.type = AssemblyLine::Type::Instruction;
                        result.instruction = instructionDefinition.name;
                        result.addressValue = currentAddress;
                        result.instructionInfo.format = instructionDefinition.format;
                        result.instructionInfo.opcode = opCodeValue;

                        if (instructionDefinition.format == InstructionInfo::Format::Two)
                        {
                            index += 2;
                        }
                        else if (instructionDefinition.format == InstructionInfo::Format::ThreeOrFour)
                        {
                            InstructionInfo::FormatThreeOrFourInfo& info = result.instructionInfo.formatThreeOrFourInfo;
                            info.n = (opCodeAndNI & 0b00000010) != 0;
                            info.i = (opCodeAndNI & 0b00000001) != 0;

                            std::string nixbpeHex {line.substr(index, 1)};
                            index += 1;
                            int nixbpeValue {};
                            StringParsingTools::tryGetInt(nixbpeHex, nixbpeValue);

                            info.x = (nixbpeValue & 0b1000) != 0;
                            info.b = (nixbpeValue & 0b0100) != 0;
                            info.p = (nixbpeValue & 0b0010) != 0;
                            info.e = (nixbpeValue & 0b0001) != 0;
                            index += info.e ? 5 : 3;

                            if (instructionDefinition.name == "LDB")
                            {
                                isLdb = true;
                                baseInfo.addressHex = "";
                                baseInfo.label = "";
                                baseInfo.instruction = "BASE";
                                baseInfo.value = result.value;
                                baseInfo.objectCode = "";
                                baseInfo.type = AssemblyLine::Type::Decoration;
                                StringParsingTools::tryGetInt(baseInfo.value, currentBase);
                            }
                        }

                        result.objectCode = line.substr(start, index - start);
                        result.addressHex = StringParsingTools::getHex(currentAddress);
                    }

                    currentAddress += (index - start) / 2;
                    lines->emplace_back(result);

                    if (isLdb)
                        lines->emplace_back(baseInfo);
                }
            }
        }
    }

    // Finish parsing the instructions, and write into the assembly lines
    int startingAddressValue;
    StringParsingTools::tryGetInt(headerStartingAddressHex, startingAddressValue);

    AssemblyLine header {};
    header.addressHex = "0000";
    header.label = headerProgramName;
    header.instruction = "START";
    header.value = std::to_string(startingAddressValue);
    header.objectCode = "";
    header.type = AssemblyLine::Type::Decoration;
    lines->emplace(lines->begin(), header);

    size_t nextAddress {};
    int currentBase {};
    int currentX {};

    for (int i {0}; i < lines->size(); i++)
    {
        AssemblyLine& line = (*lines)[i];

        if (i < lines->size() - 1)
            nextAddress = (*lines)[i + 1].addressValue;

        if (line.type == AssemblyLine::Type::Instruction)
        {
            if (line.instructionInfo.format == InstructionInfo::Format::One)
            {
                line.value = "";
            }
            if (line.instructionInfo.format == InstructionInfo::Format::Two)
            {
                if (line.instruction == "COMPR") setValueRegisterMultiple(line);
                if (line.instruction == "DIVR") setValueRegisterMultiple(line);
                if (line.instruction == "MULR") setValueRegisterMultiple(line);
                if (line.instruction == "SUBR") setValueRegisterMultiple(line);
                if (line.instruction == "ADDR") setValueRegisterMultiple(line);
                if (line.instruction == "TIXR") setValueRegister(line);
                if (line.instruction == "CLEAR") setValueRegister(line);
                if (line.instruction == "SHIFTL") setValueRegisterConstant(line);
                if (line.instruction == "SHIFTR") setValueRegisterConstant(line);
                if (line.instruction == "SVC") setValueConstant(line);
            }
            if (line.instructionInfo.format == InstructionInfo::Format::ThreeOrFour)
            {
                InstructionInfo::FormatThreeOrFourInfo& info = line.instructionInfo.formatThreeOrFourInfo;

                // Check if base-relative
                if (info.b)
                {
                    Logger::log_info("base rel: %s", line.instruction.c_str());
                    std::string displacementHex {line.objectCode.substr(3, 3)};
                    int displacementValue {};
                    StringParsingTools::tryGetInt(displacementHex, displacementValue);

                    if (info.x)
                        displacementValue += currentX;

                    line.value = StringParsingTools::getHex(displacementValue + currentBase);
                }
                    // Check if PC-relative
                else if (info.p)
                {
                    Logger::log_info("pc rel: %s", line.instruction.c_str());
                    std::string displacementHex {line.objectCode.substr(3, 3)};
                    int displacementValue {};
                    StringParsingTools::tryGetInt(displacementHex, displacementValue);
                    displacementValue = extend(displacementValue, 12);

                    if (info.x)
                        displacementValue += currentX;

                    line.value = StringParsingTools::getHex(displacementValue + nextAddress);
                }
                    // Then we must be direct
                else
                {
                    Logger::log_info("direct: %s", line.instruction.c_str());
                    int length = info.e ? 5 : 3;
                    std::string addressHex {line.objectCode.substr(3, length)};
                    int addressValue;
                    StringParsingTools::tryGetInt(addressHex, addressValue);

                    if (info.x)
                        addressValue += currentX;

                    line.value = StringParsingTools::getHex(addressValue);
                }

                if (line.instruction == "LDB")
                    StringParsingTools::tryGetInt(line.value, currentBase);

                if (line.instruction == "LDX")
                    StringParsingTools::tryGetInt(line.value, currentX);

                if (line.instruction == "ADD")
                    line.instruction = "ADD";

                // Apply decorations

                if (info.i && !info.n) // Immediate
                    line.value.insert(0, "#");

                if (!info.i && info.n) // Indirect
                    line.value.insert(0, "@");
            }

            // Decorate format 4 instructions.
            if (line.instructionInfo.formatThreeOrFourInfo.e)
                line.instruction.insert(0, "+");
        }

        else if (line.instruction == "BASE")
            line.value = StringParsingTools::getHex(currentBase);
    }

    AssemblyLine footer {};
    footer.addressHex = "";
    footer.label = "";
    footer.instruction = "END";
    footer.value = headerProgramName;
    footer.objectCode = "";
    footer.type = AssemblyLine::Type::Decoration;
    lines->emplace_back(footer);

    outData.assemblyLineCount = lines->size();
    outData.assemblyLines = lines->data();
    return true;
}

// Writes assembly listing data to an output file.
void outputObjectCodeData(const ObjectCodeData& data, const std::string& outputFileName)
{
    std::ofstream outputFileStream {outputFileName};
    int tabSize{12};

    for (int i = 0; i < data.assemblyLineCount; ++i)
    {
        AssemblyLine cur {data.assemblyLines[i]};

        outputFileStream << std::setw(tabSize) << std::left << cur.addressHex
                         << std::setw(tabSize) << std::left << cur.label
                         << std::setw(tabSize) << std::left << cur.instruction
                         << std::setw(tabSize) << std::left << cur.value
                         << std::setw(tabSize) << std::left << cur.objectCode
                         << std::endl;
    }
}

int main(int argc, char* argv[])
{
    Logger::enabled = false;

    if (argc != 3)
    {
        printf("usage: ./disassem <object code file> <symbol table file>\n");
        return -1;
    }

    std::string objectCodeFile {argv[1]};
    std::string symbolTableFile {argv[2]};

    SymbolTableData symbolTableData {};
    parseSymbolTableFile(symbolTableFile, symbolTableData);

    ObjectCodeData objectCodeData {};

    if (!parseObjectCodeFile(objectCodeFile, symbolTableData, objectCodeData))
    {
        printf("Failed to parse object code file!\n");
        return -3;
    }

    outputObjectCodeData(objectCodeData, "out.lst");
    return 0;
}