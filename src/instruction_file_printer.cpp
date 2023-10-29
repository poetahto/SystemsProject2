#include <iomanip>
#include "instruction_file_printer.hpp"
#include "logger.hpp"

bool InstructionFilePrinter::init(const CreateInfo &createInfo)
{
    m_columnWidth = createInfo.columnWidth;
    m_outputFileName = createInfo.outputFileName;

    // Open the file stream to prepare for writing instructions.
    m_outputFileStream.open(m_outputFileName);

    if (!m_outputFileStream.is_open())
    {
        Logger::log_error("[Instruction Printer] Failed to open output file %s", m_outputFileName);
        return false;
    }

    Logger::log_info("[Instruction Printer] Successfully opened file %s", m_outputFileName);

    // Print the headers
    printRow("INSTR", "FORMAT", "OAT", "TAAM", "OBJ");
    return true;
}

void InstructionFilePrinter::free()
{
    if (m_outputFileStream.is_open())
    {
        Logger::log_info("[Instruction Printer] Successfully closed file %s", m_outputFileName);
        m_outputFileStream.close();
    }
    else
    {
        Logger::log_warning("[Instruction Printer] Tried to free resources, but none existed.");
    }
}

void InstructionFilePrinter::print(const InstructionInfo &instruction)
{
    if (!m_outputFileStream.is_open())
    {
        Logger::log_error("[Instruction Printer] Tried to print an instruction before printer was initialized!");
        return;
    }

    std::string name;
    std::string format;
    std::string operandAddressingType;
    std::string targetAddressAddressingMode;
    std::string objectCode;

    name = instruction.name;
    objectCode = instruction.objectCode;

    if (instruction.format == InstructionInfo::Format::Two)
    {
        format = "2";
        targetAddressAddressingMode = "";
        operandAddressingType = "";
    }
    else if (instruction.format == InstructionInfo::Format::ThreeOrFour)
    {
        InstructionInfo::FormatThreeOrFourInfo info {instruction.formatThreeOrFourInfo};

        if (info.b)
            targetAddressAddressingMode = "base";
        if (info.p)
            targetAddressAddressingMode = "pc";
        if (!info.b && !info.p)
            targetAddressAddressingMode = "absolute";
        if (info.x)
            targetAddressAddressingMode += "_indexed";

        if (info.i)
            operandAddressingType = "immediate";
        if (info.n)
            operandAddressingType = "indirect";
        if ((!info.i && !info.n) || (info.i && info.n))
            operandAddressingType = "simple";

        if (info.e)
            format = "4";

        else format = "3";
    }

    printRow(
            name.c_str(),
            format.c_str(),
            operandAddressingType.c_str(),
            targetAddressAddressingMode.c_str(),
            objectCode.c_str()
    );

    Logger::log_info("[Instruction Printer] Printed \"%s %s %s %s %s\"",
                     name.c_str(),
                     format.c_str(),
                     operandAddressingType.c_str(),
                     targetAddressAddressingMode.c_str(),
                     objectCode.c_str());
}

void InstructionFilePrinter::printRow(const char *c1, const char *c2, const char *c3, const char *c4, const char *c5)
{
    m_outputFileStream << std::setw(m_columnWidth) << c1
                       << std::setw(m_columnWidth) << c2
                       << std::setw(m_columnWidth) << c3
                       << std::setw(m_columnWidth) << c4
                       << std::setw(m_columnWidth) << c5
                       << std::endl;
}
