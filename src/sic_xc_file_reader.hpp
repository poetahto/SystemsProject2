// SIC/XC Object File Reader
// Date: 28-Oct-23
// Author: Daniel Walls
// RedID: 825776127

#ifndef ASSIG2_SIC_XC_FILE_READER_HPP
#define ASSIG2_SIC_XC_FILE_READER_HPP

#include <fstream>
#include "types.hpp"
#include "instruction_info.hpp"

// Opens a compiled SIC / XC object file, and extracts information from it.
class SicXcFileReader
{
public:
    struct CreateInfo
    {
        const char *inputFileName{};
    };

    bool init(const CreateInfo &createInfo);
    void free();

    // Try to access the next instruction from the file.
    // Returns true if there was an available instruction, false if the end has been reached.
    bool tryRead(InstructionInfo &outInfo);

private:
    const char* m_inputFileName{};
    std::ifstream m_inputFileStream{};
    s32 m_remainingBytes{};

    void skipToNextTextSegment();
};

#endif // ASSIG2_SIC_XC_FILE_READER_HPP
