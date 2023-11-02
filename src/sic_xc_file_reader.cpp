#include <limits>
#include <unordered_map>
#include <string>
#include <iostream>
#include "sic_xc_file_reader.hpp"
#include "logger.hpp"

using namespace std;

bool SicXcFileReader::init(const CreateInfo &createInfo)
{
    m_inputFileName = createInfo.inputFileName;
    m_inputFileStream.open(m_inputFileName, std::ios::binary);

    if (!m_inputFileStream.is_open())
    {
        Logger::log_error("[Disassembler] Failed to open input file %s", m_inputFileName);
        printf("missing the input file\n");
        return false;
    }

    std::ifstream opCodeTableStream {createInfo.opCodeTableFileName};

    if (!opCodeTableStream.is_open())
    {
        Logger::log_error("[Disassembler] Failed to open op code table %s!", createInfo.opCodeTableFileName);
        return false;
    }

    std::string line {};

    while (std::getline(opCodeTableStream, line))
    {
        InstructionDefinition definition {};
        tryGetArg(line, 0, &definition.name, ',');
        std::string format {};
        tryGetArg(line, 1, &format, ',');

        if (format == "1")
            definition.format = InstructionInfo::Format::One;
        else if (format == "2")
            definition.format = InstructionInfo::Format::Two;
        else if (format == "3/4")
            definition.format = InstructionInfo::Format::ThreeOrFour;

        std::string opcodeHex {};
        tryGetArg(line, 2, &opcodeHex, ',');
        u8 opcodeValue = std::stoi(opcodeHex, nullptr, 16);
        m_instructionTable.emplace(opcodeValue, definition);
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
    if (!m_inputFileStream)
        return false;

    if (m_remainingBytes <= 0)
    {
        m_inputFileStream.ignore(); // skip the newline
        skipToNextTextSegment();
        return tryRead(outInfo);
    }

    outInfo.objectCode = {};

    // Parse the object code from the instruction.
    char opCodeBuffer[3] {};
    m_inputFileStream.read(opCodeBuffer, 2);
    opCodeBuffer[2] = '\0';

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
    if (m_instructionTable.count(opCode) == 0)
        return false;

    InstructionDefinition definition = m_instructionTable.at(opCode);
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
