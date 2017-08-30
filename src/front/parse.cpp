#include "front/parse.h"
#include "LwnnParser.h"


namespace lwnn { namespace front {

    ast::Module *parseModule(std::istream &inputStream, const std::string &name) {

        parser::SourceReader reader{name, inputStream};
        error::ErrorStream errorStream{std::cerr};
        parser::Lexer lexer{reader, errorStream};
        parser::LwnnParser parser{lexer, errorStream};
        ast::Module *expr = parser.parseModule();

        return expr;
    }

    ast::Module *parseModule(const std::string &filename)
    {
        std::ifstream inputFileStream{filename};
        inputFileStream.open(filename, std::ios_base::in);
        if(!inputFileStream.is_open()) {
            throw std::runtime_error(std::string("Couldn't open input file: ") + filename);
        }

        return parseModule(inputFileStream, filename);
    }

    ast::Module *parseModule(const std::string &lineOfCode, const std::string &inputName) {
        std::stringstream inputStringStream{lineOfCode};
        return parseModule(inputStringStream, inputName);
    }

}}