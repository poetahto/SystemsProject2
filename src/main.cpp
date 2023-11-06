#include <string>
#include <fstream>
#include <vector>
#include <iomanip>
#include <iostream>

#include "string_parsing_tools.hpp"
#include "logger.hpp"
#include "types.hpp"

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

bool parseObjectCodeFile(const std::string& fileName, const SymbolTableData& symbolData, ObjectCodeData& outData);

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