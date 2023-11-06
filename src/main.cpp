#include <string>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "logger.hpp"
#include "types.hpp"

bool parseSymbolTableFile(const std::string& fileName, SymbolTableData& outData);
bool parseObjectCodeFile(const std::string& fileName, const SymbolTableData& symbolData, ObjectCodeData& outData);

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

    // Output the results to a text file.
    {
        std::ofstream outputFileStream {"out.lst"};
        int tabSize{12};

        for (int i = 0; i < objectCodeData.assemblyLineCount; ++i)
        {
            AssemblyLine cur = objectCodeData.assemblyLines[i];

            outputFileStream << std::setw(tabSize) << std::left << cur.addressHex
                             << std::setw(tabSize) << std::left << cur.label
                             << std::setw(tabSize) << std::left << cur.instruction
                             << std::setw(tabSize) << std::left << cur.value
                             << std::setw(tabSize) << std::left << cur.objectCode
                             << std::endl;
        }
    }

    return 0;
}