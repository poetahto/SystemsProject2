#include <string>
#include <fstream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <iostream>

#include "sic_xc_file_reader.hpp"
#include "logger.hpp"
#include "global.hpp"

// todo: fix the hex formatting
// todo: fill in the value column
// todo: header start and end

struct Symbol
{
    std::string name;
    std::string addressHex;
    std::string flags;
    int addressValue;
};

struct Literal
{
    std::string name;
    std::string value;
    std::string lengthHex;
    std::string addressHex;
    int lengthValue;
    int addressValue;
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

bool tryGetInt(const std::string& hex, int& outResult)
{
    try
    {
        outResult = std::stoi(hex, nullptr, 16);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

template<typename T>
std::string getHex(T value)
{
    std::ostringstream ss {};
    ss << std::hex << value;
    return ss.str();
}

std::string getBetween(const std::string& value, char delimiter)
{
    size_t start {value.find_first_of(delimiter) + 1};
    size_t end {value.find_first_of(delimiter, start)};
    return value.substr(start, end - start);
}

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
            failure |= !tryGetArg(line, 1, &symbol.addressHex);
            failure |= !tryGetArg(line, 2, &symbol.flags);
            failure |= !tryGetInt(symbol.addressHex, symbol.addressValue);

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
            failure |= !tryGetArg(line, 2, &literal.lengthHex);
            failure |= !tryGetArg(line, 3, &literal.addressHex);
            failure |= !tryGetInt(literal.addressHex, literal.addressValue);
            failure |= !tryGetInt(literal.lengthHex, literal.lengthValue);

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
        // Iterate on symbols
        for (int i = 0; i < symbolData.symbolCount; ++i)
        {
            Symbol cur {symbolData.symbols[i]};
            Logger::log_info("symbol: %s %s %s", cur.name.c_str(), cur.addressHex.c_str(), cur.flags.c_str());
        }

        // Iterate on literals
        for (int i = 0; i < symbolData.literalCount; ++i)
        {
            Literal cur {symbolData.literals[i]};
            Logger::log_info("literal: %s %s %s %s", cur.name.c_str(), cur.value.c_str(), cur.lengthHex.c_str(), cur.addressHex.c_str());
        }
    }

    std::unordered_map<u8, InstructionDefinition> instructionTable {};

    // Parse the instructions from a CSV file
    {
        std::ifstream opCodeTableStream {"opcode_table.csv"};

        if (!opCodeTableStream.is_open())
        {
            Logger::log_error("[Disassembler] Failed to open op code table!");
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
            instructionTable.emplace(opcodeValue, definition);
        }
    }

    std::string line {};
    std::ifstream objectCodeStream {fileName};
    size_t address {};
    auto* lines = new std::vector<AssemblyLine>;

    while (std::getline(objectCodeStream, line))
    {
        // todo: parse header
        if (line[0] == 'H')
        {
            Logger::log_info("parsing header");
        }
        else if (line[0] == 'T')
        {
            Logger::log_info("parsing text record");
            std::string startingAddressHex {line.substr(1, 6)};
            std::string lengthHex {line.substr(7, 2)};
            int startingAddressValue, lengthValue;
            tryGetInt(startingAddressHex, startingAddressValue);
            tryGetInt(lengthHex, lengthValue);
            address = startingAddressValue;
            Logger::log_info("start: %s (%i), lengthHex: %s (%i)", startingAddressHex.c_str(), startingAddressValue, lengthHex.c_str(), lengthValue);

            int index {9};
            size_t end = address + lengthValue;

            while (address < end)
            {
                AssemblyLine result {};
                int start {index};

                // check to see if current addressHex has a label
                for (int i {0}; i < symbolData.symbolCount; ++i)
                {
                    if (address == symbolData.symbols[i].addressValue)
                        result.label = symbolData.symbols[i].name;
                }

                // check to see if current addressHex is a literal
                bool foundLiteral {false};

                for (int i {0}; i < symbolData.literalCount; ++i)
                {
                    Literal cur {symbolData.literals[i]};

                    if (address == cur.addressValue)
                    {
                        result.address = cur.addressHex;
                        result.label = cur.name;
                        result.instruction = "BYTE"; // in reality, we can't assume everything is a byte
                        result.value1 = cur.value;
                        result.objectCode = getBetween(cur.value, '\'');
                        index += cur.lengthValue;
                        foundLiteral = true;
                    }
                }

                // Only parse an instruction if we didn't have a literal
                if (!foundLiteral)
                {
                    // Now we can assume we found an instruction to parse.
                    std::string opCodeHex {line.substr(index, 2)};
                    index += 2;
                    int opCodeValue {};
                    tryGetInt(opCodeHex, opCodeValue);
                    opCodeValue = opCodeValue & 0b11111100;

                    // Make sure our table contains the opcode
                    if (instructionTable.count(opCodeValue) == 0)
                        return false;

                    InstructionDefinition instructionDefinition = instructionTable[opCodeValue];
                    result.instruction = instructionDefinition.name;

                    if (instructionDefinition.format == InstructionInfo::Format::Two)
                    {
                        // todo: handle format 2
                        index += 2;
                    }
                    else if (instructionDefinition.format == InstructionInfo::Format::ThreeOrFour)
                    {
                        bool n, i, x, b, p, e;

                        n = (opCodeValue & 0b00000010) != 0;
                        i = (opCodeValue & 0b00000001) != 0;

                        std::string nixbpeHex {line.substr(index, 1)};
                        index += 1;
                        int nixbpeValue {};
                        tryGetInt(nixbpeHex, nixbpeValue);

                        x = (nixbpeValue & 0b1000) != 0;
                        b = (nixbpeValue & 0b0100) != 0;
                        p = (nixbpeValue & 0b0010) != 0;
                        e = (nixbpeValue & 0b0001) != 0;

                        if (e) index += 5;
                        else index += 3;

                        // todo: handle format 3/4
                    }

                    result.objectCode = line.substr(start, index - start);
                    result.address = getHex(address);
                }

                address += (index - start) / 2;
                lines->emplace_back(result);
            }
        }
    }

    outData.assemblyLineCount = lines->size();
    outData.assemblyLines = lines->data();
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