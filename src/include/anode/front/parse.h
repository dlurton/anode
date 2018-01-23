/**
 * This file defines the public interface used by all parts of Anode to convert a series of characters to an AST.
 */

#pragma once

#include "ast/ast.h"
#include <fstream>

#include <string>
#include "front/ErrorKind.h"
#include "front/ErrorStream.h"

namespace anode { namespace front {

ast::Module &parseModule(const std::string &filename);
ast::Module &parseModule(std::istream &inputStream, const std::string &name);
ast::Module &parseModule(std::istream &inputStream, const std::string &name, error::ErrorStream &errorStream);
ast::Module &parseModule(const std::string &lineOfCode, const std::string &inputName);

/** Thrown when the parser has determined that it is unable to continue parsing.*/
class ParseAbortedException : public exception::Exception {
public:
    ParseAbortedException(const std::string &message = std::string(""))
        : Exception(message){
    }
};


}}