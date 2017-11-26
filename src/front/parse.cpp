
#include "common/exception.h"
#include "front/parse.h"
#include "parser/AnodeParser.h"


namespace anode { namespace front {

    ast::Module &parseModule(std::istream &inputStream, const std::string &name) {
        error::ErrorStream errorStream{std::cerr};
        return parseModule(inputStream, name, errorStream);
    }

    ast::Module &parseModule(std::istream &inputStream, const std::string &name, error::ErrorStream &errorStream) {
        parser::SourceReader reader{name, inputStream};
        parser::AnodeLexer lexer{reader, errorStream};
        parser::AnodeParser parser{lexer, errorStream};
        ast::Module &module = parser.parseModule();

        if(errorStream.errorCount() > 0) {
            throw ParseAbortedException("Parse aborted.");
        }

        return module;
    }

    ast::Module &parseModule(const std::string &filename)
    {
        std::ifstream inputFileStream{filename};
        if(!inputFileStream) {
            throw ParseAbortedException(std::string("Couldn't open input file: ") + filename);
        }

        return parseModule(inputFileStream, filename);
    }

    ast::Module &parseModule(const std::string &lineOfCode, const std::string &inputName) {
        std::stringstream inputStringStream{lineOfCode};
        return parseModule(inputStringStream, inputName);
    }

}}