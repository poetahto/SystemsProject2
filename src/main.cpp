#include <iostream>
#include <string>
#include "types.hpp"

struct Symbol
{
    std::string name {};
    std::string address {};
    std::string flags {};
};

struct Literal
{
    std::string name {};
    std::string value {};
    std::string length {};
    std::string address {};
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
    std::string address {};
    std::string label {};
    std::string instruction {};
    std::string value1 {};
    std::string value2 {};
};

struct ObjectCodeData
{
    u32 assemblyLineCount {};
    AssemblyLine* assemblyLines {};
};

bool parseSymbolTableFile(const std::string& fileName, SymbolTableData& outData);

bool parseObjectCodeFile(const std::string& fileName, const SymbolTableData& symbolData, ObjectCodeData& outData);

void outputObjectCodeData(const ObjectCodeData& data, const std::string& outputFileName);

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printf("usage: ./disassem <object code file> <symbol table file>\n");
        return -1;
    }

    std::string objectCodeFile {argv[1]};
    std::string symbolTableFile {argv[2]};

    SymbolTableData symbolTableData {};

    if (!parseSymbolTableFile(symbolTableFile, symbolTableData))
    {
        printf("Failed to parse symbol table file!\n");
        return -2;
    }

    ObjectCodeData objectCodeData {};

    if (!parseObjectCodeFile(objectCodeFile, symbolTableData, objectCodeData))
    {
        printf("Failed to parse object code file!\n");
        return -3;
    }

    outputObjectCodeData(objectCodeData, "out.lst");
    return 0;
}