#include <string>
#include <fstream>
#include <vector>
#include "types.hpp"
#include "string_parsing_tools.hpp"

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
