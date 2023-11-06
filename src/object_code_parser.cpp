#include <string>
#include <fstream>
#include <vector>

#include "logger.hpp"
#include "types.hpp"
#include "string_parsing_tools.hpp"
#include "instruction_definition_table.hpp"

// Adds leading ones or zeros to a signed integer (e.g. 12bit -> 16 bit)
int extend(int value, int bits);

// Example: SVC n
static void setValueConstant(AssemblyLine& line);

// Example: CLEAR r1
static void setValueRegister(AssemblyLine& line);

// Example: SHIFTL r1,n
static void setValueRegisterConstant(AssemblyLine& line);

// Example: ADDR r1, r2
static void setValueRegisterMultiple(AssemblyLine& line);

bool parseObjectCodeFile(const std::string& fileName, const SymbolTableData& symbolData, ObjectCodeData& outData)
{
    auto* lines = new std::vector<AssemblyLine>;

    // Header information
    std::string headerProgramName {};
    std::string headerStartingAddressHex {};
    int headerLengthBytes {};

    // Do the first pass to determine header info, addressHex, object code, label, and instruction for each line.
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
                // Decode the initial info for the text segment
                Logger::log_info("parsing text record");

                std::string lengthHex {line.substr(7, 2)};
                int lengthValue;
                StringParsingTools::tryGetInt(lengthHex, lengthValue);

                std::string startingAddressHex {line.substr(1, 6)};
                int startingAddressValue;
                StringParsingTools::tryGetInt(startingAddressHex, startingAddressValue);

                currentAddress = startingAddressValue;
                Logger::log_info("start: %s (%i), lengthHex: %s (%i)", startingAddressHex.c_str(), startingAddressValue, lengthHex.c_str(), lengthValue);

                // Now we actually loop over each instruction in the text segment
                int index {9}; // keeps track of the current character in the line
                size_t end = currentAddress + lengthValue;

                while (currentAddress < end)
                {
                    AssemblyLine result {};
                    int start {index};

                    // we keep track of this info here because LDB needs that special "BASE" decoration - we append it later
                    bool isLdb {};
                    AssemblyLine baseInfo {};

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
                            // Luckily, we can immediately decode the entire literal on the spot, no need for a second pass.
                            result.type = AssemblyLine::Type::Literal;
                            result.addressHex = StringParsingTools::getHex(currentAddress);
                            result.addressValue = currentAddress;
                            result.label = cur.name;
                            result.instruction = "BYTE"; // todo: in reality, we shouldn't assume everything is a byte, but this is a lab
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
                        std::string opCodeHex = line.substr(index, 2);
                        index += 2;
                        int opCodeAndNI {};
                        StringParsingTools::tryGetInt(opCodeHex, opCodeAndNI);
                        int opCodeValue = opCodeAndNI & 0b11111100;

                        // Make sure our table contains the opcode
                        if (!InstructionDefinitionTable::contains(opCodeValue))
                            return false;

                        InstructionDefinition instructionDefinition = InstructionDefinitionTable::get(opCodeValue);
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
                            // More complicated case, because the size depends on if we are extended.
                            // Since we decode here, might as well cache all the values.
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

                            // Also, we check here for the LDB outlier (and any other decorations needed in the future)
                            if (instructionDefinition.name == "LDB")
                            {
                                isLdb = true;
                                baseInfo.addressHex = "";
                                baseInfo.label = "";
                                baseInfo.instruction = "BASE";
                                baseInfo.value = result.value;
                                baseInfo.objectCode = "";
                                baseInfo.type = AssemblyLine::Type::Decoration;
                            }
                        }

                        // Regardless of instruction vs. literal, we always calculate objectCode and address the same.
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

    // Do a second pass where we populate all the values for the instructions, and any extra decorations.
    {
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
                    // Format 2 has a lot of annoying edge cases in their formatting - hence this conditional block
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
                    // Format 3/4 is more consistent than 2, but a pain to calculate due to addressing modes.

                    InstructionInfo::FormatThreeOrFourInfo& info = line.instructionInfo.formatThreeOrFourInfo;

                    if (info.b) // Check if base-relative
                    {
                        Logger::log_info("base rel: %s", line.instruction.c_str());
                        std::string displacementHex {line.objectCode.substr(3, 3)};
                        int displacementValue {};
                        StringParsingTools::tryGetInt(displacementHex, displacementValue);

                        if (info.x)
                            displacementValue += currentX;

                        line.value = StringParsingTools::getHex(displacementValue + currentBase);
                    }
                    else if (info.p) // Check if PC-relative
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
                    else // Then we must be direct
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

                    // These instructions do special things and have lasting effects on preceding instructions

                    if (line.instruction == "LDB")
                        StringParsingTools::tryGetInt(line.value, currentBase);

                    if (line.instruction == "LDX")
                        StringParsingTools::tryGetInt(line.value, currentX);

                    // Apply decorations

                    if (info.i && !info.n) // Immediate
                        line.value.insert(0, "#");

                    if (!info.i && info.n) // Indirect
                        line.value.insert(0, "@");
                }

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
    }

    outData.assemblyLineCount = lines->size();
    outData.assemblyLines = lines->data();
    return true;
}

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

// Table that converts the raw value for registers into readable strings.
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

static void setValueRegisterMultiple(AssemblyLine& line)
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

static void setValueRegisterConstant(AssemblyLine& line)
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

static void setValueConstant(AssemblyLine& line)
{
    std::string constantHex {line.objectCode.substr(2, 1)};
    int constantValue;
    StringParsingTools::tryGetInt(constantHex, constantValue);
    line.value = std::to_string(constantValue);
}

static void setValueRegister(AssemblyLine& line)
{
    std::string registerHex {line.objectCode.substr(2, 1)};
    int registerValue;
    StringParsingTools::tryGetInt(registerHex, registerValue);

    line.value = s_registerNameMapping[registerValue];
}
