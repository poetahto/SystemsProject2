#include <string>
#include <fstream>
#include <vector>
#include <iomanip>

#include "sic_xc_file_reader.hpp"
#include "logger.hpp"
#include "global.hpp"

struct Symbol
{
    std::string name;
    std::string address;
    std::string flags;
};

struct Literal
{
    std::string name;
    std::string value;
    std::string length;
    std::string address;
};

struct SymbolTableData
{
    u32 symbolCount {};
    Symbol* symbols {};

    u32 literalCount {};
    Literal* literals {};
};

struct AssemblyLine
{
    std::string address;
    std::string label;
    std::string instruction;
    std::string value1;
    std::string objectCode;
};

struct ObjectCodeData
{
    u32 assemblyLineCount {};
    AssemblyLine* assemblyLines {};
};

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

            Symbol symbol;
            tryGetArg(line, 0, &symbol.name);
            failure |= !tryGetArg(line, 1, &symbol.address);
            failure |= !tryGetArg(line, 2, &symbol.flags);

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

            Literal literal;
            tryGetArg(line, 0, &literal.name);
            failure |= !tryGetArg(line, 1, &literal.value);
            failure |= !tryGetArg(line, 2, &literal.length);
            failure |= !tryGetArg(line, 3, &literal.address);

            if (failure)
                break;

            literals->emplace_back(literal);
        }

        outData.literalCount = literals->size();
        outData.literals = literals->data();
    }

    return true;
}

bool parseObjectCodeFile(const std::string& fileName, const SymbolTableData& symbolData, ObjectCodeData& outData)
{
    // debug print symbols
    {
//        // Iterate on symbols
//        for (int i = 0; i < symbolData.symbolCount; ++i)
//        {
//            Symbol cur {symbolData.symbols[i]};
//            Logger::log_info("symbol: %s %s %s", cur.name.c_str(), cur.address.c_str(), cur.flags.c_str());
//        }
//
//        // Iterate on literals
//        for (int i = 0; i < symbolData.literalCount; ++i)
//        {
//            Literal cur {symbolData.literals[i]};
//            Logger::log_info("literal: %s %s %s %s", cur.name.c_str(), cur.value.c_str(), cur.length.c_str(), cur.address.c_str());
//        }
    }

    // debug generate data
    {
//        outData.assemblyLineCount = 5;
//        outData.assemblyLines = new AssemblyLine[]
//                {
//                        {"0000", "Assign", "START", "0", ""},
//                        {"0000", "FIRST", "+LDB", "#02C6", ""},
//                        {"0004", "", "BASE", "02C6", "691002C6"},
//                        {"0007", "", "STL", "02C6", "1722BF"},
//                        {"02C7", "", "CLEAR", "A", "B400"},
//                };
    }


    SicXcFileReader fileReader {};
    fileReader.init({fileName.c_str(), "opcode_table.csv"});
    InstructionInfo info {};
    // todo: start and end instructions
    auto* lines = new std::vector<AssemblyLine>;

    while (fileReader.tryRead(info))
    {
        AssemblyLine line {};
        Logger::log_info("%s %i %i %s", info.name.c_str(), info.opCode, info.format, info.objectCode.c_str());
        line.instruction = info.name;
        line.objectCode = info.objectCode;
        lines->emplace_back(line);
    }

    outData.assemblyLineCount = lines->size();
    outData.assemblyLines = lines->data();
    fileReader.free();
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

        outputFileStream << std::setw(tabSize) << std::left << cur.address
                         << std::setw(tabSize) << std::left << cur.label
                         << std::setw(tabSize) << std::left << cur.instruction
                         << std::setw(tabSize) << std::left << cur.value1
                         << std::setw(tabSize) << std::left << cur.objectCode
                         << std::endl;
    }
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printf("usage: ./disassem <object code file> <symbol table file>");
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