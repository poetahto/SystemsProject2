// SIC/XC Instruction File Printer
// Date: 25-Sep-23
// Author: Daniel Walls
// RedID: 825776127

#ifndef ASSIG1_INSTRUCTION_FILE_PRINTER_HPP
#define ASSIG1_INSTRUCTION_FILE_PRINTER_HPP

#include <fstream>
#include "global.hpp"
#include "instruction_info.hpp"

// Given an instruction, this class will format and print a human-readable version into a file.
class InstructionFilePrinter
{
public:
    struct CreateInfo
    {
        const char *outputFileName{};
        u8 columnWidth{};
    };

    bool init(const CreateInfo &createInfo);
    void free();
    void print(const InstructionInfo &instruction);

private:
    u8 m_columnWidth{};
    const char* m_outputFileName{};
    std::ofstream m_outputFileStream{};

    void printRow(const char* c1, const char* c2, const char* c3, const char* c4, const char* c5);
};

#endif // ASSIG1_INSTRUCTION_FILE_PRINTER_HPP
