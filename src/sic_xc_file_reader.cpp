#include <limits>
#include <unordered_map>
#include <string>
#include <iostream>
#include "sic_xc_file_reader.hpp"
#include "logger.hpp"

using namespace std;

struct InstructionDefinition
{
    string name;
    InstructionInfo::Format format;
};

// Maps the decimal-opcode to extra instruction information.
const static unordered_map<u8, InstructionDefinition> instructionLookup{
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
                {196, {"FIX", InstructionInfo::Format::ThreeOrFour}},
                {192, {"FLOAT", InstructionInfo::Format::ThreeOrFour}},
                {244, {"HIO", InstructionInfo::Format::ThreeOrFour}},
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
                {200, {"NORM", InstructionInfo::Format::ThreeOrFour}},
                {68, {"OR", InstructionInfo::Format::ThreeOrFour}},
                {216, {"RD", InstructionInfo::Format::ThreeOrFour}},
                {172, {"RMO", InstructionInfo::Format::Two}},
                {76, {"RSUB", InstructionInfo::Format::ThreeOrFour}},
                {164, {"SHIFTL", InstructionInfo::Format::Two}},
                {168, {"SHIFTR", InstructionInfo::Format::Two}},
                {240, {"SIO", InstructionInfo::Format::ThreeOrFour}},
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
                {248, {"TIO", InstructionInfo::Format::ThreeOrFour}},
                {44, {"TIX", InstructionInfo::Format::ThreeOrFour}},
                {184, {"TIXR", InstructionInfo::Format::Two}},
                {220, {"WD", InstructionInfo::Format::ThreeOrFour}},
        }
};

bool SicXcFileReader::init(const CreateInfo &createInfo)
{
    m_inputFileName = createInfo.inputFileName;
    m_inputFileStream.open(m_inputFileName, std::ios::binary);

    if (!m_inputFileStream.is_open())
    {
        Logger::log_error("[Disassembler] Failed to open input file %s", m_inputFileName);
        printf("missing the input file\n"); // for autograder?
        return false;
    }

    skipToNextTextSegment();
    Logger::log_info("[Disassembler] Successfully opened file %s, it has %u bytes of text.", m_inputFileName, m_remainingBytes);
    return true;
}

void SicXcFileReader::free()
{
    if (m_inputFileStream.is_open())
    {
        m_inputFileStream.close();
        Logger::log_info("[Disassembler] Successfully closed file %s", m_inputFileName);
    }
    else
    {
        Logger::log_warning("[Disassembler] Tried to free resources, but none existed.");
    }
}

bool SicXcFileReader::tryRead(InstructionInfo &outInfo)
{
    // Return if we ran out of stuff in the file.
    if (!m_inputFileStream || m_remainingBytes <= 0)
        return false;

    outInfo.objectCode = {};

    // Parse the object code from the instruction.
    char opCodeBuffer[3] {};
    m_inputFileStream.read(opCodeBuffer, 2);
    opCodeBuffer[2] = '\0';

    // If we hit the end of a line, go ahead and check for another one, try to parse it.
    if (opCodeBuffer[1] == '\n')
    {
        skipToNextTextSegment();
        return tryRead(outInfo);
    }

    u32 rawOpCode;

    try
    {
        rawOpCode = std::stoi(opCodeBuffer, nullptr, 16);
    }
    catch(...)
    {
        return false;
    }

    u32 opCode = rawOpCode & 0b11111100;
    outInfo.objectCode += opCodeBuffer;
    outInfo.opCode = opCode;

    // Look up extra instruction information based on the opcode.
    if (instructionLookup.count(opCode) == 0)
        return false;

    InstructionDefinition definition = instructionLookup.at(opCode);
    outInfo.format = definition.format;
    outInfo.name = definition.name;

    // If we are format two, we are already done and can return.
    if (definition.format == InstructionInfo::Format::Two)
    {
        char smallBuffer[3];
        m_inputFileStream.read(smallBuffer, 2);
        smallBuffer[2] = '\0';

        outInfo.objectCode += smallBuffer;
        m_remainingBytes -= 2;
        return true;
    }

    // Format three / four have more information that needs to be extracted: start with the packed bits.
    outInfo.formatThreeOrFourInfo.n = (rawOpCode & 0b00000010) != 0;
    outInfo.formatThreeOrFourInfo.i = (rawOpCode & 0b00000001) != 0;

    char xbpeBuffer[2] {};
    m_inputFileStream.read(xbpeBuffer, 1);
    xbpeBuffer[1] = '\0';
    u8 xbpe;

    try
    {
        xbpe = static_cast<u8>(std::stoi(xbpeBuffer, nullptr, 16));
    }
    catch (...)
    {
        return false;
    }

    outInfo.objectCode += xbpeBuffer;

    outInfo.formatThreeOrFourInfo.x = (xbpe & 0b1000) != 0;
    outInfo.formatThreeOrFourInfo.b = (xbpe & 0b0100) != 0;
    outInfo.formatThreeOrFourInfo.p = (xbpe & 0b0010) != 0;
    outInfo.formatThreeOrFourInfo.e = (xbpe & 0b0001) != 0;

    // Figure out if the instruction is extended or not, and read that much data.
    if (outInfo.formatThreeOrFourInfo.e)
    {
        char extendedBuffer[6];
        m_inputFileStream.read(extendedBuffer, 5);
        extendedBuffer[5] = '\0';
        outInfo.objectCode += extendedBuffer;
        m_remainingBytes -= 4;
    }
    else
    {
        char normalBuffer[4];
        m_inputFileStream.read(normalBuffer, 3);
        normalBuffer[3] = '\0';
        outInfo.objectCode += normalBuffer;
        m_remainingBytes -= 3;
    }

    return true;
}

void SicXcFileReader::skipToNextTextSegment()
{
    char lineId;
    m_inputFileStream.read(&lineId, 1);

    if (m_inputFileStream)
    {
        if (lineId != 'T')
        {
            m_inputFileStream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            skipToNextTextSegment();
            return;
        }
    }
    else
    {
        return;
    }

    m_inputFileStream.ignore(6); // Skip the starting address (maybe for now only)
    char sizeBuffer[3];
    m_inputFileStream.read(sizeBuffer, 2);
    sizeBuffer[2] = '\0';
    // Convert the string-based hex characters into an actual integer value.
    m_remainingBytes = static_cast<u16>(std::stoi(sizeBuffer, nullptr, 16));
}
